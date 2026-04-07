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
    cout << "      TURKMENTELEKOM BLOKCEYN OPERATOR ULGAMY       " << endl;
    cout << "====================================================\n" << endl;

    try {
        DatabaseManager::initTables();
        
        // --- TÄZE: P2P PORT WE BIRIKME SAZLAMALARY ---
        int localPort;
        cout << ">>> Bu düwün üçin lokal porty giriziň (meselem: 8080 ýa-da 8081): ";
        while (!(cin >> localPort)) {
            cout << "San giriziň: ";
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        Blockchain telekomBC;
        P2PNode myNode(localPort, telekomBC);
        myNode.start_node();

        char birikmeSaylaw;
        cout << ">>> Başga bir düwüne (peer) birikmek isleýärsiňizmi? (h/ý): ";
        cin >> birikmeSaylaw;
        if (birikmeSaylaw == 'h' || birikmeSaylaw == 'H') {
            string remoteHost, remotePort;
            cout << ">>> Birikmeli IP adres (meselem: 127.0.0.1): ";
            cin >> remoteHost;
            cout << ">>> Birikmeli port (meselem: 8080): ";
            cin >> remotePort;
            myNode.connect_to_peer(remoteHost, remotePort);
        }

        // ----------------------------------------------

        while (true) {
            cout << "\n--- ESASY MENYU (Lokal Port: " << localPort << ") ---" << endl;
            cout << "1. Täze töleg" << endl;
            cout << "2. Abonent maglumatlary. Yok bolsa doretmek" << endl;
            cout << "3. Internet tizligini üýtgetmek" << endl;
            cout << "4. IPTV sanyny üýtgetmek" << endl;
            cout << "5. Aktiw P2P düwünlerini görkez (List Peers)" << endl;
            cout << "6. Zynjyry gorkez" << endl;
            cout << "7. Zynjyry barlamak (Audit Chain)" << endl;
            cout << "8. Ahli ulanyjylary barlamak (Full Audit)" << endl;
            cout << "9. Programmadan çykmak" << endl;
            cout << "Saýlawyňyz: ";

            int anaSaylaw;
            if (!(cin >> anaSaylaw)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore(); 

            if (anaSaylaw == 9) break;

            if (anaSaylaw == 8) {
                SmartContract::fullSystemAudit(telekomBC);
                continue;
            }

            if (anaSaylaw == 7) {
                telekomBC.fullAudit();
                continue;
            }

            if (anaSaylaw == 6) {
                telekomBC.listAllBlocks();
                continue;
            }

            if (anaSaylaw == 5) {
                myNode.list_active_peers();
                continue;
            }

            if (anaSaylaw == 4) {
                string abonentAdy;
                cout << "\nAbonent adyny giriziň: ";
                getline(cin, abonentAdy);

                Abonent ulanyjy(abonentAdy);
                if (SmartContract::getAbonentFromDB(abonentAdy, ulanyjy)) {
                    cout << "[INFO]: Abonent tapyldy." << endl;
                    cout << "[INFO]: IPTV sany: " << ulanyjy.iptv.tvSany << endl;

                    cout << "Täze IPTV sany (1-10): ";
                    int newTVsany;
                    while (!(cin >> newTVsany) || newTVsany < 1 || newTVsany > 10) {
                        cout << "1-den 10-a çenli san giriziň: ";
                        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                    if (ulanyjy.iptv.setTVsany(newTVsany)) {
                        SmartContract::updateAbonentInDB(ulanyjy);
                        cout << "[INFO]: IPTV sany üýtgedildi!" << endl;
                    }
                } else {
                    cout << "[ERROR]: Abonent tapylmady!" << endl;
                }
                continue;
            }

            if (anaSaylaw == 3) {
                string abonentAdy;
                cout << "\nAbonent adyny giriziň: ";
                getline(cin, abonentAdy);

                Abonent ulanyjy(abonentAdy);
                if (SmartContract::getAbonentFromDB(abonentAdy, ulanyjy)) {
                    cout << "[INFO]: Abonent tapyldy." << endl;
                    cout << "[INFO]: Internet tizligi: " << ulanyjy.internet.tizlik << " Mbit/s" << endl;

                    cout << "Täze internet tizligi (1, 2, 4 ýa-da 6 Mbit/s): ";
                    string newTizlik;
                    while (!(cin >> newTizlik) || (newTizlik != "1" && newTizlik != "2" && newTizlik != "4" && newTizlik != "6")) {
                        cout << "1, 2, 4 ýa-da 6 giriziň: ";
                        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                    if (ulanyjy.internet.setTizlik(newTizlik)) {
                        SmartContract::updateAbonentInDB(ulanyjy);
                        cout << "[INFO]: Internet tizligi üýtgedildi!" << endl;
                    }
                }else {
                    cout << "[ERROR]: Abonent tapylmady!" << endl;
                }
                continue;
            }

            if (anaSaylaw == 2) {
                string abonentAdy;
                cout << "\nAbonent adyny giriziň: ";
                getline(cin, abonentAdy);

                Abonent ulanyjy(abonentAdy);
                if (SmartContract::getAbonentFromDB(abonentAdy, ulanyjy)) {
                    cout << "[INFO]: Abonent tapyldy." << endl;
                    cout << "[INFO]: Balans: " << ulanyjy.balans << " TMT" << endl;
                    cout << "[INFO]: Internet: " << (ulanyjy.internetWagty != "Işjeň däl" ? "Işjeň. Internet " + ulanyjy.internet.tizlik + " Mbit/s. Galan gun: " + to_string(ulanyjy.galanGun(ulanyjy.internetWagty)) : "Işjeň däl") << endl;
                    cout << "[INFO]: IP-TV: " << (ulanyjy.iptvWagty != "Işjeň däl" ? "Işjeň. Galan gun: " + to_string(ulanyjy.galanGun(ulanyjy.iptvWagty)) : "Işjeň däl") << endl;
                    cout << "[INFO]: Telefon: " << (ulanyjy.telefonWagty != "Işjeň däl" ? "Işjeň. Galan gun: " + to_string(ulanyjy.galanGun(ulanyjy.telefonWagty)) : "Işjeň däl") << endl;
                } else {
                    SmartContract::updateAbonentInDB(Abonent(abonentAdy));
                    cout << "[INFO]: Abonent taze gosuldy!" << endl;
                }
                continue;
            }

            if (anaSaylaw == 1) {
                string abonentAdy;
                cout << "\nAbonent adyny giriziň: ";
                getline(cin, abonentAdy);

                Abonent ulanyjy(abonentAdy);

                cout << "[INFO]: Abonentin balansy: " << ulanyjy.balans << " TMT" << endl;

                int saylaw;
                cout << "\nHyzmat saýlaň:\n";
                cout << "1. Internet " + to_string(ulanyjy.internet.getBaha()) + " TMT" << endl;
                cout << "2. IPTV " + to_string(ulanyjy.iptv.getBaha()) + " TMT" << endl;
                cout << "3. Telefon " + to_string(ulanyjy.telefon.getBaha()) + " TMT" << endl;

                cout << "Saýlaw: ";
                while (!(cin >> saylaw) || saylaw < 1 || saylaw > 3) {
                    cout << "Ýalňyş saýlaw! (1-3): ";
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }

                double girizilenToleg;
                cout << "Töleg mukdary: ";
                while (!(cin >> girizilenToleg)) {
                    cout << "San giriziň: ";
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }

                HyzmatType hGornus = static_cast<HyzmatType>(saylaw);
                string transactionData = ulanyjy.getTransactionData(hGornus,girizilenToleg);

                RSAKeys keys = DigitalSignature::getKeys("keys_"+ulanyjy.ady+".txt");
                vector<long long> signature = DigitalSignature::encrypt(transactionData, keys.d, keys.n);
                string decryptedData = DigitalSignature::decrypt(signature, keys.e, keys.n);

                string sigStr = "";
                for (size_t i = 0; i < signature.size(); i++) {
                    sigStr += to_string(signature[i]);
                    if (i < signature.size() - 1) sigStr += ","; 
                }

                if (decryptedData == transactionData) {
                    cout << "[INFO]: RSA TASSYKLANDY." << endl;
                    
                    SmartContract::hyzmatyIslet(ulanyjy, girizilenToleg, hGornus);
                    telekomBC.AddBlock(Block(telekomBC.getLatestBlock().getIndex() + 1, transactionData));
                    string p2pMsg = "NEW_BLOCK|" + transactionData + "|" + sigStr + "|" + to_string(keys.e) + "|" + to_string(keys.n);
                    myNode.broadcast_message(p2pMsg);
                    cout << "[SUCCESS]: Blok torda ýaýradyldy." << endl;
                } else {
                    cout << "[ERROR]: Kriptografik barlag şowsuz!" << endl;
                }
            }
        }

    } catch (const std::exception &e) {
        cerr << "\n[CRITICAL ERROR]: " << e.what() << endl;
        return 1;
    }

    cout << "====================================================" << endl;
    cout << "                      ULGAM YAPYLDY                 " << endl;
    cout << "====================================================\n" << endl;
    return 0;
}
