FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-date-time-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Скачиваем crow.h (header-only библиотека)
RUN wget -q https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h \
    -O /usr/local/include/crow.h

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
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /data

COPY --from=builder /build/build/app/promo_notify /usr/local/bin/promo_notify

EXPOSE 18080

CMD ["/usr/local/bin/promo_notify"]