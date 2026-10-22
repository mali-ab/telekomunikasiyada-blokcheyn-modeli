FROM debian:bookworm-slim AS api-build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libboost-system-dev \
        libpqxx-dev \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY tests ./tests
COPY web ./web

RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_DESKTOP=OFF \
        -DBUILD_CONSOLE=OFF \
        -DBUILD_WEB_API=ON \
        -DBUILD_TESTING=OFF \
    && cmake --build build --target telekom_web --parallel

FROM debian:bookworm-slim AS api

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libboost-system-dev \
        libpqxx-dev \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=api-build /src/build/telekom_web ./telekom_web
COPY web ./web
EXPOSE 8081
CMD ["./telekom_web", "8081", "/app/web"]

FROM nginx:1.27-alpine AS web

COPY docker/nginx.conf /etc/nginx/conf.d/default.conf
COPY web /usr/share/nginx/html
EXPOSE 80

