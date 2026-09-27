# 🌐 Blockchain Model for Telecommunications (Turkmentelekom)

This project is a **Distributed Ledger Technology (DLT)** model developed using the example of **"Turkmentelekom" telecommunications company**, designed for managing services, automating payments, and ensuring data security.

## 🖥 Desktop UI (Windows + Linux)

The project now includes a native desktop operator interface built with **Qt Widgets**. It provides an overview dashboard, subscriber search and onboarding, payment creation, blockchain/audit view, P2P network view, and settings. It uses the native window decorations and fonts of the operating system, so it feels at home on Windows and Linux.

The desktop UI loads subscribers and blocks from PostgreSQL, creates payments through the same RSA + smart-contract + blockchain pipeline as the console app, and refreshes views after each change. Start the database (`docker compose up -d`) and run the app from the project root or from `build/` (it looks for `.env` in the current directory or one level up). P2P networking remains in the `telekom_system` console app.

### Build the desktop application

Install Qt 6 development tools (Qt 5 is also supported) and CMake, then run:

```bash
cmake -S . -B build -DBUILD_CONSOLE=OFF
cmake --build build --config Release
```

### Run the desktop application

Before launching, copy `.env.example` to `.env` if needed, enter the PostgreSQL connection values, and start the database:

```bash
docker compose up -d
```

Launch the UI from the project root so it can find `.env`:

```bash
# Linux / single-config CMake generators
./build/telekom_desktop

# macOS
./build/telekom_desktop.app/Contents/MacOS/telekom_desktop

# Windows with Visual Studio or another multi-config generator
.\build\Release\telekom_desktop.exe
```

You can also start the Linux binary from the build directory with `./telekom_desktop`; the application checks both `build/.env` and the project-root `.env`. For a Windows distribution, run Qt's `windeployqt` against the generated `.exe` to copy the required Qt DLLs. To build the original console app as well, omit `-DBUILD_CONSOLE=OFF` after installing its PostgreSQL, OpenSSL, and Boost dependencies.

### Change a subscriber's service plan

Open **Subscribers**, select a subscriber, then choose **Edit selected services** (or double-click their row). The dialog lets an operator set the Internet plan to **1, 2, 4, or 6 Mbit/s** and choose **1–10 IPTV channels**. It also shows the current phone-service status. Save applies the Internet and IPTV changes to PostgreSQL; use **New payment** to activate or renew Internet, IPTV, or phone service.

Each saved plan change is also written as a zero-payment blockchain event. Subscriber expiry dates are stored as PostgreSQL timestamps, and the application automatically migrates older text-based expiry columns at startup.

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
│   ├── smartContract.cpp
│   └── subscriber_repository.cpp # Subscriber data access
├── ui/                   # Qt desktop application
│   ├── main.cpp          # UI entry point and environment discovery
│   ├── main_window.h     # Main window interface and state
│   ├── main_window.cpp   # Data loading, refresh, and payment operations
│   ├── main_window_pages.cpp # UI pages and operator dialogs
│   └── ui_helpers.*      # Shared styles, table, and display helpers
├── .env                  # DB settings (DB_NAME, DB_PASS, etc.)
├── console/main.cpp      # Console/P2P program entry point
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
