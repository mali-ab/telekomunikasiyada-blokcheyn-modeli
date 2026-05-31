#include "blockchain.h"
#include "database.h"

using namespace std;

// --- SHA-256 hash function using OpenSSL EVP API ---
string sha256(const string &str) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, str.c_str(), str.size());
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);

    stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// --- Block constructor ---
Block::Block(int indexIn, const string &dataIn) : nIndex(indexIn), sData(dataIn) {
    iNonce = -1;
    tTime = time(nullptr);
}

// --- Calculate block hash from its contents ---
string Block::CalculateHash() const {
    stringstream ss;
    ss << sPrevHash << tTime << sData << iNonce;
    return sha256(ss.str());
}

// --- Proof-of-Work mining: find hash with required leading zeros ---
void Block::MineBlock(uint32_t difficulty) {
    string target(difficulty, '0');
    do {
        iNonce++;
        sHash = CalculateHash();
    } while (sHash.substr(0, difficulty) != target);
}

// --- Blockchain constructor: load from DB or create genesis block ---
Blockchain::Blockchain() {
    try {
        loadChainFromDB();
        if (vChain.empty()) {
            Block genesis(0, "Genesis Block");
            genesis.sPrevHash = "0";
            genesis.MineBlock(nDifficulty);
            vChain.push_back(genesis);
            saveBlockToDB(genesis);
        }
    } catch (const std::exception &e) {
        cerr << "[ÝALŇYŞLYK]: " << e.what() << endl;
    }
}

// --- Add a new block to the chain ---
void Blockchain::AddBlock(Block newBlock) {
    if (vChain.empty()) {
        newBlock.sPrevHash = "0";
    } else {
        newBlock.sPrevHash = vChain.back().sHash;
    }

    newBlock.MineBlock(nDifficulty);
    vChain.push_back(newBlock);
    saveBlockToDB(newBlock);
}

// --- Persist a block to the PostgreSQL database ---
void Blockchain::saveBlockToDB(const Block &block) {
    try {
        pqxx::connection* conn = DatabaseManager::getConnection();
        pqxx::work W(*conn);

        string sql = "INSERT INTO blocks (prev_hash, block_hash, block_data, timestamp) VALUES (" +
                     W.quote(block.sPrevHash) + ", " +
                     W.quote(block.sHash) + ", " +
                     W.quote(block.sData) + ", " +
                     to_string(block.tTime) + ");";

        W.exec(sql);
        W.commit();
        cout << "[MAGLUMAT BAZASY]: Blok maglumat bazasyna ýazdyryldy." << endl;
    } catch (const std::exception &e) {
        cerr << "[MB ÝALŇYŞLYK]: " << e.what() << endl;
    }
}

// --- Get the most recent block ---
Block Blockchain::getLatestBlock() const {
    return vChain.back();
}

// --- Load entire chain from database on startup ---
void Blockchain::loadChainFromDB() {
    try {
        pqxx::connection* conn = DatabaseManager::getConnection();
        pqxx::work W(*conn);

        pqxx::result R = W.exec("SELECT prev_hash, block_hash, block_data, timestamp FROM blocks ORDER BY id ASC");

        for (const auto &row : R) {
            Block b(vChain.size(), row["block_data"].as<string>());

            b.sPrevHash = row["prev_hash"].as<string>();
            b.sHash = row["block_hash"].as<string>();
            b.tTime = row["timestamp"].as<long long>();

            vChain.push_back(b);
        }

        if (!vChain.empty()) {
            cout << "[MAGLUMAT BAZASY]: " << vChain.size() << " blok maglumat bazasyndan ýüklendi." << endl;
        }

    } catch (const std::exception &e) {
        cerr << "[MB ÝÜKLEME ÝALŇYŞLYGY]: " << e.what() << endl;
    }
}

// --- Display all blocks in the chain ---
void Blockchain::listAllBlocks() const {
    cout << "\n--- BLOKÇEÝN ---" << endl;
    for (const auto &block : vChain) {
        cout << "Indeks: " << block.getIndex() << ", Heş: " << block.sHash << ", Maglumat: " << block.sData << endl;
    }
    cout << "-------------------\n" << endl;
}

// --- Return copy of the entire chain ---
vector<Block> Blockchain::getAllBlocks() const {
    return vChain;
}

// --- Validate a single block against its predecessor ---
bool Blockchain::isChainValid(size_t index) {
    if (index == 0 || index >= vChain.size()) return true;

    const Block &currentBlock = vChain[index];
    const Block &prevBlock = vChain[index - 1];
    Block recalculated = currentBlock;
    recalculated.MineBlock(nDifficulty);

    if (currentBlock.sHash != recalculated.sHash) {
        cout << "[HOWPSUZLYK DUÝDURYŞY]: Blok " << index << " maglumaty ýa-da heşi üýtgedilipdir!" << endl;
        return false;
    }

    if (currentBlock.sPrevHash != prevBlock.sHash) {
        cout << "[HOWPSUZLYK DUÝDURYŞY]: Blok " << index << " zynjyrdan üzüldi (Öňki heş gabat gelmeýär)!" << endl;
        return false;
    }

    return true;
}

// --- Validate the entire blockchain ---
bool Blockchain::fullAudit() {
    bool totalValid = true;

    if (vChain.empty()) return true;

    for (size_t i = 1; i < vChain.size(); i++) {
        if (!isChainValid(i)) {
            totalValid = false;
            break;
        }
    }

    if (totalValid) {
        cout << "[AUDIT]: Blokçeýn doly we dogry." << endl;
    } else {
        cout << "[KRITIKI]: Blokçeýn manipulýasiýasy ýüze çykaryldy!" << endl;
    }

    return totalValid;
}