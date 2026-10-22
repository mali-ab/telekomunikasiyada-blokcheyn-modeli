# Web operator UI

Static, responsive browser UI for the TürkmenTelekom blockchain operator console.

## Install with Docker and Nginx

From the repository root:

```bash
cp .env.example .env
docker compose up --build -d
```

Open `http://localhost:8085`. The Compose stack runs three services:

- `web`: Nginx serving this directory and proxying `/api` requests.
- `api`: the existing C++ HTTP API and domain implementation.
- `postgres`: PostgreSQL with a persistent Docker volume.

Use `WEB_PORT` in `.env` to change the browser port or `API_PORT` to change the
API's direct host port. Stop the stack with `docker compose down`; add `-v` only
when you also want to delete database data.

## Run locally without Nginx

```bash
cmake -S . -B build-web -DBUILD_DESKTOP=OFF -DBUILD_CONSOLE=OFF -DBUILD_WEB_API=ON
cmake --build build-web --config Release
./build-web/telekom_web
```

Open `http://localhost:8081`. The server hosts both the static UI and the API, so
no CORS setup is needed. It reads the existing `.env` database configuration and
uses the same C++ domain logic as the desktop application. Pass a custom port and
web root as optional arguments: `./build-web/telekom_web 9000 ./web`.

Build prerequisites are the same as the console backend: a C++17 compiler,
OpenSSL, libpqxx, PostgreSQL client libraries, Boost.System, and Threads.

For a UI-only shell without PostgreSQL:

```bash
python3 -m http.server 8081 --directory web
```

## Backend contract

The API base defaults to `/api`. Override it before `app.js` loads with
`window.TELEKOM_API_URL`, or set `telekomApiUrl` in local storage.

Expected endpoints:

- `GET /dashboard`
- `GET /subscribers`
- `POST /subscribers` with `{ "name": string }`
- `GET /blocks`
- `POST /blocks/audit`
- `POST /payments` with `{ "subscriber": string, "service": string, "amount": number }`

The integration boundary is contained in `api.js`. The interface does not contain
sample records; when the API is unavailable, tables remain empty and write actions
show a connection error.
