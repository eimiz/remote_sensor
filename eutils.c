#include <string.h>
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

