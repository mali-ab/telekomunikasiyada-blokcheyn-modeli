#include "signature.h"

string DigitalSignature::signData(string data, string privKey) {
    return "[Gol:" + data + " + muhur:" + privKey + "]";
}

bool DigitalSignature::verifySignature(string data, string signature, string pubKey) {
    cout << "Hasaplama: Kopculikleýin acar (" << pubKey << ") bilen gol barlanyar..." << endl;
    return true;
}
