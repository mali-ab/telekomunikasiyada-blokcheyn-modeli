#ifndef SMARTCONTRACT_H
#define SMARTCONTRACT_H

#include <iostream>
#include <string>

using namespace std;

struct Abonent {
    string ady;
    double balans;
    bool internetIscenmi;
    int galanGunler;
};

class SmartContract {
public:
    static void hyzmatyIslet(Abonent &abonent, double tolegMukdary);
};

#endif
