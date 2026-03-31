#include <iostream>
#include "blockchain.h"
#include "signature.h"
#include "smartContract.h"

using namespace std;

int main() {
    cout << "--- Turkmentelekom Blockchain Integrated System ---\n" << endl;

    Abonent meredow = {"Meredow", 5.0, false, 0};
    KeyPair user = {"TM-USER-001", "PRIVATE-KEY-777"};

    string transactionData = "Payment: 50 TMT for 30-day Internet";
    double tolegMukdary = 50.0;

    string signature = DigitalSignature::signData(transactionData, user.privateKey);
    cout << "[Step 1] Transaction signed by user." << endl;

    if (DigitalSignature::verifySignature(transactionData, signature, user.publicKey)) {
        cout << "[Step 2] Identity Verified." << endl;

        SmartContract::hyzmatyIslet(meredow, tolegMukdary);
        if (meredow.internetIscenmi) {
            cout << "[Step 3] Smart Contract Conditions Met." << endl;

            Blockchain telekomBC;
            telekomBC.AddBlock(Block(1, transactionData));
            cout << "[Step 4] Transaction permanently recorded in Blockchain." << endl;
        }
    }

    cout << "\n--- System Cycle Complete ---" << endl;
    return 0;
}
