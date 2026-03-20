#!/bin/bash
set -e

echo "[+] Build x86_64 PAM module"

gcc -fPIC -shared pam_otp_pin.c \
    -o pam_otp_pin.so \
    -lpam -lcrypto -lssl

echo "[+] Done"
file pam_otp_pin.so
ls -lah pam_otp_pin.so
