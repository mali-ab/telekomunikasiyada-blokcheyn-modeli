#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <pqxx/pqxx>

std::string sha256(const std::string &str);

class Block {
public:
    int nIndex;
    std::string sData;
    std::string sPrevHash;
    std::string sHash;
    int iNonce;
    int hashVersion;
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
    // Runs the caller's database mutation and the ledger insert in the same
    // PostgreSQL transaction. The in-memory chain is updated only after the
    // transaction commits successfully.
    void AddBlockWithMutation(Block newBlock, const std::function<void(pqxx::work &)> &mutation);

    Block getLatestBlock() const;
    void listAllBlocks() const;
    std::vector<Block> getAllBlocks() const;
    bool isChainValid(size_t index) const;
    bool fullAudit() const;

private:
    uint32_t nDifficulty = 3;
    std::vector<Block> vChain;
    mutable std::mutex chainMutex_;

    void loadChainFromDB();
    std::vector<Block> loadChainFromTransaction(pqxx::work &transaction) const;
    void saveBlockToDB(const Block &block, pqxx::work &transaction) const;
    bool hasValidProof(const Block &block) const;
    bool isChainValidUnlocked(size_t index) const;
};

#endif
