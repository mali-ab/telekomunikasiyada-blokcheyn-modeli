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
    map<string, double> realBalances;

    for (auto& block : bc.getAllBlocks()) {
        string data = block.sData;

        size_t namePos = data.find("Abonent: ");
        size_t commaPos = data.find(",", namePos);
        
        if (namePos != string::npos && commaPos != string::npos) {
            string ady = data.substr(namePos + 9, commaPos - (namePos + 9));

            size_t payPos = data.find("Toleg: ");
            size_t tmtPos1 = data.find(" TMT", payPos);

            size_t hyzBahPos = data.find("Hyzmatyn bahasy: ");
            size_t tmtPos2 = data.find(" TMT", hyzBahPos);

            if (payPos != string::npos && tmtPos1 != string::npos && 
                hyzBahPos != string::npos && tmtPos2 != string::npos) {
                
                try {
                    string tolegStr = data.substr(payPos + 7, tmtPos1 - (payPos + 7));
                    string hyzBahaStr = data.substr(hyzBahPos + 17, tmtPos2 - (hyzBahPos + 17));
                    
                    double tolegJemi = stod(tolegStr);
                    double hyzmatBaha = stod(hyzBahaStr);
                    
                    realBalances[ady] += (tolegJemi - hyzmatBaha); 
                    
                } catch (...) {
                    continue;
                }
            }
        }
    }

    pqxx::connection* C = DatabaseManager::getConnection();
    pqxx::nontransaction N(*C);
    pqxx::result R = N.exec("SELECT ady, balans FROM abonents");
    for (auto row : R) {
        string ady = row["ady"].as<string>();
        double dbBalans = row["balans"].as<double>();
        
        if (dbBalans != realBalances[ady]) {
            cout << "[CRITICAL]: " << ady << " balansy galp! Bazada: " 
                 << dbBalans << ", Blokçeýnde: " << realBalances[ady] << endl;
        }
    }
    cout << "[AUDIT COMPLETE]: Ähli abonentleriň balanslary barlandy." << endl;
}