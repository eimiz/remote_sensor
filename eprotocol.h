#pragma once
#include <stdint.h>
#define BUF_LEN 128
#define NONCE_LEN 12
#define RAND_NONCE_LEN 16
#define HASH_LEN 16
#define HELLO_MAGIC_LEN 16
#define HELLO_LEN (HELLO_MAGIC_LEN + RAND_NONCE_LEN)

void eproCreateHelloBuffer(uint8_t *bufout);
