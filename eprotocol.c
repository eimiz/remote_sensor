#include <stdlib.h>
#include <string.h>
#include "ebase64.h"
#include "eprotocol.h"
const static uint8_t HELLO_MAGIC[HELLO_MAGIC_LEN] = {0xe9, 0xde, 0xcb, 0xd9, 0xe9, 0xd4, 0xb2, 0x7a, 0xe8, 0xde, 0xfd, 0xf9, 0x5c, 0x73, 0xd4, 0xf4};
static int currentState = 0;
static uint8_t clNonce[RAND_NONCE_LEN] = {0};
TEbase64 b64;
static void genRandom(uint8_t *buffer, int len) {
    for (int i = 0; i < len; i++) {
        uint32_t rnum;
        if (i % 4 == 0) {
            rnum = rand();
        }

        buffer[i] = rnum & 255;
        rnum >>= 8;
    }
}

void eproCreateHelloBuffer(uint8_t *bufout) {
	genRandom(clNonce, sizeof(clNonce));
	uint8_t lbuffer[HELLO_LEN];
	memcpy(lbuffer, HELLO_MAGIC, HELLO_MAGIC_LEN);
    memcpy(lbuffer + HELLO_MAGIC_LEN, clNonce, RAND_NONCE_LEN);
	ebase64Reset(&b64);
    int wrote = ebase64Encode(&b64, lbuffer, sizeof(lbuffer), bufout);
	wrote += ebase64FinishEncode(&b64, bufout + wrote);
}

