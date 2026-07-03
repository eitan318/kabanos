#!/bin/bash
# Host setup for MyOS developers
sudo apt update
sudo apt install -y qemu-system-x86 python3-pip 
pip3 install -r scripts/requirements.txt

# 1. Install Docker
curl -fsSL https://get.docker.com | sh

# 2. Add your user to docker group (avoid needing sudo)
sudo usermod -aG docker $USER

# 3. Enable and start Docker service
sudo systemctl enable docker
sudo systemctl start docker

# 4. Allow passwordless sudo for TAP network setup (required for make run)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
echo "$USER ALL=(ALL) NOPASSWD: /bin/bash $REPO_ROOT/scripts/run/setup-tap.sh" | sudo tee /etc/sudoers.d/kabanos-tap > /dev/null
sudo chmod 440 /etc/sudoers.d/kabanos-tap

echo "Host environment ready!"
