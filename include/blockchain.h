#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <pqxx/pqxx>

using namespace std;

string sha256(const string str);

class Block {
public:
    int nIndex;
    string sData;
    string sPrevHash;
    string sHash;
    int iNonce;
    time_t tTime;

    Block(int nIndexIn, const string &sDataIn);

    string CalculateHash() const;
    void MineBlock(uint32_t nDifficulty);
    int getIndex() const { return nIndex; }
};

class Blockchain {
public:
    Blockchain();
    
    void AddBlock(Block bNew);
    
    void saveBlockToPostgres(const Block &bNew);
    Block getLatestBlock() const;
    void listAllBlocks() const;
    vector<Block> getAllBlocks() const;
    bool isChainValid(size_t i);
    bool fullAudit();

private:
    uint32_t nDifficulty = 3;
    vector<Block> vChain;

    void loadChainFromPostgres();
};

#endif
