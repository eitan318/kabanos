#!/bin/bash
# Host setup for MyOS developers
sudo apt update
sudo apt install -y qemu-system-x86 python3-pip
pip3 install -r scripts/requirements.txt
echo "Host environment ready!"
