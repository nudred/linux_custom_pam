#!/bin/bash
set -e

REPO="https://github.com/nudred/linux_custom_pam.git"
DIR="/tmp/pam_otp_pin"

echo "[+] Cloning repo..."
rm -rf "$DIR"
git clone "$REPO" "$DIR"

cd "$DIR"

echo "[+] Building in Docker..."

docker build -t pam-builder .

docker run --rm \
  -v "$DIR:/src" \
  pam-builder \
  bash -c "gcc -fPIC -shared pam_otp_pin.c -o pam_otp_pin.so -lpam -lcrypto"

echo "[+] Build done"

echo "[+] Detecting architecture..."

ARCH=$(uname -m)

case "$ARCH" in
  x86_64)
    TARGET="/lib/x86_64-linux-gnu/security"
    ;;
  aarch64)
    TARGET="/lib/aarch64-linux-gnu/security"
    ;;
  *)
    TARGET="/lib/security"
    ;;
esac

sudo mkdir -p "$TARGET"
sudo cp pam_otp_pin.so "$TARGET/"
sudo chmod 644 "$TARGET/pam_otp_pin.so"

echo "[+] Installed to $TARGET"

echo ""
echo "[+] CONFIG"

sudo mkdir -p /etc/pam_custom

read -p "Enter PIN (or empty to skip): " PIN
if [ ! -z "$PIN" ]; then
    HASH=$(echo -n "$PIN" | sha256sum | awk '{print $1}')
    echo "$HASH" | sudo tee /etc/pam_custom/pin > /dev/null
    echo "[+] PIN saved"
else
    echo "[!] PIN skipped"
fi

read -p "Enter TOTP secret (or empty): " TOTP
if [ ! -z "$TOTP" ]; then
    echo "$TOTP" | sudo tee /etc/pam_custom/otp.secret > /dev/null
    echo "[+] TOTP saved"
else
    echo "[!] TOTP skipped"
fi

echo ""
echo "[+] PAM CONFIG HINT"

cat <<EOF

Add to /etc/pam.d/common-auth (TOP):

auth sufficient pam_otp_pin.so
auth required pam_unix.so

EOF

echo "[+] DONE"
