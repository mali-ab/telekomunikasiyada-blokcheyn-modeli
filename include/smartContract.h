#ifndef SMARTCONTRACT_H
#define SMARTCONTRACT_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <iomanip>
#include <map>
#include "blockchain.h"

namespace pqxx { class connection; }

// --- Service type enumeration ---
enum class ServiceType { INTERNET = 1, IPTV = 2, PHONE = 3 };

// --- Abstract base class for all services ---
class ServiceBase {
public:
    virtual double getPrice() const = 0;
    virtual std::string getStatus() const = 0;
    virtual ~ServiceBase() {}
};

// --- Internet service ---
class InternetService : public ServiceBase {
public:
    std::string speed;
    InternetService(std::string s = "1") : speed(std::move(s)) {}

    bool setSpeed(const std::string &newSpeed);

    double getPrice() const override {
        if (speed == "1") return 150.0;
        if (speed == "2") return 180.0;
        if (speed == "4") return 230.0;
        if (speed == "6") return 280.0;
        return 0.0;
    }
    std::string getStatus() const override { return speed + " Mbit/s"; }
};

// --- IPTV service ---
class IPTVService : public ServiceBase {
public:
    int tvCount;
    IPTVService(int count = 1) : tvCount(count) {}

    bool setTVCount(int newCount);

    double getPrice() const override { return tvCount ? 10 + tvCount - 1 : 0; }
    std::string getStatus() const override { return std::to_string(tvCount) + " TV"; }
};

// --- Phone service ---
class PhoneService : public ServiceBase {
public:
    double getPrice() const override { return 10.0; }
    std::string getStatus() const override { return "Active"; }
};

// --- Subscriber data structure ---
struct Subscriber {
    std::string name;
    double balance;

    InternetService internet;
    IPTVService iptv;
    PhoneService phone;

    std::string internetExpiry;
    std::string iptvExpiry;
    std::string phoneExpiry;

    Subscriber(const std::string &subscriberName);

    int remainingDays(const std::string &expiry) const;
    bool isServiceActive(ServiceType type) const;
    std::string getServiceName(ServiceType type) const;
    std::string getTransactionData(ServiceType type, double payment) const;
};

// --- Smart contract logic ---
class SmartContract {
public:
    static bool processService(Subscriber &subscriber, double payment, ServiceType service);

    static void updateSubscriberInDB(const Subscriber &subscriber);
    static bool getSubscriberFromDB(const std::string &name, Subscriber &subscriber);
    static void fullSystemAudit(const Blockchain &bc);
    static std::string getCurrentTimestamp();
};

#endif