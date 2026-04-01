#include "blockchain.h"
#include "database.h"

string sha256(const string str) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, str.c_str(), str.size());
    EVP_DigestFinal_ex(context, hash, &lengthOfHash);
    EVP_MD_CTX_free(context);

    stringstream ss;
    for(unsigned int i = 0; i < lengthOfHash; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

Block::Block(int nIndexIn, const string &sDataIn) : nIndex(nIndexIn), sData(sDataIn) {
    iNonce = -1;
    tTime = time(nullptr);
}

string Block::CalculateHash() const {
    stringstream ss;
    ss << sPrevHash << tTime << sData << iNonce;
    return sha256(ss.str());
}

void Block::MineBlock(uint32_t nDifficulty) {
    string target(nDifficulty, '0');
    do {
        iNonce++;
        sHash = CalculateHash();
    } while (sHash.substr(0, nDifficulty) != target);

    cout << "[MINING]: Blok gazyldy: " << sHash << endl;
}

Blockchain::Blockchain() {
    try {
        loadChainFromPostgres();
        if (vChain.empty()) {
            Block genesis(0, "Bashlangych Blok (Genesis)");
            genesis.sPrevHash = "0";
            genesis.MineBlock(nDifficulty);
            vChain.push_back(genesis);
            saveBlockToPostgres(genesis);
        }
    } catch (const std::exception &e) {
        cerr << "[ERROR]: " << e.what() << endl;
    }
}

void Blockchain::AddBlock(Block bNew) {
    if (vChain.empty()) {
        bNew.sPrevHash = "0";
    } else {
        bNew.sPrevHash = vChain.back().sHash;
    }
    
    bNew.MineBlock(nDifficulty);
    vChain.push_back(bNew);
    saveBlockToPostgres(bNew);
}

void Blockchain::saveBlockToPostgres(const Block &bNew) {
    try {
        pqxx::connection* conn = DatabaseManager::getConnection();
        pqxx::work W(*conn);
        
        string sql = "INSERT INTO blocks (prev_hash, block_hash, block_data, timestamp) VALUES (" +
                     W.quote(bNew.sPrevHash) + ", " +
                     W.quote(bNew.sHash) + ", " +
                     W.quote(bNew.sData) + ", " +
                     to_string(bNew.tTime) + ");";

        W.exec(sql);
        W.commit();
        cout << "[DATABASE]: Blok bazada saklandy." << endl;
    } catch (const std::exception &e) {
        cerr << "[DB ERROR]: " << e.what() << endl;
    }
}

Block Blockchain::getLatestBlock() const {
    return vChain.back();
}

void Blockchain::loadChainFromPostgres() {
    try {
        pqxx::connection* conn = DatabaseManager::getConnection();
        pqxx::work W(*conn);
        
        pqxx::result R = W.exec("SELECT prev_hash, block_hash, block_data, timestamp FROM blocks ORDER BY id ASC");

        for (auto const &row : R) {
            Block b(vChain.size(), row["block_data"].as<string>());
            
            b.sPrevHash = row["prev_hash"].as<string>();
            b.sHash = row["block_hash"].as<string>();
            b.tTime = row["timestamp"].as<long long>();
            
            vChain.push_back(b);
        }

        if (!vChain.empty()) {
            cout << "[DATABASE]: " << vChain.size() << " sany blok bazadan üstünlikli ýüklendi." << endl;
        }

    } catch (const std::exception &e) {
        cerr << "[DB LOAD ERROR]: " << e.what() << endl;
    }
}

void Blockchain::listAllBlocks() const {
    cout << "\n--- BLOK ZYNJYRY ---" << endl;
    for (const auto &blok : vChain) {
        cout << "Index: " << blok.getIndex() << ", Hash: " << blok.sHash << ", Data: " << blok.sData << endl;
    }
    cout << "-------------------\n" << endl;
}