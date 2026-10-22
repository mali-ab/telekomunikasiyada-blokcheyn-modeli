# Blockchain Model for Telecommunications (TürkmenTelekom)

A C++17 telecom-operator prototype for managing subscribers, prepaid services, and an append-only audit ledger. PostgreSQL stores subscriber state and blocks; the Qt desktop app is the primary operator interface, while the console app contains an experimental Boost.Asio peer-to-peer demo.

This is an educational model, not a production distributed ledger or telecom billing platform. PostgreSQL is still a shared central dependency, and the P2P protocol has no authenticated peer identity, TLS, consensus, or chain synchronisation.

## What the application does

- Registers subscribers and maintains Internet, IPTV, and phone plans.
- Accepts only positive payments that cover the selected service after available balance is considered.
- Activates a service for 30 days or renews it from its current expiry, preserving remaining days.
- Writes the subscriber mutation and its ledger event in one PostgreSQL transaction.
- Mines a SHA-256 proof-of-work block, stores its nonce and hash version, and supports integrity audits.
- Records plan changes as zero-payment ledger events.

The desktop P2P and Settings pages are intentionally informational: peer networking and its runtime settings are available only in `telekom_system`.

The browser interface is served by `telekom_web`. Its `/api` routes call the same
PostgreSQL repositories, smart-contract validation, and blockchain implementation
as the desktop application.

## Quick start

### Docker installation (recommended for the web UI)

Install Docker Engine with the Docker Compose v2 plugin, then run:

```bash
cp .env.example .env
docker compose up --build -d
```

Open `http://localhost:8080`. Nginx serves the browser UI and proxies `/api` to
the C++ service; PostgreSQL is exposed on `localhost:5434` for local development.
Change `WEB_PORT` or `DB_PORT` in `.env` if either port is already in use.

Useful commands:

```bash
docker compose logs -f
docker compose down
docker compose down -v  # also removes the PostgreSQL data volume
```

The first startup creates an empty database. Add subscribers through the web UI;
the project does not install sample customer or ledger records.

### Local installation

#### 1. Start PostgreSQL only

```bash
cp .env.example .env
docker compose up -d postgres
```

PostgreSQL is exposed on host port `5434` by default.

#### 2. Install build dependencies

On Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install cmake g++ libssl-dev libpqxx-dev libboost-system-dev qt6-base-dev
```

Qt is required only for the desktop target. The console target requires Boost.System but not Qt.

#### 3. Build and run the desktop UI

```bash
cmake -S . -B build -DBUILD_DESKTOP=ON -DBUILD_CONSOLE=OFF
cmake --build build --config Release
./build/telekom_desktop
```

On macOS, launch `./build/telekom_desktop.app/Contents/MacOS/telekom_desktop`. On Windows with a multi-config generator, launch `build\Release\telekom_desktop.exe`; use `windeployqt` when packaging it.

#### 4. Build and run the console/P2P prototype

```bash
make
./build-console/telekom_system
```

#### 5. Build and run the web interface without Nginx

```bash
cmake -S . -B build-web -DBUILD_DESKTOP=OFF -DBUILD_CONSOLE=OFF -DBUILD_WEB_API=ON
cmake --build build-web --config Release
./build-web/telekom_web
```

Open `http://localhost:8081`. The web server serves the UI and its database-backed
API from the same origin. See `web/README.md` for the endpoint contract.

`make` configures CMake with `BUILD_DESKTOP=OFF`, so it works on machines without Qt. To build both targets, omit the options:

```bash
cmake -S . -B build-all
cmake --build build-all --config Release
```

#### 6. Run domain checks

```bash
make test
```

The checks cover payment rejection/activation/renewal rules, canonical block hashing, tamper detection, and UTF-8 byte handling in the demo signature routine.

## Operator workflow

1. Create the account in **Subscribers**.
2. Set the Internet speed and IPTV channel count if needed.
3. Use **New payment** to activate or renew one service. The app shows a validation error rather than writing a block for insufficient or invalid payments.
4. Open **Blockchain** and run **Integrity audit**. The interface deliberately shows `Not audited` until that check completes.

Each accepted payment creates a 30-day term. If a service is still active, its new term begins at the current expiry rather than discarding remaining days.

## Architecture

```text
ui/                    Qt Widgets operator interface
console/main.cpp       Terminal operator and experimental P2P entry point
src/smartContract.cpp  Service eligibility, renewal, and subscriber persistence
src/blockchain.cpp     Canonical SHA-256 block hashing, mining, and ledger audit
src/database.cpp       PostgreSQL connection, schema setup, and legacy migration
src/p2p.cpp            Newline-framed experimental peer transport
tests/                 Focused domain behaviour tests
```

The ledger writer uses a PostgreSQL advisory transaction lock. It reloads the persisted chain tip before mining, preventing two local application processes from committing blocks against the same stale predecessor. Subscriber changes and the block insert share that transaction; the in-memory chain updates only after commit.

New blocks use a length-prefixed, version-2 hash payload that includes index, previous hash, timestamp, data, and nonce. Earlier version-1 records remain readable. Legacy blocks without a persisted nonce receive a link-and-prefix compatibility check; they cannot be fully replayed.

## Security and scope

- SHA-256 hashing uses OpenSSL EVP.
- The `DigitalSignature` class is a small, demonstrative RSA-style routine used for the console exercise. Its tiny keys, plaintext key files, and absent persisted signatures are **not suitable for real security**. Replace it with OpenSSL EVP signing (for example, Ed25519 or RSA-PSS), protected key storage, and persisted public-key/signature metadata before any production use.
- The P2P transport now frames TCP messages correctly and shuts down its I/O thread cleanly, but it still trusts sender-supplied public keys and does not replicate subscriber state. Treat it as a lab demo only.
- A shared PostgreSQL database remains a central dependency; this project does not remove the single point of failure or establish Byzantine-fault-tolerant consensus.

## Database notes

`DatabaseManager::initTables()` creates the schema and only converts old text-based expiry/channel columns when their database type requires it. Fresh schemas constrain balances, Internet plans, and IPTV channel counts. Existing malformed legacy expiry strings are converted to `NULL` rather than preventing startup.

The schema in `scripts/init-db.sql` is applied only when Docker initializes a new
PostgreSQL volume. It creates empty `subscribers` and `blocks` tables; persistent
data is retained across normal `docker compose down` and restart operations.
