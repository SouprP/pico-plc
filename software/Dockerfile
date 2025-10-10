FROM debian:latest

# Core libs
RUN apt-get update -y
RUN apt-get update -y && \
    apt-get install -y \
    python3 \
    git \
    tar \
    build-essential \
    cmake  \
    pkg-config  \
    libusb-1.0-0-dev \
    gcc-arm-none-eabi  \
    libnewlib-arm-none-eabi  \
    libstdc++-arm-none-eabi-newlib && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*


# Installing Pico SDK
ARG SDK_PATH=/usr/local/picosdk
RUN git clone --depth 1 https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init

ENV PICO_SDK_PATH=$SDK_PATH

ARG PICOTOOL_PATH=/usr/local/picotool
RUN git clone https://github.com/raspberrypi/picotool.git $PICOTOOL_PATH

RUN cd $PICOTOOL_PATH && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j4 && \
    make install

WORKDIR /work