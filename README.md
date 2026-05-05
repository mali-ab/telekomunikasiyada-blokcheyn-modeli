# 🌐 Blockchain Model for Telecommunications (Turkmentelekom)

This project is a **Distributed Ledger Technology (DLT)** model developed using the example of **"Turkmentelekom" telecommunications company**, designed for managing services, automating payments, and ensuring data security.

## 📌 Project Objective

Eliminate the "Single Point of Failure" problem inherent in centralized systems, prevent manipulation of subscriber data, and ensure transaction integrity through **RSA + SHA-256** algorithms.

---

## 🛠 Technical Foundation and Architecture

The project is built using modern C++ standards (C++17) with a modular architecture:

* **Backend:** C++ (High performance and resource management).
* **Database:** PostgreSQL (Persistent storage of subscriber data).
* **Networking:** Boost.Asio (Asynchronous P2P node communication).
* **Cryptography:** OpenSSL (SHA-256 and RSA cryptographic protection).

### Core Layers
1. **Data Layer (Blockchain):** Manages the chain of blocks and the PoW (Proof of Work) mining process.
2. **Security Layer (Digital Signature):** Verifies transaction authenticity using the RSA algorithm.
3. **Business Logic Layer (Smart Contract):** Automatically validates payment conditions for Internet, IP-TV, and Phone services.
4. **Network Layer (P2P Node):** Broadcasts new blocks to other peers on the network in real time.

---

## 📂 Project Structure

```text
Turkmentelekom_Blockchain/
├── include/              # Header files (.h)
│   ├── blockchain.h      # Blockchain data structures
│   ├── database.h        # PostgreSQL Singleton Manager
│   ├── p2p.h             # Boost.Asio P2P Node
│   ├── signature.h       # RSA Cryptography
│   └── smartContract.h   # Smart contract logic
├── src/                  # Implementation files (.cpp)
│   ├── blockchain.cpp
│   ├── database.cpp
│   ├── p2p.cpp
│   ├── signature.cpp
│   └── smartContract.cpp
├── .env                  # DB settings (DB_NAME, DB_PASS, etc.)
├── main.cpp              # Program entry point and interactive menu
├── Makefile              # Automated build system
└── README.md             # Documentation
```

---

## 🚀 Installation and Running

### 1. Required Libraries (Dependencies)
On a Linux (Ubuntu/Debian) system, run:
```bash
sudo apt-get update
sudo apt-get install libssl-dev libpqxx-dev postgresql libboost-all-dev
```

### 2. Database Setup
Create the `telekom_db` database in PostgreSQL and enter your password in the `.env` file.

### 3. Compilation and Execution
```bash
# Clean and build
make clean && make

# Run the program
./telekom_system
```

---

## 📊 Technical Efficiency and Security

### System Resilience
The system's resilience against hacking attacks is calculated using probability theory, based on the number of nodes ($n$) and mining difficulty ($d$):

$$P = \sum_{k=\lceil n/2 \rceil}^{n} \binom{n}{k} p^k (1-p)^{n-k}$$

*Note: To alter data in the distributed system, an attacker would need to control more than 51% of the network nodes, which is practically infeasible.*

---

## 👨‍💻 Operator User Guide
1. **Port Selection:** Enter a local port (e.g., 8080) when the program starts.
2. **P2P Connection:** To connect to another operator on the network, enter their IP address and port.
3. **Service Activation:** Enter the subscriber name and payment amount. If the smart contract approves the payment, a new block is created and broadcast across the network.
