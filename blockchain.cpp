#include "blockchain.h"

string sha256(const string str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

Block::Block(int nIndexIn, const string &sDataIn) : sData(sDataIn) {
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

    cout << "Blok gazyldy (Mined): " << sHash << endl;
}

Blockchain::Blockchain() {
    vChain.emplace_back(Block(0, "Bashlangych Blok (Genesis)"));
    vChain[0].sPrevHash = "0";
    vChain[0].MineBlock(nDifficulty);
}

void Blockchain::AddBlock(Block bNew) {
    bNew.sPrevHash = vChain.back().sHash;
    bNew.MineBlock(nDifficulty);
    vChain.push_back(bNew);
}
