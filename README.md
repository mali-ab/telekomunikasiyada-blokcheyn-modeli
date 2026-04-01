# 🌐 Telekomunikasiýada Blokçeýn Modeli (Turkmentelekom)

Bu taslama **"Türkmentelekom" elektrik aragatnaşyk kompaniýasynyň** mysalynda, hyzmatlary dolandyrmak, tölegleri awtomatlaşdyrmak we maglumat howpsuzlygyny üpjün etmek üçin işlenip düzülen **Paýlanan Reýestr Tehnologiýasy (DLT)** modelidir.

## 📌 Taslamanyň Maksady

Merkezleşdirilen ulgamlardaky "ýeke-täk şowsuzlyk nokady" (Single Point of Failure) meselesini aradan aýyrmak, abonent maglumatlarynyň manipulýasiýasyny bökdemek we **RSA + SHA-256** algoritmleri arkaly tranzaksiýalaryň ynamlylygyny üpjün etmek.

---

## 🛠 Tehniki Binýat we Arhitektura

Taslama döwrebap C++ standartlarynda (C++17) we modully binýatda guraldy:

* **Backend:** C++ (Ýokary tizlik we resurs dolandyryşy).
* **Database:** PostgreSQL (Abonent maglumatlarynyň hemişelik saklanmagy).
* **Networking:** Boost.Asio (P2P düwünleriniň arasynda asinhron habarlaşyk).
* **Cryptography:** OpenSSL (SHA-256 we RSA kriptografik gorag).

### Esasy Gatlaklar (Layers)
1.  **Maglumat Gatlagy (Blockchain):** Bloklaryň zynjyryny we PoW (Proof of Work) mining prosesini dolandyrýar.
2.  **Howpsuzlyk Gatlagy (Digital Signature):** RSA algoritmi arkaly tranzaksiýalaryň hakykylygyny tassyklaýar.
3.  **Amallar Gatlagy (Smart Contract):** Internet, IP-TV we Telefon hyzmatlarynyň töleg şertlerini awtomatiki barlaýar.
4.  **Tor Gatlagy (P2P Node):** Täze bloklary tordaky beýleki düwünlere (peers) hakyky wagtda ýaýradýar.

---

## 📂 Taslamanyň Gurluşy (Project Tree)

```text
Turkmentelekom_Blockchain/
├── include/              # Header faýllary (.h)
│   ├── blockchain.h      # Blokçeýn gurluşy
│   ├── database.h        # PostgreSQL Singleton Manager
│   ├── p2p.h             # Boost.Asio P2P Düwüni
│   ├── signature.h       # RSA Kriptografiýa
│   └── smartContract.h   # Akylly şertnama logikasy
├── src/                  # Amala aşyryş faýllary (.cpp)
│   ├── blockchain.cpp
│   ├── database.cpp
│   ├── p2p.cpp
│   ├── signature.cpp
│   └── smartContract.cpp
├── .env                  # DB sazlamalary (DB_NAME, DB_PASS we s.m.)
├── main.cpp              # Programmanyň girişi we interaktiw menýu
├── Makefile              # Awtomatiki build ulgamy
└── README.md             # Dokumentasiýa
```

---

## 🚀 Gurnama we Işletmek

### 1. Gerekli kitaphanalar (Dependencies)
Linux (Ubuntu/Debian) ulgamynda aşakdaky buýrugy işlediň:
```bash
sudo apt-get update
sudo apt-get install libssl-dev libpqxx-dev postgresql libboost-all-dev
```

### 2. Maglumat bazasyny sazlamak
PostgreSQL-de `telekom_db` bazasyny dörediň we `.env` faýlynda öz paroluňyzy giriziň.

### 3. Kompilýasiýa we Işletmek
```bash
# Programmany arassalamak we gurmak
make clean && make

# Programmany işletmek
./telekom_system
```

---

## 📊 Tehniki Netijelilik we Howpsuzlyk

### Ulgamyň Durnuklylygy
Ulgamyň haker hüjümlerine durnuklylygy düwünleriň sany ($n$) we mining kynlygy ($d$) bilen baglylykda ähtimallyklar nazaryýeti arkaly hasaplanýar:

$$P = \sum_{k=\lceil n/2 \rceil}^{n} \binom{n}{k} p^k (1-p)^{n-k}$$

*Bellik: Paýlanan ulgamda maglumaty üýtgetmek üçin tordaky düwünleriň 51%-inden gowragyny ele geçirmek gerek, bu bolsa amaly taýdan imkansyzdyr.*

---

## 👨‍💻 Operatoryň Ulanyş Gollanmasy
1. **Port Saýlawy:** Programma açylanda lokal porty (mysal üçin: 8080) giriziň.
2. **P2P Birikme:** Tordaky beýleki operatora birikmek üçin onuň IP-sini we portuny giriziň.
3. **Hyzmat Aktiwleşdirmek:** Abonent adyny we töleg mukdaryny giriziň. Akylly şertnama tölegi tassyklasa, täze blok dörediler we torda ýaýradylar.
