#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

std::string sha256(const std::string &str);

class Block {
public:
    int nIndex;
    std::string sData;
    std::string sPrevHash;
    std::string sHash;
    int iNonce;
    time_t tTime;

    Block(int indexIn, const std::string &dataIn);

    std::string CalculateHash() const;
    void MineBlock(uint32_t difficulty);
    int getIndex() const { return nIndex; }
};

class Blockchain {
public:
    Blockchain();

    void AddBlock(Block newBlock);

    void saveBlockToDB(const Block &block);
    Block getLatestBlock() const;
    void listAllBlocks() const;
    std::vector<Block> getAllBlocks() const;
    bool isChainValid(size_t index);
    bool fullAudit();

private:
    uint32_t nDifficulty = 3;
    std::vector<Block> vChain;

    void loadChainFromDB();
};

#endif
