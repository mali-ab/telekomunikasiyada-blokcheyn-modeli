# 🌐 Telekomunikasiýada Blokçeýn Modeli (Turkmentelekom)

Bu taslama **"Türkmentelekom" elektrik aragatnaşyk kompaniýasynyň** mysalynda, telekommunikasiýa hyzmatlarynda maglumat howpsuzlygyny üpjün etmek we amallary awtomatlaşdyrmak üçin blokçeýn tehnologiýasyny ulanmagyň amaly modelidir.

## 📌 Taslamanyň Maksady

Häzirki wagtda ulanylýan merkezleşdirilen ulgamlaryň gowşak taraplaryny (ýeke-täk şowsuzlyk nokady, maglumatlaryň manipulýasiýasy) aradan aýyrmak we abonent maglumatlarynyň bitinligini kriptografik usullar arkaly goramak.

---

## 🛠 Tehniki Binýat

Model **C++** dilinde işlenip taýýarlanyldy. Saýlanyp alnan tehnologiýalar aşakdaky artykmaçlyklary üpjün edýär:
* **Ýokary tizlik:** Maglumatlary real wagtda işlemek.
* **Low-level dolandyryş:** Resurslary we ýady netijeli ulanmak.
* **Kriptografik bitinlik:** OpenSSL kitaphanasy arkaly ýokary derejeli gorag.

### Esasy Komponentler
1.  **Block Moduly:** Maglumatlary, wagt belgisini (timestamp) we öňki bloguň heş-koduny (hash) saklaýar.
2.  **SHA-256 Algoritmi:** Maglumatlaryň üýtgedilmezligini üpjün edýän esasy kriptografik gural.
3.  **Akylly Şertnamalar (Smart Contracts):** Tölegleri we hyzmatlary adam gatnaşygyndan daşary awtomatlaşdyrmak.

---

## 📊 Ykdysady we Tehniki Netijelilik

| Görkeziji | Merkezleşdirilen Ulgam | Blokçeýn Modeli |
| :--- | :--- | :--- |
| **Maglumat saklanyşy** | Ýeke-täk merkezi serwer | Paýlanan reýestr (DLT) |
| **Töleg tassyklanyşy** | El bilen (Operator) | Awtomatiki (Smart Contract) |
| **Durnuklylyk** | Pes (Single point of failure) | Örän ýokary (Decentralized) |
| **Admin Harajatlary** | Standart | **30-40% tygşytlylyk** |

### Howpsuzlyk Formulasy
Ulgamyň haker hüjümlerine durnuklylygy $51\%$ hüjümine garşy ähtimallyklar nazaryýeti bilen hasaplanýar:

$$P = \sum_{k=\lceil n/2 \rceil}^{n} \binom{n}{k} p^k (1-p)^{n-k}$$

*Bellik: Düwünleriň sany ($n$) artdygyça, ulgamyň döwülme ähtimallygy nola ýakynlaşýar.*

---

## 📂 Faýllaryň Gurluşy (File Structure)

Taslama modully arhitekturada gurlan bolup, her bir faýl aýratyn gatlaga jogap berýär:

* **`blockchain.cpp / .h`**: **Maglumat gatlagy.** Bloklary döretmek we zynjyryň bitinligini saklamak üçin jogapkär.
* **`signature.cpp / .h`**: **Howpsuzlyk gatlagy.** Abonentleriň şahsy we köpçülikleýin açarlary arkaly sanly gol çekmek mehanizmini amala aşyrýar.
* **`smartContract.cpp / .h`**: **Amallar gatlagy.** Hyzmatlaryň (internet, töleg) awtomatiki işjeňleşmegini dolandyrýar.
* **`main.cpp`**: Ähli modullary birleşdirýän we ulgamyň işleyiş siklini görkezýän esasy faýl.

---

## 🚀 Gurnama we Işletmek

### Gerekli kitaphanalar
Kody işletmek üçin ulgamyňyzda **OpenSSL** kitaphanasynyň gurnalan bolmagy gerek.

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install libssl-dev
```

### Kompilýasiýa we Işletmek
Taslamany awtomatiki birleşdirmek we işletmek üçin:

```bash
# Programmany gurmak
make

# Programmany işletmek
./telekom_system
```

---

## 📈 Geljekdäki Ösüşler
* **P2P Tor:** `boost::asio` ulanyp, düwünleriň arasynda hakyky wagtly maglumat alyş-çalyşygyny goşmak.
* **Maglumat Bazasy:** Bloklary hemişelik saklamak üçin `LevelDB` ýa-da `SQLite` integrasiýasy.
* **Sanly Gol:** ECDSA algoritmi arkaly abonentleriň tranzaksiýalaryna has çylşyrymly gol çekmek ulgamyny kämilleşdirmek.