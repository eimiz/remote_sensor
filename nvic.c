#include <stdint.h>
void nvicEnableIRQ(int irq) {
    uint32_t *pISER = (uint32_t *)(0xE000E100);
    pISER[irq / 32] |= 1 << (irq % 32);
}
