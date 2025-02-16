#ifndef EITOA_H
#define EITOA_H
#include <stdint.h>
#define ALEN(x) (sizeof(x)/sizeof(x[0]))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define MIN(a, b) (((a) < (b))?(a):(b))
void eitoa(uint8_t *buffer, int32_t num);
#endif
