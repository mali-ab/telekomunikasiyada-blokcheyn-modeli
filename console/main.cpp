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
    cout << "      TÜRKMENTELEKOM BLOKÇEÝN OPERATOR ULGAMY      " << endl;
    cout << "====================================================\n" << endl;

    try {
        DatabaseManager::initTables();

        // --- P2P PORT AND CONNECTION SETUP ---
        int localPort;
        cout << ">>> Bu düwün üçin içki porty giriziň (mysal üçin 8080 ýa-da 8081): ";
        while (!(cin >> localPort)) {
            cout << "San giriziň: ";
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        Blockchain blockchain;
        P2PNode myNode(localPort, blockchain);
        myNode.start_node();

        char connectChoice;
        cout << ">>> Başga bir düwüne birikmelimi? (yes/No): ";
        cin >> connectChoice;
        if (connectChoice == 'y' || connectChoice == 'Y') {
            string remoteHost, remotePort;
            cout << ">>> Daşarky IP adresi (mysal üçin 127.0.0.1): ";
            cin >> remoteHost;
            cout << ">>> Daşarky port (mysal üçin 8080): ";
            cin >> remotePort;
            myNode.connect_to_peer(remoteHost, remotePort);
        }

        // --- MAIN LOOP ---
        while (true) {
            cout << "\n--- ESASY MENÝU (Içki Port: " << localPort << ") ---" << endl;
            cout << "1. Täze töleg" << endl;
            cout << "2. Abonent maglumaty (tapylmasa döret)" << endl;
            cout << "3. Internet tizligini üýtget" << endl;
            cout << "4. IPTV kanal sanyny üýtget" << endl;
            cout << "5. Işjeň P2P düwünlerini görkez" << endl;
            cout << "6. Blokçeýni görkez" << endl;
            cout << "7. Blokçeýniň bitewiligini barlamak" << endl;
            cout << "8. Doly ulgam audity (DB vs Blokçeýn)" << endl;
            cout << "9. Çykyş" << endl;
            cout << "Siziň saýlawyňyz: ";

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
                cout << "\nAbonentiň adyny giriziň: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                if (SmartContract::getSubscriberFromDB(subscriberName, sub)) {
                    cout << "[MAGLUMAT]: Abonent tapyldy." << endl;
                    cout << "[MAGLUMAT]: IPTV kanallary: " << sub.iptv.tvCount << endl;

                    cout << "Täze IPTV kanal sany (1-10): ";
                    int newCount;
                    while (!(cin >> newCount) || newCount < 1 || newCount > 10) {
                        cout << "1 we 10 aralygynda san giriziň: ";
                        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                    if (sub.iptv.setTVCount(newCount)) {
                        SmartContract::updateSubscriberInDB(sub);
                        cout << "[MAGLUMAT]: IPTV kanal sany täzelendi!" << endl;
                    }
                } else {
                    cout << "[ÝALŇYŞLYK]: Abonent tapylmady!" << endl;
                }
                break;
            }

            case 3: {
                string subscriberName;
                cout << "\nAbonentiň adyny giriziň: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                if (SmartContract::getSubscriberFromDB(subscriberName, sub)) {
                    cout << "[MAGLUMAT]: Abonent tapyldy." << endl;
                    cout << "[MAGLUMAT]: Internet tizligi: " << sub.internet.speed << " Mbit/s" << endl;

                    cout << "Täze internet tizligi (1, 2, 4 ýa-da 6 Mbit/s): ";
                    string newSpeed;
                    while (!(cin >> newSpeed) || (newSpeed != "1" && newSpeed != "2" && newSpeed != "4" && newSpeed != "6")) {
                        cout << "1, 2, 4 ýa-da 6 giriziň: ";
                        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                    if (sub.internet.setSpeed(newSpeed)) {
                        SmartContract::updateSubscriberInDB(sub);
                        cout << "[MAGLUMAT]: Internet tizligi täzelendi!" << endl;
                    }
                } else {
                    cout << "[ÝALŇYŞLYK]: Abonent tapylmady!" << endl;
                }
                break;
            }

            case 2: {
                string subscriberName;
                cout << "\nAbonentiň adyny giriziň: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                if (SmartContract::getSubscriberFromDB(subscriberName, sub)) {
                    cout << "[MAGLUMAT]: Abonent tapyldy." << endl;
                    cout << "[MAGLUMAT]: Balans: " << sub.balance << " TMT" << endl;
                    cout << "[MAGLUMAT]: Internet: " << (sub.internetExpiry != "Inactive" ? "Işjeň. Tizlik " + sub.internet.speed + " Mbit/s. Galan günler: " + to_string(sub.remainingDays(sub.internetExpiry)) : "Işjeň däl") << endl;
                    cout << "[MAGLUMAT]: IP-TV: " << (sub.iptvExpiry != "Inactive" ? "Işjeň. Galan günler: " + to_string(sub.remainingDays(sub.iptvExpiry)) : "Işjeň däl") << endl;
                    cout << "[MAGLUMAT]: Telefon: " << (sub.phoneExpiry != "Inactive" ? "Işjeň. Galan günler: " + to_string(sub.remainingDays(sub.phoneExpiry)) : "Işjeň däl") << endl;
                } else {
                    SmartContract::updateSubscriberInDB(Subscriber(subscriberName));
                    cout << "[MAGLUMAT]: Täze abonent hasaba alyndy!" << endl;
                }
                break;
            }

            case 1: {
                string subscriberName;
                cout << "\nAbonentiň adyny giriziň: ";
                getline(cin, subscriberName);

                Subscriber sub(subscriberName);
                cout << "[MAGLUMAT]: Abonentiň balanysy: " << sub.balance << " TMT" << endl;

                int serviceChoice;
                cout << "\nHyzmaty saýlaň:\n";
                cout << "1. Internet " + to_string(sub.internet.getPrice()) + " TMT" << endl;
                cout << "2. IPTV " + to_string(sub.iptv.getPrice()) + " TMT" << endl;
                cout << "3. Telefon " + to_string(sub.phone.getPrice()) + " TMT" << endl;
                cout << "Saýlawyňyz: ";
                while (!(cin >> serviceChoice) || serviceChoice < 1 || serviceChoice > 3) {
                    cout << "Nädogry saýlaw! (1-3): ";
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }

                double paymentAmount;
                cout << "Töleg möçberi: ";
                while (!(cin >> paymentAmount)) {
                    cout << "San giriziň: ";
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
                    cout << "[MAGLUMAT]: RSA GOLY TASSYKLANDY." << endl;

                    SmartContract::processService(sub, paymentAmount, serviceType);
                    blockchain.AddBlock(Block(blockchain.getLatestBlock().getIndex() + 1, txData));
                    string p2pMsg = "NEW_BLOCK|" + txData + "|" + sigStr + "|" + to_string(keys.e) + "|" + to_string(keys.n);
                    myNode.broadcast_message(p2pMsg);
                    cout << "[ÜSTÜNLIK]: Blok tora ýaýradyldy." << endl;
                } else {
                    cout << "[ÝALŇYŞLYK]: Kriptografik barlag şowsuz!" << endl;
                }
                break;
            }

            default:
                cout << "[ÝALŇYŞLYK]: Nädogry menýu saýlawy." << endl;
                break;
            }
        }
        exit_loop:;

    } catch (const std::exception &e) {
        cerr << "\n[KRITIKI ÝALŇYŞLYK]: " << e.what() << endl;
        return 1;
    }

    cout << "====================================================" << endl;
    cout << "                   ULGAM ÝAPYLDY                     " << endl;
    cout << "====================================================\n" << endl;
    return 0;
}
