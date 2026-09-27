#include "subscriber_repository.h"

#include "database.h"

int SubscriberRepository::count() {
    pqxx::nontransaction tx(*DatabaseManager::getConnection());
    return tx.exec("SELECT COUNT(*) FROM subscribers")[0][0].as<int>();
}

std::vector<Subscriber> SubscriberRepository::list() {
    pqxx::nontransaction tx(*DatabaseManager::getConnection());
    const auto rows = tx.exec("SELECT name, balance, COALESCE(TO_CHAR(internet_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS internet_expiry, internet_speed, COALESCE(TO_CHAR(iptv_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS iptv_expiry, iptv_count, COALESCE(TO_CHAR(phone_expiry, 'YYYY-MM-DD HH24:MI:SS'), 'Inactive') AS phone_expiry FROM subscribers ORDER BY name ASC");
    std::vector<Subscriber> subscribers;
    subscribers.reserve(rows.size());
    for (const auto &row : rows) {
        Subscriber subscriber(row["name"].as<std::string>(), false);
        subscriber.balance = row["balance"].as<double>();
        subscriber.internetExpiry = row["internet_expiry"].as<std::string>();
        subscriber.internet.speed = row["internet_speed"].as<std::string>();
        subscriber.iptvExpiry = row["iptv_expiry"].as<std::string>();
        subscriber.iptv.tvCount = row["iptv_count"].as<int>();
        subscriber.phoneExpiry = row["phone_expiry"].as<std::string>();
        subscribers.push_back(std::move(subscriber));
    }
    return subscribers;
}

std::vector<std::string> SubscriberRepository::names() {
    pqxx::nontransaction tx(*DatabaseManager::getConnection());
    const auto rows = tx.exec("SELECT name FROM subscribers ORDER BY name ASC");
    std::vector<std::string> names;
    names.reserve(rows.size());
    for (const auto &row : rows) names.push_back(row["name"].as<std::string>());
    return names;
}
