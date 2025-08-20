
# PWO Login Server


This is the login server source code for the **Poke World** game.
## Features

- [x]  Login
- [ ]  Create Account
- [ ]  Create Character
- [ ]  Account Recovery


## Stack

**Message Broker:** Redis

**Back-end:** C++, Lua

## Compilation

### Docker
    docker build -t pwo-loginserver .
    docker run -d -p 7171:7171 pwo-loginserver

### Debian GNU Linux
  Download dependencies:
  
    apt-get install build-essential cmake pkg-config libboost-all-dev libcrypto++-dev libluajit-5.1-dev libmariadb-dev libhiredis-dev libfmt-dev
    
  Compile
  
    mkdir build && cd build
    cmake ..
    make -j $(nproc)

### Windows
  You need Visual Studio 2022, then go to the vc22 folder, open **pwo-login-server.sln** and run the build. The dependencies will be installed automatically.
  
