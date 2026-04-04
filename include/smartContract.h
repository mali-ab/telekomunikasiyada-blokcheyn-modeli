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

using namespace std;

enum class HyzmatType { INTERNET = 1, IPTV = 2, TELEFON = 3 };

class HyzmatBase {
public:
    virtual double getBaha() const = 0;
    virtual string getStatus() const = 0;
    virtual ~HyzmatBase() {}
};

class InternetHyzmaty : public HyzmatBase {
public:
    string tizlik;
    InternetHyzmaty(string t = "1") : tizlik(t) {}

    bool setTizlik(const string &newTizlik);
    
    double getBaha() const override {
        if (tizlik == "1") return 150.0;
        if (tizlik == "2") return 180.0;
        if (tizlik == "4") return 230.0;
        if (tizlik == "6") return 280.0;
        return 0.0;
    }
    string getStatus() const override { return tizlik + " Mbit/s"; }
};

class IPTVHyzmaty : public HyzmatBase {
public:
    int tvSany;
    IPTVHyzmaty(int s = 1) : tvSany(s) {}
    
    bool setTVsany(int newTVsany);
    
    double getBaha() const override { return tvSany ? 10 + tvSany - 1 : 0; }
    string getStatus() const override { return to_string(tvSany) + " TV"; }
};

class TelefonHyzmaty : public HyzmatBase {
public:
    double getBaha() const override { return 10.0; }
    string getStatus() const override { return "Aktiw"; }
};

struct Abonent {
    string ady;
    double balans;
    
    InternetHyzmaty internet;
    IPTVHyzmaty iptv;
    TelefonHyzmaty telefon;

    string internetWagty;
    string iptvWagty;
    string telefonWagty;

    Abonent(string adyData);

    int galanGun(const string& wagt) const;
    string getHyzmatynAdy(HyzmatType h) const;
    string getTransactionData(HyzmatType h, double toleg) const;
};

class SmartContract {
public:
    static bool hyzmatyIslet(Abonent &abonent, double toleg, HyzmatType hyzmat);

    static void updateAbonentInDB(const Abonent &abonent);
    static bool getAbonentFromDB(const string ady, Abonent &abonent);
    static void fullSystemAudit(const Blockchain &bc);
    static string häzirkiWagtyAl();
};

#endif