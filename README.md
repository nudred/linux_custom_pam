# Linux Custom PAM

## 1. DOCKER СБОРКА

```Dockerfile
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    gcc \
    libpam0g-dev \
    libssl-dev \
    file
    
WORKDIR /src
CMD ["bash"]
```

### build.sh

```
#!/bin/bash
set -e

echo "[+] build PAM module"

gcc -fPIC -shared pam_otp_pin.c \
  -o pam_otp_pin.so \
  -lpam -lcrypto -lssl

echo "[+] done"
file pam_otp_pin.so
ls -lah pam_otp_pin.so
```

### build

```bash
docker build -t pam-builder .
```

```bash
docker run --rm -v "$PWD:/src" pam-builder bash build.sh
```

## 2. УСТАНОВКА МОДУЛЯ

```bash
sudo mkdir -p /lib/security
sudo cp pam_otp_pin.so /lib/security/
sudo chmod 644 /lib/security/pam_otp_pin.so
```

## 3. ХРАНЕНИЕ СЕКРЕТОВ

### PIN (SHA256)

```bash
echo -n "1234" | sha256sum | awk '{print $1}' | sudo tee /etc/pam_custom/pin
```

### OTP SECRET

```bash
sudo mkdir -p /etc/pam_custom
echo "JBSWY3DPEHPK3PXP" | sudo tee /etc/pam_custom/otp.secret
```

### генерация OTP

```bash
oathtool --totp -b JBSWY3DPEHPK3PXP
```

## 4. PAM КОНФИГУРАЦИЯ (ГЛАВНАЯ СХЕМА)

ИСПОЛЬЗУЕТСЯ ВЕЗДЕ:

```
sudo / ssh / login / sddm / lockscreen
```

### /etc/pam.d/common-auth

```pam
auth sufficient pam_otp_pin.so
auth sufficient pam_unix.so
```

## 5. SUDO

```bash
sudo nano /etc/pam.d/sudo
```

```pam
auth sufficient pam_otp_pin.so
auth sufficient pam_unix.so

@include common-auth
```

## 6. SSH

```bash
sudo nano /etc/pam.d/sshd
```

```pam
auth sufficient pam_otp_pin.so
auth sufficient pam_unix.so

@include common-auth

```

## 7. LOGIN (TTY)

```bash
sudo nano /etc/pam.d/login
```

```pam
auth sufficient pam_otp_pin.so
auth sufficient pam_unix.so

@include common-auth
```

## 8. SDDM (GUI LOGIN)

```bash
sudo nano /etc/pam.d/sddm
```

```pam
auth sufficient pam_otp_pin.so
auth sufficient pam_unix.so

@include common-auth
```

## 9. TESTING

```bash
sudo apt install pamtester
```

```bash
pamtester login root authenticate
pamtester sudo root authenticate
pamtester sshd root authenticate
```

## 10. DEBUG

```bash
journalctl -f
```

```bash
loginctl lock-session
```

## 11. БЕЗОПАСНОСТЬ

НЕ использовать:

`auth required pam_otp_pin.so`   ❌ (может заблокировать систему)

## 12. РЕКОМЕНДУЕМАЯ СХЕМА

```pam
auth sufficient pam_otp_pin.so
auth sufficient pam_unix.so
```
