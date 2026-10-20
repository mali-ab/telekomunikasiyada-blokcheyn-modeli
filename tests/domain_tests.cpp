#include "blockchain.h"
#include "signature.h"
#include "smartContract.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

Subscriber subscriber(const std::string &name) {
    Subscriber value(name, false);
    value.internet.setSpeed("1");
    value.iptv.setTVCount(0);
    return value;
}
}

int main() {
    try {
        const std::time_t now = std::time(nullptr);

        Subscriber newSubscriber = subscriber("Aman Orazow");
        std::string error;
        require(!SmartContract::processService(newSubscriber, -1.0, ServiceType::INTERNET, &error, now),
                "Negative payment was accepted");
        require(newSubscriber.balance == 0.0 && newSubscriber.internetExpiry == "Inactive",
                "Rejected payment changed subscriber state");

        require(SmartContract::processService(newSubscriber, 200.0, ServiceType::INTERNET, &error, now),
                "Valid Internet activation was rejected");
        require(std::abs(newSubscriber.balance - 50.0) < 0.001, "Activation balance is incorrect");
        require(newSubscriber.remainingDays(newSubscriber.internetExpiry) >= 29,
                "Activation did not create a 30-day service period");

        Subscriber renewal = subscriber("Mähri Döwletowa");
        renewal.internetExpiry = SmartContract::formatTimestamp(now + 5 * 24 * 60 * 60);
        require(SmartContract::processService(renewal, 150.0, ServiceType::INTERNET, &error, now),
                "Valid renewal was rejected");
        require(std::abs(renewal.balance) < 0.001, "Renewal balance is incorrect");
        require(renewal.remainingDays(renewal.internetExpiry) >= 34,
                "Renewal did not preserve the remaining service period");

        Subscriber invalidPlan = subscriber("Serdar Geldiýew");
        require(!SmartContract::processService(invalidPlan, 10.0, ServiceType::IPTV, &error, now),
                "Zero-channel IPTV plan was accepted");

        Block block(1, "canonical payload");
        block.sPrevHash = "previous";
        block.tTime = 1;
        block.MineBlock(1);
        const std::string originalHash = block.sHash;
        require(block.CalculateHash() == originalHash, "Mined block hash cannot be reproduced");
        block.sData = "tampered payload";
        require(block.CalculateHash() != originalHash, "Block payload tampering was not detected");

        const RSAKeys keys = DigitalSignature::generateKeys();
        const std::string utf8Message = "Türkmenistanyň telekomy";
        const auto signature = DigitalSignature::encrypt(utf8Message, keys.d, keys.n);
        require(DigitalSignature::decrypt(signature, keys.e, keys.n) == utf8Message,
                "RSA byte round-trip failed for UTF-8 data");

        std::cout << "All domain tests passed.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Domain test failed: " << e.what() << '\n';
        return 1;
    }
}
