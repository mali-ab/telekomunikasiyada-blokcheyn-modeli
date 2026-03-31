#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <iostream>
#include <string>

using namespace std;

struct KeyPair {
    string publicKey;
    string privateKey;
};

class DigitalSignature {
public:
    static string signData(string data, string privKey);

    static bool verifySignature(string data, string signature, string pubKey);
};

#endif
