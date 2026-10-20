#include "smartContract.h"
#include "database.h"

#include <cmath>
#include <cctype>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {
constexpr double kMoneyTolerance = 0.00001;
constexpr time_t kServicePeriodSeconds = 30 * 24 * 60 * 60;

bool localTimeFor(time_t value, tm &result) {
#ifdef _WIN32
    return localtime_s(&result, &value) == 0;
#else
    return localtime_r(&value, &result) != nullptr;
#endif
}

optional<time_t> stringToTime(const string &timeString) {
    if (timeString.empty() || timeString == "Inactive") return nullopt;

    tm parsed{};
    parsed.tm_isdst = -1;
    istringstream stream(timeString);
    stream >> get_time(&parsed, "%Y-%m-%d %H:%M:%S");
    if (stream.fail()) return nullopt;

    const time_t value = mktime(&parsed);
    if (value == static_cast<time_t>(-1)) return nullopt;
    return value;
}

double roundedMoney(double amount) {
    return round(amount * 100.0) / 100.0;
}

string money(double amount) {
    ostringstream output;
    output << fixed << setprecision(2) << roundedMoney(amount);
    return output.str();
}

bool hasValidSubscriberName(const string &name) {
    if (name.empty() || name.size() > 80) return false;
    for (const unsigned char character : name) {
        if (iscntrl(character) || character == ',' || character == '|') return false;
    }
    return true;
}

void validateSubscriber(const Subscriber &subscriber) {
    if (!hasValidSubscriberName(subscriber.name)) {
        throw invalid_argument("Subscriber name must be 1–80 characters and cannot contain commas or control characters");
    }
    if (!isfinite(subscriber.balance) || subscriber.balance < -kMoneyTolerance) {
        throw invalid_argument("Subscriber balance is invalid");
    }
    if (subscriber.internet.speed != "1" && subscriber.internet.speed != "2" &&
        subscriber.internet.speed != "4" && subscriber.internet.speed != "6") {
        throw invalid_argument("Internet speed is invalid");
    }
    if (subscriber.iptv.tvCount < 0 || subscriber.iptv.tvCount > 10) {
        throw invalid_argument("IPTV channel count must be between 0 and 10");
    }
}

void setError(string *errorOut, const string &message) {
    if (errorOut) *errorOut = message;
}

const string *expiryFor(const Subscriber &subscriber, ServiceType type) {
    switch (type) {
        case ServiceType::INTERNET: return &subscriber.internetExpiry;
        case ServiceType::IPTV: return &subscriber.iptvExpiry;
        case ServiceType::PHONE: return &subscriber.phoneExpiry;
    }
    return nullptr;
}

optional<string> eventField(const string &data, const string &name) {
    const string prefix = name + ": ";
    const size_t position = data.find(prefix);
    if (position == string::npos) return nullopt;
    const size_t start = position + prefix.size();
    const size_t end = data.find(", ", start);
    return data.substr(start, end == string::npos ? string::npos : end - start);
}

optional<double> eventMoney(const string &data, const string &name) {
    const auto value = eventField(data, name);
    if (!value) return nullopt;
    const size_t currency = value->find(" TMT");
    try {
        return stod(value->substr(0, currency));
    } catch (...) {
        return nullopt;
    }
}
}

bool InternetService::setSpeed(const string &newSpeed) {
    if (newSpeed == "1" || newSpeed == "2" || newSpeed == "4" || newSpeed == "6") {
        speed = newSpeed;
        return true;
    }
    return false;
}

bool IPTVService::setTVCount(int newCount) {
    if (newCount >= 0 && newCount <= 10) {
        tvCount = newCount;
        return true;
    }
    cout << "[ÝALŇYŞLYK]: TV sany 0 we 10 aralygynda bolmaly!" << endl;
    return false;
}

Subscriber::Subscriber(const string &subscriberName, bool loadFromDatabase) {
    if (loadFromDatabase && SmartContract::getSubscriberFromDB(subscriberName, *this)) return;

    name = subscriberName;
    balance = 0.0;
    internetExpiry = "Inactive";
    iptvExpiry = "Inactive";
    phoneExpiry = "Inactive";
}

int Subscriber::remainingDays(const string &expiry) const {
    const auto expiryTime = stringToTime(expiry);
    if (!expiryTime) return 0;

    const double secondsRemaining = difftime(*expiryTime, time(nullptr));
    if (secondsRemaining <= 0) return 0;
    return static_cast<int>(ceil(secondsRemaining / (24.0 * 60.0 * 60.0)));
}

bool Subscriber::isServiceActive(ServiceType type) const {
    const string *expiry = expiryFor(*this, type);
    return expiry && remainingDays(*expiry) > 0;
}

string Subscriber::getServiceName(ServiceType type) const {
    switch (type) {
        case ServiceType::INTERNET: return "Internet " + internet.speed + " Mbit/s";
        case ServiceType::IPTV: return "IPTV " + to_string(iptv.tvCount) + " channels";
        case ServiceType::PHONE: return "Phone";
    }
    return "Unknown";
}

string SmartContract::formatTimestamp(time_t timestamp) {
    tm local{};
    if (!localTimeFor(timestamp, local)) throw runtime_error("Could not format service expiry");
    char buffer[20];
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local) == 0) {
        throw runtime_error("Could not format service expiry");
    }
    return buffer;
}

string SmartContract::getCurrentTimestamp() {
    return formatTimestamp(time(nullptr));
}

bool SmartContract::processService(Subscriber &subscriber, double payment, ServiceType service,
                                   string *errorOut, time_t processedAt) {
    if (!hasValidSubscriberName(subscriber.name)) {
        setError(errorOut, "Subscriber name is invalid.");
        return false;
    }
    if (!isfinite(payment) || payment <= 0.0) {
        setError(errorOut, "Payment must be a positive amount.");
        return false;
    }
    if (processedAt <= 0) processedAt = time(nullptr);

    double price = 0.0;
    string *currentExpiry = nullptr;
    string serviceName;
    switch (service) {
        case ServiceType::INTERNET:
            price = subscriber.internet.getPrice();
            currentExpiry = &subscriber.internetExpiry;
            serviceName = "Internet (" + subscriber.internet.speed + " Mbit/s)";
            break;
        case ServiceType::IPTV:
            price = subscriber.iptv.getPrice();
            currentExpiry = &subscriber.iptvExpiry;
            serviceName = "IPTV (" + to_string(subscriber.iptv.tvCount) + " channels)";
            break;
        case ServiceType::PHONE:
            price = subscriber.phone.getPrice();
            currentExpiry = &subscriber.phoneExpiry;
            serviceName = "Phone";
            break;
        default:
            setError(errorOut, "Unknown service type.");
            return false;
    }
    if (price <= 0.0) {
        setError(errorOut, "Choose a valid service plan before accepting payment.");
        return false;
    }

    const double total = roundedMoney(subscriber.balance + payment);
    if (total + kMoneyTolerance < price) {
        setError(errorOut, "Payment plus available balance is below the service price.");
        return false;
    }

    time_t renewalBase = processedAt;
    if (const auto current = stringToTime(*currentExpiry); current && *current > renewalBase) {
        renewalBase = *current;
    }
    if (renewalBase > numeric_limits<time_t>::max() - kServicePeriodSeconds) {
        setError(errorOut, "Service expiry is outside the supported date range.");
        return false;
    }

    subscriber.balance = roundedMoney(total - price);
    *currentExpiry = formatTimestamp(renewalBase + kServicePeriodSeconds);
    cout << "[ÜSTÜNLIK]: " << serviceName << " 30 gün uzaldyldy." << endl;
    return true;
}

string SmartContract::getPaymentData(const Subscriber &before, const Subscriber &after,
                                     ServiceType service, double payment, time_t processedAt) {
    const string *expiry = expiryFor(after, service);
    return "Event: Payment, Subscriber: " + after.name +
           ", Service: " + after.getServiceName(service) +
           ", Previous balance: " + money(before.balance) + " TMT" +
           ", Payment: " + money(payment) + " TMT" +
           ", Price: " + money(service == ServiceType::INTERNET ? after.internet.getPrice() :
                                service == ServiceType::IPTV ? after.iptv.getPrice() : after.phone.getPrice()) + " TMT" +
           ", New balance: " + money(after.balance) + " TMT" +
           ", Service expiry: " + (expiry ? *expiry : "Inactive") +
           ", Timestamp: " + formatTimestamp(processedAt);
}

string SmartContract::getPlanChangeData(const Subscriber &before, const Subscriber &after) {
    return "Event: Plan change, Subscriber: " + after.name +
           ", Service: Plan update" +
           ", Internet speed: " + before.internet.speed + " -> " + after.internet.speed + " Mbit/s" +
           ", IPTV channels: " + to_string(before.iptv.tvCount) + " -> " + to_string(after.iptv.tvCount) +
           ", Previous balance: " + money(before.balance) + " TMT" +
           ", Payment: 0.00 TMT, Price: 0.00 TMT, New balance: " + money(after.balance) +
           " TMT, Timestamp: " + getCurrentTimestamp();
}

void SmartContract::updateSubscriberInDB(const Subscriber &subscriber, pqxx::work &transaction) {
    validateSubscriber(subscriber);
    const string sql = "INSERT INTO subscribers "
                       "(name, balance, internet_expiry, internet_speed, iptv_expiry, iptv_count, phone_expiry) VALUES (" +
                       transaction.quote(subscriber.name) + ", " +
                       money(subscriber.balance) + ", " +
                       "NULLIF(" + transaction.quote(subscriber.internetExpiry) + ", 'Inactive')::TIMESTAMPTZ, " +
                       transaction.quote(subscriber.internet.speed) + ", " +
                       "NULLIF(" + transaction.quote(subscriber.iptvExpiry) + ", 'Inactive')::TIMESTAMPTZ, " +
                       to_string(subscriber.iptv.tvCount) + ", " +
                       "NULLIF(" + transaction.quote(subscriber.phoneExpiry) + ", 'Inactive')::TIMESTAMPTZ) " +
                       "ON CONFLICT (name) DO UPDATE SET "
                       "balance = EXCLUDED.balance, internet_expiry = EXCLUDED.internet_expiry, "
                       "internet_speed = EXCLUDED.internet_speed, iptv_expiry = EXCLUDED.iptv_expiry, "
                       "iptv_count = EXCLUDED.iptv_count, phone_expiry = EXCLUDED.phone_expiry;";
    transaction.exec(sql);
}

void SmartContract::updateSubscriberInDB(const Subscriber &subscriber) {
    pqxx::work transaction(*DatabaseManager::getConnection());
    updateSubscriberInDB(subscriber, transaction);
    transaction.commit();
    cout << "[MB]: '" << subscriber.name << "' ýazgysy täzelendi." << endl;
}

bool SmartContract::createSubscriberInDB(const Subscriber &subscriber) {
    validateSubscriber(subscriber);
    pqxx::work transaction(*DatabaseManager::getConnection());
    const string sql = "INSERT INTO subscribers "
                       "(name, balance, internet_expiry, internet_speed, iptv_expiry, iptv_count, phone_expiry) VALUES (" +
                       transaction.quote(subscriber.name) + ", " + money(subscriber.balance) + ", " +
                       "NULL, " + transaction.quote(subscriber.internet.speed) + ", NULL, " +
                       to_string(subscriber.iptv.tvCount) + ", NULL) "
                       "ON CONFLICT (name) DO NOTHING RETURNING name;";
    const bool created = !transaction.exec(sql).empty();
    transaction.commit();
    return created;
}

bool SmartContract::getSubscriberFromDB(const string &name, Subscriber &subscriber) {
    pqxx::nontransaction transaction(*DatabaseManager::getConnection());
    const string sql = "SELECT COALESCE(balance, 0) AS balance, "
                       "COALESCE(TO_CHAR(internet_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS internet_expiry, "
                       "COALESCE(internet_speed, '1') AS internet_speed, "
                       "COALESCE(TO_CHAR(iptv_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS iptv_expiry, "
                       "COALESCE(iptv_count, 0) AS iptv_count, "
                       "COALESCE(TO_CHAR(phone_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS phone_expiry "
                       "FROM subscribers WHERE name = " + transaction.quote(name) + ";";
    const pqxx::result rows = transaction.exec(sql);
    if (rows.empty()) return false;

    subscriber.name = name;
    subscriber.balance = rows[0]["balance"].as<double>();
    subscriber.internetExpiry = rows[0]["internet_expiry"].as<string>();
    subscriber.internet.speed = rows[0]["internet_speed"].as<string>();
    subscriber.iptvExpiry = rows[0]["iptv_expiry"].as<string>();
    subscriber.iptv.tvCount = rows[0]["iptv_count"].as<int>();
    subscriber.phoneExpiry = rows[0]["phone_expiry"].as<string>();
    return true;
}

void SmartContract::fullSystemAudit(const Blockchain &bc) {
    if (!bc.fullAudit()) {
        cout << "[KRITIKI]: Maglumat bazasy bilen deňeşdirme geçirilmän saklandy." << endl;
        return;
    }

    struct LatestState {
        optional<double> balance;
        optional<string> internetExpiry;
        optional<string> iptvExpiry;
        optional<string> phoneExpiry;
    };
    map<string, LatestState> ledgerStates;

    for (const auto &block : bc.getAllBlocks()) {
        const auto subscriberName = eventField(block.sData, "Subscriber");
        const auto newBalance = eventMoney(block.sData, "New balance");
        if (!subscriberName || !newBalance) continue;

        auto &state = ledgerStates[*subscriberName];
        state.balance = *newBalance;
        const auto service = eventField(block.sData, "Service");
        const auto expiry = eventField(block.sData, "Service expiry");
        if (!service || !expiry) continue; // Legacy events did not retain expiry snapshots.
        if (service->find("Internet") != string::npos) state.internetExpiry = *expiry;
        else if (service->find("IPTV") != string::npos || service->find("IP-TV") != string::npos) state.iptvExpiry = *expiry;
        else if (service->find("Phone") != string::npos) state.phoneExpiry = *expiry;
    }

    pqxx::nontransaction transaction(*DatabaseManager::getConnection());
    const pqxx::result rows = transaction.exec(
        "SELECT name, COALESCE(balance, 0) AS balance, "
        "COALESCE(TO_CHAR(internet_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS internet_expiry, "
        "COALESCE(TO_CHAR(iptv_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS iptv_expiry, "
        "COALESCE(TO_CHAR(phone_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS phone_expiry FROM subscribers");

    cout << "\n========== [DOLY ULGAM AUDITY] ==========" << endl;
    for (const auto &row : rows) {
        const string subscriberName = row["name"].as<string>();
        const auto found = ledgerStates.find(subscriberName);
        if (found == ledgerStates.end()) {
            cout << "[MAGLUMAT]: " << subscriberName << " üçin heniz blokçeýnde tranzaksiýa ýok." << endl;
            continue;
        }

        bool mismatch = false;
        const LatestState &state = found->second;
        const double dbBalance = row["balance"].as<double>();
        if (state.balance && abs(dbBalance - *state.balance) > 0.01) {
            cout << "[KRITIKI]: " << subscriberName << " balansy gabat gelmeýär (MB: "
                 << dbBalance << " / BÇ: " << *state.balance << ")" << endl;
            mismatch = true;
        }
        const auto compareExpiry = [&](const optional<string> &expected, const char *column, const char *label) {
            if (expected && row[column].as<string>() != *expected) {
                cout << "[DUÝDURYŞ]: " << subscriberName << " " << label << " möhleti gabat gelmeýär!" << endl;
                mismatch = true;
            }
        };
        compareExpiry(state.internetExpiry, "internet_expiry", "Internet");
        compareExpiry(state.iptvExpiry, "iptv_expiry", "IPTV");
        compareExpiry(state.phoneExpiry, "phone_expiry", "Telefon");
        if (!mismatch) cout << "[OK]: " << subscriberName << " ähli ýazgylar tassyklandy." << endl;
    }
    cout << "==========================================\n" << endl;
}
