#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ebase64.h"
#include "eprotocol.h"
#include "poly1305.h"
#include "chacha.h"
#include "uart.h"
const static uint8_t HELLO_MAGIC[HELLO_MAGIC_LEN] = {0xe9, 0xde, 0xcb, 0xd9, 0xe9, 0xd4, 0xb2, 0x7a, 0xe8, 0xde, 0xfd, 0xf9, 0x5c, 0x73, 0xd4, 0xf4};
static uint8_t clNonce[RAND_NONCE_LEN] = {0};
static uint8_t endpointNonce[RAND_NONCE_LEN];
static uint32_t polynonce[3];
static uint32_t chakey[8] = ${chakey};
static uint32_t chanonce[3];
uint32_t chacounter = 0;

TEbase64 b64;
static void incPolyNonce() {
    polynonce[2]++;
    if (polynonce[2] != 0)
        return;

    polynonce[1]++;
    if (polynonce[1] != 0)
        return;

    polynonce[0]++;
}

static void printHex(const char txt[], int len, const char *msg) {
	uartSendLog(msg);
	char buf[16];
    for (int i = 0; i < len; i++) {
        sprintf(buf, "0x%02x, ", (uint8_t)txt[i]);
		uartSendStr(buf);
        if ((i + 1) % 16 == 0)
            uartSendStr("\r\n");
    }

    uartSendStr("\r\n");
}


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

void eproCreateHelloBuffer(uint8_t *encbuffer) {
	genRandom(clNonce, sizeof(clNonce));
	uint8_t lbuffer[HELLO_LEN];
	memcpy(lbuffer, HELLO_MAGIC, HELLO_MAGIC_LEN);
    memcpy(lbuffer + HELLO_MAGIC_LEN, clNonce, RAND_NONCE_LEN);
	ebase64Reset(&b64);
    int wrote = ebase64Encode(&b64, lbuffer, sizeof(lbuffer), encbuffer);
	wrote += ebase64FinishEncode(&b64, encbuffer + wrote);
}

EproRez eproReadServerNonces(uint8_t *buf) {
    uint8_t polykey[32];
    ebase64Reset(&b64);
    //clnonce, endpoint_nonce, cha_nonce, poly_nonce, hash
    uint8_t fullbuffer[RAND_NONCE_LEN + RAND_NONCE_LEN + NONCE_LEN + NONCE_LEN + HASH_LEN];
    uint8_t decodedBuffer[SERVER_NONCES_LEN];
    int valread = ebase64Decode(&b64, buf, strlen(buf), decodedBuffer);
    if (valread != SERVER_NONCES_LEN) {
        return EPRO_WRONG_NONCES_LEN;
    }

    memcpy(fullbuffer, clNonce, RAND_NONCE_LEN);
    memcpy(fullbuffer + RAND_NONCE_LEN, decodedBuffer, SERVER_NONCES_LEN);
    memcpy(endpointNonce, decodedBuffer, RAND_NONCE_LEN);
    uint8_t *hashReceived = fullbuffer + sizeof(fullbuffer) - HASH_LEN;
    memcpy(polynonce, fullbuffer + RAND_NONCE_LEN + RAND_NONCE_LEN + NONCE_LEN, NONCE_LEN);
    polyGenKey(chakey, polynonce, polykey);
    uint8_t hash[HASH_LEN];
    printHex(fullbuffer, sizeof(fullbuffer) - HASH_LEN, "Computing hash on");
    poly(polykey, fullbuffer, sizeof(fullbuffer) - HASH_LEN, hash);
    printHex(hash, HASH_LEN, "Hash is");
    printHex(hashReceived, HASH_LEN, "Received hash");
    if (memcmp(hash, hashReceived, HASH_LEN)) {
        uartSendLog("Hash mismatch");
        return EPRO_HASH_MISMATCH;
    }

    memcpy(chanonce, fullbuffer + RAND_NONCE_LEN + RAND_NONCE_LEN, NONCE_LEN);
    uartSendLog("Hash OK\n");
    return EPRO_OK;
}

void eproCreateClientHash(uint8_t *encbuffer) {
    uint8_t polykey[32];
    uint8_t hashbuffer[RAND_NONCE_LEN + RAND_NONCE_LEN + HASH_LEN];
    memcpy(hashbuffer, endpointNonce, RAND_NONCE_LEN);
    genRandom(hashbuffer + RAND_NONCE_LEN, RAND_NONCE_LEN);
    uint8_t *hash = hashbuffer + sizeof(hashbuffer) - HASH_LEN;
    incPolyNonce();
    polyGenKey(chakey, polynonce, polykey);
    poly(polykey, hashbuffer, sizeof(hashbuffer) - HASH_LEN, hash);
    ebase64Reset(&b64);
    uint8_t *outbuffer = hashbuffer + RAND_NONCE_LEN;
    int wrote = ebase64Encode(&b64, outbuffer, CLIENT_HASH_LEN, encbuffer);
    wrote += ebase64FinishEncode(&b64, encbuffer + wrote);
}

void eproCreateDataBuf(uint8_t *encbuffer, uint8_t *data, int len) {
    chacounter++;
    uint8_t polykey[32];
    const int OUT_BUFFER_LEN =NONCE_LEN +  CHA_COUNTER_LEN + len + HASH_LEN;
                    //polynonce, counter, cryptdata, hash
    uint8_t outbuffer[OUT_BUFFER_LEN];
    uint8_t *hash = outbuffer + OUT_BUFFER_LEN - HASH_LEN;
    memcpy(outbuffer + NONCE_LEN, &chacounter, sizeof(chacounter));
    uint8_t *cdata = outbuffer + NONCE_LEN + CHA_COUNTER_LEN;
    memcpy(cdata, data, len);
    incPolyNonce();
    memcpy(outbuffer, polynonce, NONCE_LEN);
    chaEnc(chakey, chanonce, chacounter, cdata, len);
    polyGenKey(chakey, polynonce, polykey);
    poly(polykey, outbuffer, OUT_BUFFER_LEN - HASH_LEN, hash);
    printHex(outbuffer, OUT_BUFFER_LEN, "client sendEncData sending with hash");
	ebase64Reset(&b64);
    int wrote = ebase64Encode(&b64, outbuffer, OUT_BUFFER_LEN, encbuffer);
    wrote += ebase64FinishEncode(&b64, encbuffer + wrote);

}
