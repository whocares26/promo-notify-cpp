FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake g++ git \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-date-time-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    libgtest-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libboost-program-options1.74.0 \
    libboost-system1.74.0 \
    libboost-date-time1.74.0 \
    libcurl4 \
    libsqlite3-0 \
    sqlite3 \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /data

WORKDIR /app

# Копируем оба бинарника
COPY --from=builder /build/build/app/promo_notify .
COPY --from=builder /build/build/app/tests/promo_tests .

EXPOSE 18080

CMD ["/app/promo_notify"]