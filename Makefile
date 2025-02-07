CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

MACH=cortex-m3
CFLAGS= -c -mcpu=$(MACH) -mthumb -std=gnu11 -Wall -g -O0
LDFLAGS= -T stm32_ls.ld -nostdlib  -Wl,-Map=serial.map -specs=nosys.specs 

all: serial.bin

serial.bin: serial.elf
	$(OBJCOPY) -O binary $^ $@

serial.elf: udma.o serial.o delay.o led.o stm32_startup.o uart.o eutils.o
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm *.o *.elf

.PHONY: flash
flash: all
	st-flash write serial.bin 0x08000000

.PHONY: reset
reset:
	st-flash reset

.PHONY: debughost
debughost:
	st-util

.PHONY: gdb
gdb: serial.elf
	arm-none-eabi-gdb serial.elf
