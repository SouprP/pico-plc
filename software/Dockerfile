FROM debian:latest

# Core libs
RUN apt-get update -y
RUN apt-get update -y && \
    apt-get install -y \
    python3 \
    git \
    build-essential \
    cmake \
    gdb-multiarch \
    automake \
    autoconf \
    libtool \
    libftdi-dev \
    libusb-1.0-0-dev \
    pkg-config \
    clang-format \
    jq \
    minicom \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*


# Installing Pico SDK
ARG SDK_PATH=/usr/local/picosdk
RUN git clone --depth 1 https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init

ENV PICO_SDK_PATH=$SDK_PATH

WORKDIR /work