#include "blockchain.h"
#include "database.h"

#include <limits>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {
constexpr uint32_t kDatabaseLedgerLock = 458007;

string canonicalField(const string &name, const string &value) {
    return name + ":" + to_string(value.size()) + ":" + value + "\n";
}
}

// --- SHA-256 hash function using OpenSSL EVP API ---
string sha256(const string &str) {
    EVP_MD_CTX *context = EVP_MD_CTX_new();
    if (!context) throw runtime_error("Could not create SHA-256 context");

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;
    const bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
                    EVP_DigestUpdate(context, str.data(), str.size()) == 1 &&
                    EVP_DigestFinal_ex(context, hash, &lengthOfHash) == 1;
    EVP_MD_CTX_free(context);
    if (!ok) throw runtime_error("SHA-256 calculation failed");

    stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

Block::Block(int indexIn, const string &dataIn)
    : nIndex(indexIn), sData(dataIn), iNonce(-1), hashVersion(2), tTime(time(nullptr)) {}

// Hash version 1 is retained only to audit records written by earlier
// versions. New blocks use a length-prefixed representation so field
// boundaries cannot be ambiguous.
string Block::CalculateHash() const {
    if (hashVersion == 1) {
        stringstream legacy;
        legacy << sPrevHash << tTime << sData << iNonce;
        return sha256(legacy.str());
    }
    if (hashVersion != 2) throw runtime_error("Unsupported block hash version");

    string payload = "telekom-block-v2\n";
    payload += canonicalField("index", to_string(nIndex));
    payload += canonicalField("previous_hash", sPrevHash);
    payload += canonicalField("timestamp", to_string(static_cast<long long>(tTime)));
    payload += canonicalField("data", sData);
    payload += canonicalField("nonce", to_string(iNonce));
    return sha256(payload);
}

void Block::MineBlock(uint32_t difficulty) {
    constexpr size_t kSha256HexLength = 64;
    if (difficulty == 0 || difficulty > kSha256HexLength) {
        throw invalid_argument("Blockchain difficulty must be between 1 and 64");
    }

    const string target(difficulty, '0');
    do {
        if (iNonce == numeric_limits<int>::max()) {
            throw overflow_error("Block nonce space exhausted");
        }
        ++iNonce;
        sHash = CalculateHash();
    } while (sHash.compare(0, difficulty, target) != 0);
}

Blockchain::Blockchain() {
    try {
        lock_guard<mutex> lock(chainMutex_);
        pqxx::work transaction(*DatabaseManager::getConnection());
        transaction.exec("SELECT pg_advisory_xact_lock(" + to_string(kDatabaseLedgerLock) + ")");

        auto chain = loadChainFromTransaction(transaction);
        if (chain.empty()) {
            Block genesis(0, "Genesis Block");
            genesis.sPrevHash = "0";
            genesis.MineBlock(nDifficulty);
            saveBlockToDB(genesis, transaction);
            chain.push_back(std::move(genesis));
        }
        transaction.commit();
        vChain = std::move(chain);
    } catch (const exception &e) {
        throw runtime_error(string("Could not initialize blockchain: ") + e.what());
    }
}

void Blockchain::AddBlock(Block newBlock) {
    AddBlockWithMutation(std::move(newBlock), [](pqxx::work &) {});
}

void Blockchain::AddBlockWithMutation(Block newBlock, const function<void(pqxx::work &)> &mutation) {
    try {
        lock_guard<mutex> lock(chainMutex_);
        pqxx::work transaction(*DatabaseManager::getConnection());
        // This serializes writers across application processes that share the
        // PostgreSQL database, not merely threads in this process.
        transaction.exec("SELECT pg_advisory_xact_lock(" + to_string(kDatabaseLedgerLock) + ")");

        auto persistedChain = loadChainFromTransaction(transaction);
        if (persistedChain.empty()) {
            throw runtime_error("Ledger has no genesis block");
        }

        newBlock.nIndex = static_cast<int>(persistedChain.size());
        newBlock.sPrevHash = persistedChain.back().sHash;
        newBlock.sHash.clear();
        newBlock.iNonce = -1;
        newBlock.hashVersion = 2;
        newBlock.MineBlock(nDifficulty);

        mutation(transaction);
        saveBlockToDB(newBlock, transaction);
        transaction.commit();

        persistedChain.push_back(std::move(newBlock));
        vChain = std::move(persistedChain);
    } catch (const exception &e) {
        throw runtime_error(string("Could not commit ledger event: ") + e.what());
    }
}

void Blockchain::saveBlockToDB(const Block &block, pqxx::work &transaction) const {
    const string sql = "INSERT INTO blocks (prev_hash, block_hash, block_data, timestamp, nonce, hash_version) VALUES (" +
                       transaction.quote(block.sPrevHash) + ", " +
                       transaction.quote(block.sHash) + ", " +
                       transaction.quote(block.sData) + ", " +
                       to_string(static_cast<long long>(block.tTime)) + ", " +
                       to_string(block.iNonce) + ", " +
                       to_string(block.hashVersion) + ");";
    transaction.exec(sql);
}

Block Blockchain::getLatestBlock() const {
    lock_guard<mutex> lock(chainMutex_);
    if (vChain.empty()) throw runtime_error("Blockchain is empty");
    return vChain.back();
}

vector<Block> Blockchain::loadChainFromTransaction(pqxx::work &transaction) const {
    const pqxx::result rows = transaction.exec(
        "SELECT prev_hash, block_hash, block_data, timestamp, nonce, "
        "COALESCE(hash_version, 1) AS hash_version FROM blocks ORDER BY id ASC");

    vector<Block> chain;
    chain.reserve(rows.size());
    for (const auto &row : rows) {
        Block block(static_cast<int>(chain.size()), row["block_data"].as<string>());
        block.sPrevHash = row["prev_hash"].as<string>();
        block.sHash = row["block_hash"].as<string>();
        block.tTime = static_cast<time_t>(row["timestamp"].as<long long>());
        block.iNonce = row["nonce"].is_null() ? -1 : row["nonce"].as<int>();
        block.hashVersion = row["hash_version"].as<int>();
        chain.push_back(std::move(block));
    }
    return chain;
}

void Blockchain::loadChainFromDB() {
    try {
        lock_guard<mutex> lock(chainMutex_);
        pqxx::work transaction(*DatabaseManager::getConnection());
        auto chain = loadChainFromTransaction(transaction);
        transaction.commit();
        vChain = std::move(chain);
        if (!vChain.empty()) {
            cout << "[MAGLUMAT BAZASY]: " << vChain.size() << " blok maglumat bazasyndan ýüklendi." << endl;
        }
    } catch (const exception &e) {
        throw runtime_error(string("Could not load blockchain: ") + e.what());
    }
}

void Blockchain::listAllBlocks() const {
    const auto blocks = getAllBlocks();
    cout << "\n--- BLOKÇEÝN ---" << endl;
    for (const auto &block : blocks) {
        cout << "Indeks: " << block.getIndex() << ", Heş: " << block.sHash
             << ", Maglumat: " << block.sData << endl;
    }
    cout << "-------------------\n" << endl;
}

vector<Block> Blockchain::getAllBlocks() const {
    lock_guard<mutex> lock(chainMutex_);
    return vChain;
}

bool Blockchain::hasValidProof(const Block &block) const {
    if (block.hashVersion != 1 && block.hashVersion != 2) return false;
    // Records made before nonce persistence cannot be replayed exactly. Their
    // hash prefix and link remain visible as a legacy compatibility check.
    if (block.iNonce < 0) {
        return block.sHash.compare(0, nDifficulty, string(nDifficulty, '0')) == 0;
    }
    return block.sHash == block.CalculateHash() &&
           block.sHash.compare(0, nDifficulty, string(nDifficulty, '0')) == 0;
}

bool Blockchain::isChainValidUnlocked(size_t index) const {
    if (index >= vChain.size()) return false;

    const Block &currentBlock = vChain[index];
    if (!hasValidProof(currentBlock)) {
        cout << "[HOWPSUZLYK DUÝDURYŞY]: Blok " << index
             << " maglumaty ýa-da heşi üýtgedilipdir!" << endl;
        return false;
    }

    if (index == 0) return currentBlock.sPrevHash == "0";
    if (currentBlock.sPrevHash != vChain[index - 1].sHash) {
        cout << "[HOWPSUZLYK DUÝDURYŞY]: Blok " << index
             << " zynjyrdan üzüldi (Öňki heş gabat gelmeýär)!" << endl;
        return false;
    }
    return true;
}

bool Blockchain::isChainValid(size_t index) const {
    lock_guard<mutex> lock(chainMutex_);
    return isChainValidUnlocked(index);
}

bool Blockchain::fullAudit() const {
    lock_guard<mutex> lock(chainMutex_);
    for (size_t i = 0; i < vChain.size(); ++i) {
        if (!isChainValidUnlocked(i)) {
            cout << "[KRITIKI]: Blokçeýn manipulýasiýasy ýüze çykaryldy!" << endl;
            return false;
        }
    }
    cout << "[AUDIT]: Blokçeýn doly we dogry." << endl;
    return true;
}
