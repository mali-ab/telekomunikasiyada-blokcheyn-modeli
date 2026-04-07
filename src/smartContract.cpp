#include "smartContract.h"
#include "database.h"

string SmartContract::häzirkiWagtyAl() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct);
    return buf;
}

time_t stringToTime(string wagt) {
    if (wagt == "Işjeň däl") return 0;
    struct tm tm = {0};
    std::istringstream ss(wagt);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return mktime(&tm);
}

bool InternetHyzmaty::setTizlik(const string &newTizlik) {
    if (newTizlik == "1" || newTizlik == "2" || newTizlik == "4" || newTizlik == "6") {
        tizlik = newTizlik;
        return true;
    }
    return false;
}

bool IPTVHyzmaty::setTVsany(int newSany) {
    if (newSany >= 0 && newSany <= 10) {
        tvSany = newSany;
        return true;
    }
    cout << "[ERROR]: TV sany 0-dan 10-ga çenli bolmaly!" << endl;
    return false;
}

Abonent::Abonent(string adyData) {
    if (!SmartContract::getAbonentFromDB(adyData, *this)) {
        ady = adyData;
        balans = 0.0;
        internetWagty = "Işjeň däl";
        iptvWagty = "Işjeň däl";
        telefonWagty = "Işjeň däl";
    }
}

int Abonent::galanGun(const string &wagt) const {
    if (wagt == "Işjeň däl") return 0;
    
    time_t acylanWagt = stringToTime(wagt);
    time_t häzir = time(0);
    
    double diffSeconds = difftime(häzir, acylanWagt);
    int gecenGunler = static_cast<int>(diffSeconds / (60 * 60 * 24));
    
    return (gecenGunler < 30) ? (30 - gecenGunler) : 0;
}

bool Abonent::hyzmatIşjeňmi(HyzmatType h) const {
    switch (h) {
        case HyzmatType::INTERNET: return galanGun(internetWagty) > 0;
        case HyzmatType::IPTV: return galanGun(iptvWagty) > 0;
        case HyzmatType::TELEFON: return galanGun(telefonWagty) > 0;
        default: return false;
    }
}

string Abonent::getHyzmatynAdy(HyzmatType h) const {
    switch (h) {
        case HyzmatType::INTERNET: return "Internet " + internet.tizlik + " Mbit/s";
        case HyzmatType::IPTV: return "IPTV " + to_string(iptv.tvSany) + " sany";
        case HyzmatType::TELEFON: return "Telefon";
        default: return "N/A";
    }
}

string Abonent::getTransactionData(HyzmatType h, double toleg) const {
    string hyzAdy = getHyzmatynAdy(h);
    double hyzBaha = 0.0;

    if (h == HyzmatType::INTERNET) hyzBaha = internet.getBaha();
    else if (h == HyzmatType::IPTV) hyzBaha = iptv.getBaha();
    else if (h == HyzmatType::TELEFON) hyzBaha = telefon.getBaha();

    double tazeBalans = hyzmatIşjeňmi(h) ? (balans + toleg) : (balans + toleg - hyzBaha);

    return "Abonent: " + ady + 
           ", Hyzmat: " + hyzAdy + 
           ", Onki balans: " + to_string(balans) + " TMT" +
           ", Toleg: " + to_string(toleg) + " TMT" +
           ", Baha: " + to_string(hyzBaha) + " TMT" +
           ", Taze balans: " + to_string(tazeBalans) + " TMT" +
           ", Wagty: " + SmartContract::häzirkiWagtyAl();
}

bool SmartContract::hyzmatyIslet(Abonent &abonent, double toleg, HyzmatType hyzmat) {
    double baha = 0;
    string hyzmatAdy = "";
    string *currentWagt = nullptr;

    switch (hyzmat) {
        case HyzmatType::INTERNET:
            currentWagt = &abonent.internetWagty; 
            baha = abonent.internet.getBaha();
            hyzmatAdy = "Internet (" + abonent.internet.tizlik + " Mbit/s)";
            break;
        case HyzmatType::IPTV:     
            currentWagt = &abonent.iptvWagty; 
            baha = abonent.iptv.getBaha();
            hyzmatAdy = "IP-TV (" + to_string(abonent.iptv.tvSany) + " sany)";
            break;
        case HyzmatType::TELEFON:  
            currentWagt = &abonent.telefonWagty; 
            baha = abonent.telefon.getBaha();
            hyzmatAdy = "Telefon";  
            break;
    }

    cout << "\n[AKYLLY SERTNAMA]: " << hyzmatAdy << " barlanýar..." << endl;

    if (abonent.galanGun(*currentWagt) > 0) {
        abonent.balans += toleg;
        updateAbonentInDB(abonent);
        cout << "[INFO]: Hyzmat eýýäm işjeň. Töleg balansa goşuldy." << endl;
        return false;
    }

    double jemi = abonent.balans + toleg;
    if (jemi >= baha) {
        abonent.balans = jemi - baha;
        *currentWagt = häzirkiWagtyAl();
        
        cout << "[SUCCESS]: " << hyzmatAdy << " aktiwleşdirildi!" << endl;
        updateAbonentInDB(abonent);
        return true;
    } else {
        cout << "[ERROR]: Balans ýetersiz. Gerekli: " << baha << " TMT" << endl;
        return false;
    }
}

void SmartContract::updateAbonentInDB(const Abonent &abonent) {
    try {
        pqxx::connection* C = DatabaseManager::getConnection();
        pqxx::work W(*C);
        
        string sql = "INSERT INTO abonents (ady, balans, internet_wagt, internet_tizlik, iptv_wagt, iptv_sany, telefon_wagt) VALUES (" +
                        W.quote(abonent.ady) + ", " + 
                        to_string(abonent.balans) + ", " +
                        W.quote(abonent.internetWagty) + ", " +
                        W.quote(abonent.internet.tizlik) + ", " +
                        W.quote(abonent.iptvWagty) + ", " +
                        to_string(abonent.iptv.tvSany) + ", " +
                        W.quote(abonent.telefonWagty) + ") " +
                        "ON CONFLICT (ady) DO UPDATE SET " +
                        "balans = EXCLUDED.balans, " +
                        "internet_wagt = EXCLUDED.internet_wagt, " +
                        "internet_tizlik = EXCLUDED.internet_tizlik, " +
                        "iptv_wagt = EXCLUDED.iptv_wagt, " +
                        "iptv_sany = EXCLUDED.iptv_sany, " +
                        "telefon_wagt = EXCLUDED.telefon_wagt;";
        
        W.exec(sql);
        W.commit();
        cout << "[DB]: '" << abonent.ady << "' maglumatlary täzelendi." << endl;
    } catch (const std::exception &e) {
        cerr << "[DB UPDATE ERROR]: " << e.what() << endl;
    }
}

bool SmartContract::getAbonentFromDB(string ady, Abonent &abonent) {
    try {
        pqxx::connection* C = DatabaseManager::getConnection();
        pqxx::nontransaction N(*C);
        
        string sql = "SELECT balans, internet_wagt, internet_tizlik, iptv_wagt, iptv_sany, telefon_wagt FROM abonents WHERE ady = " + N.quote(ady) + ";";
        pqxx::result R = N.exec(sql);

        if (!R.empty()) {
            abonent.ady = ady;
            abonent.balans = R[0]["balans"].as<double>();
            abonent.internetWagty = R[0]["internet_wagt"].as<string>();
            abonent.internet.tizlik = R[0]["internet_tizlik"].as<string>();
            abonent.iptvWagty = R[0]["iptv_wagt"].as<string>();
            abonent.iptv.tvSany = R[0]["iptv_sany"].as<int>();
            abonent.telefonWagty = R[0]["telefon_wagt"].as<string>();
            return true; 
        }
    } catch (const std::exception &e) {
        cerr << "[DB READ ERROR]: " << e.what() << endl;
    }
    return false; 
}

void SmartContract::fullSystemAudit(const Blockchain& bc) {
    struct LatestState {
        double balans = 0.0;
        string internetWagt = "";
        string telefonWagt = "";
        string iptvWagt = "";
        string hyzmatGornusi = "";
    };
    map<string, LatestState> bcStates;

    for (auto& block : bc.getAllBlocks()) {
        string data = block.sData;
        try {
            size_t namePos = data.find("Abonent: ");
            size_t nameEnd = data.find(",", namePos);
            if (namePos == string::npos) continue;
            string ady = data.substr(namePos + 9, nameEnd - (namePos + 9));

            size_t balPos = data.find("Taze balans: ");
            size_t tmtPos = data.find(" TMT", balPos);
            
            size_t wagtPos = data.find("Wagty: ");
            string wagt = data.substr(wagtPos + 7);

            size_t hyzPos = data.find("Hyzmat: ");
            size_t hyzEnd = data.find(",", hyzPos);
            string hyzmat = data.substr(hyzPos + 8, hyzEnd - (hyzPos + 8));

            if (balPos != string::npos) {
                bcStates[ady].balans = stod(data.substr(balPos + 13, tmtPos - (balPos + 13)));
                bcStates[ady].hyzmatGornusi = hyzmat;
                
                if (hyzmat.find("Internet") != string::npos) {
                    bcStates[ady].internetWagt = wagt;
                } else if (hyzmat.find("IP-TV") != string::npos) {
                    bcStates[ady].iptvWagt = wagt;
                } else if (hyzmat.find("Telefon") != string::npos) {
                    bcStates[ady].telefonWagt = wagt;
                }
            }
        } catch (...) { continue; }
    }

    pqxx::connection* C = DatabaseManager::getConnection();
    pqxx::nontransaction N(*C);
    pqxx::result R = N.exec("SELECT ady, balans, internet_wagt, iptv_wagt, telefon_wagt FROM abonents");

    cout << "\n========== [ULGAMIŇ DOLY AUDITI] ==========" << endl;

    for (auto row : R) {
        string ady = row["ady"].as<string>();
        double dbBalans = row["balans"].as<double>();
        string dbIntWagt = row["internet_wagt"].as<string>();
        string dbIptvWagt = row["iptv_wagt"].as<string>();
        string dbTelWagt = row["telefon_wagt"].as<string>();

        if (bcStates.find(ady) != bcStates.end()) {
            bool errorFound = false;

            if (abs(dbBalans - bcStates[ady].balans) > 0.01) {
                cout << "[CRITICAL]: " << ady << " Balansy GALP! (DB: " << dbBalans << " / BC: " << bcStates[ady].balans << ")" << endl;
                errorFound = true;
            }

            if (!bcStates[ady].internetWagt.empty() && dbIntWagt != bcStates[ady].internetWagt) {
                cout << "[WARNING]: " << ady << " Internet möhleti gabat gelmeýär!" << endl;
                errorFound = true;
            }

            if (!bcStates[ady].iptvWagt.empty() && dbIptvWagt != bcStates[ady].iptvWagt) {
                cout << "[WARNING]: " << ady << " IP-TV möhleti gabat gelmeýär!" << endl;
                errorFound = true;
            }

            if (!bcStates[ady].telefonWagt.empty() && dbTelWagt != bcStates[ady].telefonWagt) {
                cout << "[WARNING]: " << ady << " Telefon möhleti gabat gelmeýär!" << endl;
                errorFound = true;
            }

            if (!errorFound) {
                cout << "[OK]: " << ady << " ähli maglumatlary dogry." << endl;
            }
        } else {
            cout << "[INFO]: " << ady << " barada blokçeýnde heniz tranzaksiýa ýok." << endl;
        }
    }
    cout << "===========================================\n" << endl;
}