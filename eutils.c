#include <string.h>
#include <stdio.h>
#include "eutils.h"

void eitoa(uint8_t *buffer, int32_t num) {
    int counter = 0;
    if (num < 0) {

        *buffer++ = '-';
        num *= -1;
    }
    do {
        buffer[counter++] = '0' + num % 10;
        num /= 10;
    } while (num != 0);


    //reverse
    char tmp;
    for (int i = 0; i < counter/2; i++) {
        tmp = buffer[i];
        buffer[i] = buffer[counter - i - 1];
        buffer[counter -i - 1] = tmp;
    }

//    buffer[counter] = 0;
}

void eutilsFormatTempr(char *buf, uint8_t h, uint8_t l) {
    char *sign = "";
    int16_t combined = (h << 8) | l;
    if (combined < 0) {
        combined = ~combined + 1;
        sign = "-";
        h = ((combined & 0xf00) >> 4) | ((combined & 0xf0) >> 4);
        l = combined & 0x0f;
    } else {
        h = (h << 4) | (l  >> 4);
        l = l & 0x0f;
    }


    int lbig = l * 625;
    int lrem = lbig % 1000;
    int lrnd = lbig / 1000;
    if (lrem >= 500) {
        lrnd += 1;
    }

    sprintf(buf, "%s%i.%01i", sign, h, lrnd);
}
