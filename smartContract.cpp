#include "smartContract.h"

void SmartContract::hyzmatyIslet(Abonent &abonent, double tolegMukdary) {
    double internetBahasy = 50.0; // 1 aýlyk internet bahasy

    cout << "\n--- Akylly sertnama amala asyrylyar ---" << endl;
    cout << "Gozegcilikdäki abonent: " << abonent.ady << endl;
    
    if (tolegMukdary >= internetBahasy) {
        abonent.balans += tolegMukdary;
        abonent.internetIscenmi = true;
        abonent.galanGunler = 30;
        cout << "Netije: Toleg kabul edildi. Internet 30 gunluk isjenlesdirildi!" << endl;
    } else {
        cout << "Netije: Toleg ýetersiz! Minimal bahasy: " << internetBahasy << " TMT" << endl;
    }
    cout << "--------------------------------------\n" << endl;
}
