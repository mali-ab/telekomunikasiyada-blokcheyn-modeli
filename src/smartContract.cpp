#include "smartContract.h"
#include "database.h"

using namespace std;

// --- Get current timestamp as string ---
string SmartContract::getCurrentTimestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct);
    return buf;
}

// --- Parse timestamp string to time_t ---
static time_t stringToTime(const string &timeStr) {
    if (timeStr == "Inactive") return 0;
    struct tm tm = {0};
    istringstream ss(timeStr);
    ss >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return mktime(&tm);
}

// --- Internet service: set speed ---
bool InternetService::setSpeed(const string &newSpeed) {
    if (newSpeed == "1" || newSpeed == "2" || newSpeed == "4" || newSpeed == "6") {
        speed = newSpeed;
        return true;
    }
    return false;
}

// --- IPTV service: set TV count ---
bool IPTVService::setTVCount(int newCount) {
    if (newCount >= 0 && newCount <= 10) {
        tvCount = newCount;
        return true;
    }
    cout << "[ERROR]: TV count must be between 0 and 10!" << endl;
    return false;
}

// --- Subscriber constructor: load from DB or create new ---
Subscriber::Subscriber(const string &subscriberName) {
    if (!SmartContract::getSubscriberFromDB(subscriberName, *this)) {
        name = subscriberName;
        balance = 0.0;
        internetExpiry = "Inactive";
        iptvExpiry = "Inactive";
        phoneExpiry = "Inactive";
    }
}

// --- Calculate remaining days until service expires ---
int Subscriber::remainingDays(const string &expiry) const {
    if (expiry == "Inactive") return 0;

    time_t activationTime = stringToTime(expiry);
    time_t now = time(0);

    double diffSeconds = difftime(now, activationTime);
    int elapsedDays = static_cast<int>(diffSeconds / (60 * 60 * 24));

    return (elapsedDays < 30) ? (30 - elapsedDays) : 0;
}

// --- Check if a specific service is currently active ---
bool Subscriber::isServiceActive(ServiceType type) const {
    switch (type) {
        case ServiceType::INTERNET: return remainingDays(internetExpiry) > 0;
        case ServiceType::IPTV:     return remainingDays(iptvExpiry) > 0;
        case ServiceType::PHONE:    return remainingDays(phoneExpiry) > 0;
        default: return false;
    }
}

// --- Get human-readable service name ---
string Subscriber::getServiceName(ServiceType type) const {
    switch (type) {
        case ServiceType::INTERNET: return "Internet " + internet.speed + " Mbit/s";
        case ServiceType::IPTV:     return "IPTV " + to_string(iptv.tvCount) + " channels";
        case ServiceType::PHONE:    return "Phone";
        default: return "N/A";
    }
}

// --- Build transaction data string for blockchain ---
string Subscriber::getTransactionData(ServiceType type, double payment) const {
    string serviceName = getServiceName(type);
    double servicePrice = 0.0;

    switch (type) {
        case ServiceType::INTERNET: servicePrice = internet.getPrice(); break;
        case ServiceType::IPTV:     servicePrice = iptv.getPrice(); break;
        case ServiceType::PHONE:    servicePrice = phone.getPrice(); break;
    }

    double newBalance = isServiceActive(type)
        ? (balance + payment)
        : (balance + payment - servicePrice);

    return "Subscriber: " + name +
           ", Service: " + serviceName +
           ", Previous balance: " + to_string(balance) + " TMT" +
           ", Payment: " + to_string(payment) + " TMT" +
           ", Price: " + to_string(servicePrice) + " TMT" +
           ", New balance: " + to_string(newBalance) + " TMT" +
           ", Timestamp: " + SmartContract::getCurrentTimestamp();
}

// --- Process a service payment via smart contract ---
bool SmartContract::processService(Subscriber &subscriber, double payment, ServiceType service) {
    double price = 0;
    string serviceName;
    string *currentExpiry = nullptr;

    switch (service) {
        case ServiceType::INTERNET:
            currentExpiry = &subscriber.internetExpiry;
            price = subscriber.internet.getPrice();
            serviceName = "Internet (" + subscriber.internet.speed + " Mbit/s)";
            break;
        case ServiceType::IPTV:
            currentExpiry = &subscriber.iptvExpiry;
            price = subscriber.iptv.getPrice();
            serviceName = "IP-TV (" + to_string(subscriber.iptv.tvCount) + " channels)";
            break;
        case ServiceType::PHONE:
            currentExpiry = &subscriber.phoneExpiry;
            price = subscriber.phone.getPrice();
            serviceName = "Phone";
            break;
    }

    cout << "\n[SMART CONTRACT]: Validating " << serviceName << "..." << endl;

    // If service is already active, just add payment to balance
    if (subscriber.remainingDays(*currentExpiry) > 0) {
        subscriber.balance += payment;
        updateSubscriberInDB(subscriber);
        cout << "[INFO]: Service already active. Payment added to balance." << endl;
        return false;
    }

    // Check if total funds are sufficient
    double total = subscriber.balance + payment;
    if (total >= price) {
        subscriber.balance = total - price;
        *currentExpiry = getCurrentTimestamp();

        cout << "[SUCCESS]: " << serviceName << " activated!" << endl;
        updateSubscriberInDB(subscriber);
        return true;
    } else {
        cout << "[ERROR]: Insufficient balance. Required: " << price << " TMT" << endl;
        return false;
    }
}

// --- Insert or update subscriber record in database ---
void SmartContract::updateSubscriberInDB(const Subscriber &subscriber) {
    try {
        pqxx::connection* C = DatabaseManager::getConnection();
        pqxx::work W(*C);

        string sql = "INSERT INTO subscribers (name, balance, internet_expiry, internet_speed, iptv_expiry, iptv_count, phone_expiry) VALUES (" +
                        W.quote(subscriber.name) + ", " +
                        to_string(subscriber.balance) + ", " +
                        W.quote(subscriber.internetExpiry) + ", " +
                        W.quote(subscriber.internet.speed) + ", " +
                        W.quote(subscriber.iptvExpiry) + ", " +
                        to_string(subscriber.iptv.tvCount) + ", " +
                        W.quote(subscriber.phoneExpiry) + ") " +
                        "ON CONFLICT (name) DO UPDATE SET " +
                        "balance = EXCLUDED.balance, " +
                        "internet_expiry = EXCLUDED.internet_expiry, " +
                        "internet_speed = EXCLUDED.internet_speed, " +
                        "iptv_expiry = EXCLUDED.iptv_expiry, " +
                        "iptv_count = EXCLUDED.iptv_count, " +
                        "phone_expiry = EXCLUDED.phone_expiry;";

        W.exec(sql);
        W.commit();
        cout << "[DB]: '" << subscriber.name << "' record updated." << endl;
    } catch (const std::exception &e) {
        cerr << "[DB UPDATE ERROR]: " << e.what() << endl;
    }
}

// --- Fetch subscriber record from database ---
bool SmartContract::getSubscriberFromDB(const string &name, Subscriber &subscriber) {
    try {
        pqxx::connection* C = DatabaseManager::getConnection();
        pqxx::nontransaction N(*C);

        string sql = "SELECT balance, internet_expiry, internet_speed, iptv_expiry, iptv_count, phone_expiry FROM subscribers WHERE name = " + N.quote(name) + ";";
        pqxx::result R = N.exec(sql);

        if (!R.empty()) {
            subscriber.name = name;
            subscriber.balance = R[0]["balance"].as<double>();
            subscriber.internetExpiry = R[0]["internet_expiry"].as<string>();
            subscriber.internet.speed = R[0]["internet_speed"].as<string>();
            subscriber.iptvExpiry = R[0]["iptv_expiry"].as<string>();
            subscriber.iptv.tvCount = R[0]["iptv_count"].as<int>();
            subscriber.phoneExpiry = R[0]["phone_expiry"].as<string>();
            return true;
        }
    } catch (const std::exception &e) {
        cerr << "[DB READ ERROR]: " << e.what() << endl;
    }
    return false;
}

// --- Full system audit: compare blockchain state vs database ---
void SmartContract::fullSystemAudit(const Blockchain& bc) {
    struct LatestState {
        double balance = 0.0;
        string internetExpiry;
        string phoneExpiry;
        string iptvExpiry;
        string serviceName;
    };
    map<string, LatestState> bcStates;

    // Replay blockchain to reconstruct latest state per subscriber
    for (const auto &block : bc.getAllBlocks()) {
        const string &data = block.sData;
        try {
            size_t namePos = data.find("Subscriber: ");
            size_t nameEnd = data.find(",", namePos);
            if (namePos == string::npos) continue;
            string subscriberName = data.substr(namePos + 12, nameEnd - (namePos + 12));

            size_t balPos = data.find("New balance: ");
            size_t tmtPos = data.find(" TMT", balPos);

            size_t timePos = data.find("Timestamp: ");
            string timestamp = data.substr(timePos + 11);

            size_t svcPos = data.find("Service: ");
            size_t svcEnd = data.find(",", svcPos);
            string service = data.substr(svcPos + 9, svcEnd - (svcPos + 9));

            if (balPos != string::npos) {
                bcStates[subscriberName].balance = stod(data.substr(balPos + 13, tmtPos - (balPos + 13)));
                bcStates[subscriberName].serviceName = service;

                if (service.find("Internet") != string::npos) {
                    bcStates[subscriberName].internetExpiry = timestamp;
                } else if (service.find("IP-TV") != string::npos) {
                    bcStates[subscriberName].iptvExpiry = timestamp;
                } else if (service.find("Phone") != string::npos) {
                    bcStates[subscriberName].phoneExpiry = timestamp;
                }
            }
        } catch (...) { continue; }
    }

    // Compare with database
    pqxx::connection* C = DatabaseManager::getConnection();
    pqxx::nontransaction N(*C);
    pqxx::result R = N.exec("SELECT name, balance, internet_expiry, iptv_expiry, phone_expiry FROM subscribers");

    cout << "\n========== [FULL SYSTEM AUDIT] ==========" << endl;

    for (const auto &row : R) {
        string subscriberName = row["name"].as<string>();
        double dbBalance = row["balance"].as<double>();
        string dbIntExpiry = row["internet_expiry"].as<string>();
        string dbIptvExpiry = row["iptv_expiry"].as<string>();
        string dbPhoneExpiry = row["phone_expiry"].as<string>();

        if (bcStates.find(subscriberName) != bcStates.end()) {
            bool errorFound = false;

            if (abs(dbBalance - bcStates[subscriberName].balance) > 0.01) {
                cout << "[CRITICAL]: " << subscriberName << " Balance TAMPERED! (DB: " << dbBalance << " / BC: " << bcStates[subscriberName].balance << ")" << endl;
                errorFound = true;
            }

            if (!bcStates[subscriberName].internetExpiry.empty() && dbIntExpiry != bcStates[subscriberName].internetExpiry) {
                cout << "[WARNING]: " << subscriberName << " Internet expiry mismatch!" << endl;
                errorFound = true;
            }

            if (!bcStates[subscriberName].iptvExpiry.empty() && dbIptvExpiry != bcStates[subscriberName].iptvExpiry) {
                cout << "[WARNING]: " << subscriberName << " IP-TV expiry mismatch!" << endl;
                errorFound = true;
            }

            if (!bcStates[subscriberName].phoneExpiry.empty() && dbPhoneExpiry != bcStates[subscriberName].phoneExpiry) {
                cout << "[WARNING]: " << subscriberName << " Phone expiry mismatch!" << endl;
                errorFound = true;
            }

            if (!errorFound) {
                cout << "[OK]: " << subscriberName << " all records verified." << endl;
            }
        } else {
            cout << "[INFO]: " << subscriberName << " has no transactions on the blockchain yet." << endl;
        }
    }
    cout << "==========================================\n" << endl;
}