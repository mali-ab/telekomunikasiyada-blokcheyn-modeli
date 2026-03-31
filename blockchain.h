#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

using namespace std;

string sha256(const string str);

class Block {
public:
    string sPrevHash;
    string sHash;
    string sData;
    int iNonce;
    time_t tTime;

    Block(int nIndexIn, const string &sDataIn);

    string CalculateHash() const;
    void MineBlock(uint32_t nDifficulty);
};

class Blockchain {
public:
    Blockchain();
    void AddBlock(Block bNew);

private:
    uint32_t nDifficulty = 3;
    vector<Block> vChain;
};

#endif
