#define _GNU_SOURCE
#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <security/pam_appl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#define PIN_FILE "/etc/pam_custom/pin"
#define OTP_FILE "/etc/pam_custom/otp.secret"

/* ---------------- SHA256 ---------------- */

static void sha256(const char *str, char output[65]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str, strlen(str), hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(output + (i * 2), "%02x", hash[i]);

    output[64] = 0;
}

/* ---------------- file read ---------------- */

static int read_file(const char *path, char *out, size_t size) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    if (!fgets(out, size, f)) {
        fclose(f);
        return -1;
    }

    fclose(f);
    out[strcspn(out, "\n")] = 0;
    return 0;
}


static int base32_decode(const char *encoded, unsigned char *result, int bufSize) {
    const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int buffer = 0, bitsLeft = 0, count = 0;

    for (const char *ptr = encoded; *ptr; ptr++) {
        char *p = strchr(alphabet, *ptr);
        if (!p) continue;

        buffer <<= 5;
        buffer |= (p - alphabet) & 31;
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            if (count >= bufSize) return -1;

            result[count++] = (buffer >> (bitsLeft - 8)) & 255;
            bitsLeft -= 8;
        }
    }

    return count;
}

/* ---------------- base32 (OTP) ---------------- */

static int base32_decode(const char *encoded, unsigned char *result, int bufSize);

/* ---------------- TOTP RFC6238 ---------------- */

static int hotp(const unsigned char *key, int key_len, long counter) {
    unsigned char msg[8];

    for (int i = 7; i >= 0; i--) {
        msg[i] = counter & 0xff;
        counter >>= 8;
    }

    unsigned char hash[20];
    unsigned int len = 20;

    HMAC(EVP_sha1(), key, key_len, msg, 8, hash, &len);

    int offset = hash[19] & 0xf;
    int bin_code = (hash[offset] & 0x7f) << 24 |
                   (hash[offset+1] & 0xff) << 16 |
                   (hash[offset+2] & 0xff) << 8 |
                   (hash[offset+3] & 0xff);

    return bin_code % 1000000;
}

static int verify_totp(const char *user_code, const char *secret_b32) {
    unsigned char key[64];
    int key_len = base32_decode(secret_b32, key, sizeof(key));
    if (key_len <= 0) return 0;

    long timestep = time(NULL) / 30;

    for (int i = -1; i <= 1; i++) {
        int code = hotp(key, key_len, timestep + i);

        char buf[7];
        snprintf(buf, sizeof(buf), "%06d", code);

        if (strcmp(buf, user_code) == 0)
            return 1;
    }

    return 0;
}

/* ---------------- PAM ---------------- */

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh,
                                   int flags,
                                   int argc,
                                   const char **argv)
{
    const char *input = NULL;

    if (pam_get_authtok(pamh, PAM_AUTHTOK, &input, NULL) != PAM_SUCCESS)
        return PAM_AUTH_ERR;

    /* -------- PIN CHECK -------- */

    char pin_hash[256];
    char computed[65];

    if (read_file(PIN_FILE, pin_hash, sizeof(pin_hash)) == 0) {
        sha256(input, computed);

        if (strncmp(pin_hash, computed, 64) == 0) {
            return PAM_SUCCESS;
        }
    }

    /* -------- OTP CHECK -------- */

    char otp_secret[256];

    if (read_file(OTP_FILE, otp_secret, sizeof(otp_secret)) == 0) {
        if (verify_totp(input, otp_secret)) {
            return PAM_SUCCESS;
        }
    }

    return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh,
                              int flags,
                              int argc,
                              const char **argv)
{
    return PAM_SUCCESS;
}
