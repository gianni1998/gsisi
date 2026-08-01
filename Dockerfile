FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Install the C++ and x86-64 assembly development tools
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        gdb \
        nasm \
        sudo && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create a non-root development user
RUN useradd --create-home --shell /bin/bash dev && \
    echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev && \
    chmod 0440 /etc/sudoers.d/dev

USER dev
WORKDIR /home/dev