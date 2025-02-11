#!/bin/bash
# Dependency installation script for kernel build.
# Author: Siddhant Jajoo.

# I added this one
sudo apt update
sudo apt install -y flex bison bc
sudo apt install -y gcc-aarch64-linux-gnu
# These one where here
sudo apt-get install -y libssl-dev
sudo apt-get install -y u-boot-tools
sudo apt-get install -y qemu
