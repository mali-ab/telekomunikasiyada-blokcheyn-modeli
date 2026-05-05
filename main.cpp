#include <iostream>
#include <vector>
#include <limits>
#include <string>

#include "blockchain.h"
#include "signature.h"
#include "smartContract.h"
#include "database.h"
#include "p2p.h"

using namespace std;

int main() {
    cout << "====================================================" << endl;
    cout << "      TURKMENTELEKOM BLOCKCHAIN OPERATOR SYSTEM      " << endl;
    cout << "====================================================\n" << endl;

    try {
        DatabaseManager::initTables();

        // --- P2P PORT AND CONNECTION SETUP ---
        int localPort;
        cout << ">>> Enter local port for this node (e.g. 8080 or 8081): ";
        while (!(cin >> localPort)) {
            cout << "Enter a number: ";
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        Blockchain blockchain;
        P2PNode myNode(localPort, blockchain);
        myNode.start_node();

        char connectChoice;
        cout << ">>> Connect to another peer node? (y/n): ";
        cin >> connectChoice;
        if (connectChoice == 'y' || connectChoice == 'Y') {
            string remoteHost, remotePort;
            cout << ">>> Remote IP address (e.g. 127.0.0.1): ";
            cin >> remoteHost;
            cout << ">>> Remote port (e.g. 8080): ";
            cin >> remotePort;
            myNode.connect_to_peer(remoteHost, remotePort);
        }

        // --- MAIN LOOP ---
        while (true) {
            cout << "\n--- MAIN MENU (Local Port: " << localPort << ") ---" << endl;
            cout << "1. New payment" << endl;
            cout << "2. Subscriber info (create if not found)" << endl;
            cout << "3. Change internet speed" << endl;
            cout << "4. Change IPTV channel count" << endl;
            cout << "5. List active P2P peers" << endl;
            cout << "6. Display blockchain" << endl;
            cout << "7. Audit blockchain integrity" << endl;
            cout << "8. Full system audit (DB vs Blockchain)" << endl;
            cout << "9. Exit" << endl;
            cout << "Your choice: ";

            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore();

            switch (choice) {
            case 9:
                goto exit_loop;

            case 8:
                SmartContract::fullSystemAudit(blockchain);
                break;

            case 7:
                blockchain.fullAudit();
                break;

            case 6:
                blockchain.listAllBlocks();
                break;

            case 5:
                myNode.list_active_peers();
                break;

            case 4: {
                string subscriberName;
                cout << "\nEnter subscriber name: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                if (SmartContract::getSubscriberFromDB(subscriberName, sub)) {
                    cout << "[INFO]: Subscriber found." << endl;
                    cout << "[INFO]: IPTV channels: " << sub.iptv.tvCount << endl;

                    cout << "New IPTV channel count (1-10): ";
                    int newCount;
                    while (!(cin >> newCount) || newCount < 1 || newCount > 10) {
                        cout << "Enter a number between 1 and 10: ";
                        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                    if (sub.iptv.setTVCount(newCount)) {
                        SmartContract::updateSubscriberInDB(sub);
                        cout << "[INFO]: IPTV channel count updated!" << endl;
                    }
                } else {
                    cout << "[ERROR]: Subscriber not found!" << endl;
                }
                break;
            }

            case 3: {
                string subscriberName;
                cout << "\nEnter subscriber name: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                if (SmartContract::getSubscriberFromDB(subscriberName, sub)) {
                    cout << "[INFO]: Subscriber found." << endl;
                    cout << "[INFO]: Internet speed: " << sub.internet.speed << " Mbit/s" << endl;

                    cout << "New internet speed (1, 2, 4 or 6 Mbit/s): ";
                    string newSpeed;
                    while (!(cin >> newSpeed) || (newSpeed != "1" && newSpeed != "2" && newSpeed != "4" && newSpeed != "6")) {
                        cout << "Enter 1, 2, 4 or 6: ";
                        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                    if (sub.internet.setSpeed(newSpeed)) {
                        SmartContract::updateSubscriberInDB(sub);
                        cout << "[INFO]: Internet speed updated!" << endl;
                    }
                } else {
                    cout << "[ERROR]: Subscriber not found!" << endl;
                }
                break;
            }

            case 2: {
                string subscriberName;
                cout << "\nEnter subscriber name: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                if (SmartContract::getSubscriberFromDB(subscriberName, sub)) {
                    cout << "[INFO]: Subscriber found." << endl;
                    cout << "[INFO]: Balance: " << sub.balance << " TMT" << endl;
                    cout << "[INFO]: Internet: " << (sub.internetExpiry != "Inactive" ? "Active. Speed " + sub.internet.speed + " Mbit/s. Days left: " + to_string(sub.remainingDays(sub.internetExpiry)) : "Inactive") << endl;
                    cout << "[INFO]: IP-TV: " << (sub.iptvExpiry != "Inactive" ? "Active. Days left: " + to_string(sub.remainingDays(sub.iptvExpiry)) : "Inactive") << endl;
                    cout << "[INFO]: Phone: " << (sub.phoneExpiry != "Inactive" ? "Active. Days left: " + to_string(sub.remainingDays(sub.phoneExpiry)) : "Inactive") << endl;
                } else {
                    SmartContract::updateSubscriberInDB(Subscriber(subscriberName));
                    cout << "[INFO]: New subscriber registered!" << endl;
                }
                break;
            }

            case 1: {
                string subscriberName;
                cout << "\nEnter subscriber name: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                cout << "[INFO]: Subscriber balance: " << sub.balance << " TMT" << endl;

                int serviceChoice;
                cout << "\nSelect service:\n";
                cout << "1. Internet " + to_string(sub.internet.getPrice()) + " TMT" << endl;
                cout << "2. IPTV " + to_string(sub.iptv.getPrice()) + " TMT" << endl;
                cout << "3. Phone " + to_string(sub.phone.getPrice()) + " TMT" << endl;
                cout << "Choice: ";
                while (!(cin >> serviceChoice) || serviceChoice < 1 || serviceChoice > 3) {
                    cout << "Invalid choice! (1-3): ";
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }

                double paymentAmount;
                cout << "Payment amount: ";
                while (!(cin >> paymentAmount)) {
                    cout << "Enter a number: ";
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }

                ServiceType serviceType = static_cast<ServiceType>(serviceChoice);
                string txData = sub.getTransactionData(serviceType, paymentAmount);

                RSAKeys keys = DigitalSignature::getKeys("keys_" + sub.name + ".txt");
                vector<long long> signature = DigitalSignature::encrypt(txData, keys.d, keys.n);
                string decryptedData = DigitalSignature::decrypt(signature, keys.e, keys.n);

                string sigStr;
                sigStr.reserve(signature.size() * 6);
                for (size_t i = 0; i < signature.size(); i++) {
                    sigStr += to_string(signature[i]);
                    if (i < signature.size() - 1) sigStr += ",";
                }

                if (decryptedData == txData) {
                    cout << "[INFO]: RSA SIGNATURE VERIFIED." << endl;

                    SmartContract::processService(sub, paymentAmount, serviceType);
                    blockchain.AddBlock(Block(blockchain.getLatestBlock().getIndex() + 1, txData));
                    string p2pMsg = "NEW_BLOCK|" + txData + "|" + sigStr + "|" + to_string(keys.e) + "|" + to_string(keys.n);
                    myNode.broadcast_message(p2pMsg);
                    cout << "[SUCCESS]: Block broadcast to network." << endl;
                } else {
                    cout << "[ERROR]: Cryptographic verification failed!" << endl;
                }
                break;
            }

            default:
                cout << "[ERROR]: Invalid menu option." << endl;
                break;
            }
        }
        exit_loop:;

    } catch (const std::exception &e) {
        cerr << "\n[CRITICAL ERROR]: " << e.what() << endl;
        return 1;
    }

    cout << "====================================================" << endl;
    cout << "                   SYSTEM CLOSED                     " << endl;
    cout << "====================================================\n" << endl;
    return 0;
}
