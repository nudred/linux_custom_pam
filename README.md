# Linux Custom PAM

Кастомный PAM модуль для аутентификации через:
- PIN (SHA256 хеш)
- TOTP (RFC6238)

Поддерживается fallback на стандартный `pam_unix`.

---

## 🚀 Установка

Установка одной командой:

```bash
curl -fsSL https://raw.githubusercontent.com/nudred/linux_custom_pam/refs/heads/main/install.sh -o install.sh
bash install.sh
```

---

## ⚙️ Настройка PAM

Добавить В НАЧАЛО файла:

```text
/etc/pam.d/common-auth
```

### Обязательный порядок:

```pam
auth sufficient pam_otp_pin.so
auth required pam_unix.so
```

⚠️ Важно:

* `pam_otp_pin.so` должен быть ПЕРВЫМ
* `pam_unix.so` должен остаться как fallback
* Нельзя заменять весь PAM стек

---

## 🧪 Тестирование

Установить pamtester (если нет):

```bash
sudo apt install pamtester
```

Проверка модуля:

```bash
pamtester login root authenticate
```

или:

```bash
pamtester sshd root authenticate
```

---

## 🔐 Хранение секретов

PIN хранится как SHA256 хеш:

```text
/etc/pam_custom/pin
```

TOTP секрет:

```text
/etc/pam_custom/otp.secret
```

Если TOTP пустой → OTP отключается автоматически.

---

## ⚠️ ВАЖНО

Неправильная настройка PAM может заблокировать доступ к системе.

Всегда держи активную SSH сессию при тестировании.
