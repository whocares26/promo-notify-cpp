# syntax=docker/dockerfile:1.6

# ===== Builder stage =====
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake make g++ git ca-certificates \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-date-time-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel && \
    strip build/app/promo_notify && \
    strip build/app/tests/promo_tests

# ===== Test stage (выключается из финального образа) =====
FROM builder AS tester
RUN cd /build/build && ctest --output-on-failure || true
RUN /build/build/app/tests/promo_tests

# ===== Runtime stage =====
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-program-options1.74.0 \
    libboost-system1.74.0 \
    libboost-date-time1.74.0 \
    libcurl4 \
    libsqlite3-0 \
    sqlite3 \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/apt/* /var/log/dpkg.log /tmp/* /var/tmp/* \
    && find /usr/share/doc -depth -type f ! -name copyright -delete \
    && find /usr/share/doc -empty -delete \
    && rm -rf /usr/share/man/* /usr/share/locale/* /usr/share/info/*

RUN mkdir -p /data

WORKDIR /app

COPY --from=builder /build/build/app/promo_notify /app/promo_notify

EXPOSE 18080

CMD ["/app/promo_notify"]
