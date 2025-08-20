FROM debian:bookworm-slim AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libboost-all-dev \
    libcrypto++-dev \
    libluajit-5.1-dev \
    libmariadb-dev \
    libhiredis-dev \
    libfmt-dev \
    && rm -rf /var/lib/apt/lists/*

RUN rm -rf /usr/include/mysql \
    && ln -s /usr/include/mariadb /usr/include/mysql

WORKDIR /loginserver
COPY CMakeLists.txt ./
COPY cmake ./cmake
COPY src ./src
COPY include ./include

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_MODULE_PATH=/loginserver/cmake && \
    make -j4

FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libboost-system-dev \
    libhiredis-dev \
    libluajit-5.1-dev \
    libmariadb-dev \
    libfmt-dev \
    libcrypto++8 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /loginserver

COPY --from=build /loginserver/build/loginserver ./loginserver
COPY key.pem .
COPY config.lua.docker ./config.lua
COPY lib ./lib
COPY modules ./modules

EXPOSE 7171
ENTRYPOINT ["./loginserver"]
