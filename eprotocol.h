#pragma once
#include <stdint.h>
#define BUF_LEN 128
#define NONCE_LEN 12
#define RAND_NONCE_LEN 16
#define HASH_LEN 16
#define HELLO_MAGIC_LEN 16
#define HELLO_LEN (HELLO_MAGIC_LEN + RAND_NONCE_LEN)
#define CLIENT_HASH_LEN (RAND_NONCE_LEN + HASH_LEN)
#define SERVER_NONCES_LEN 56
#define ENC_SIZE(x) (((x) * 8 + 6 - 1) / 6)
#define CHA_COUNTER_LEN 4
typedef enum {EPRO_OK = 0, EPRO_WRONG_NONCES_LEN, EPRO_HASH_MISMATCH} EproRez;

void eproCreateHelloBuffer(uint8_t *encbuffer);
EproRez eproReadServerNonces(uint8_t *buf);
void eproCreateClientHash(uint8_t *encbuffer);
void eproCreateDataBuf(uint8_t *encbuffer, uint8_t *data, int len); 
