CC=arm-none-eabi-gcc
#CC=/home/eimis/d5/progs/stcube/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.12.3.rel1.linux64_1.1.0.202410170702/tools/bin/arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

# hexdump  ../c/tempendpoint/ckey.bin -e '"%x,"'
MACH=cortex-m3
CFLAGS= -c -mcpu=$(MACH) -mthumb -std=gnu11 -Wall -g -O0 -ffunction-sections -fdata-sections -I ../../c/sim800client -I ../../c/base64/include -I ../../c/chacha20/include -I ../../c/simparser/include

LDFLAGS= -mcpu=cortex-m3 -T stm32_ls.ld  -Wl,-Map=station.map  -static --specs=nano.specs -Wl,--gc-sections -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
#LDFLAGS=-mcpu=cortex-m3 -T stcubo.ld  --specs=nosys.specs -Wl,-Map="station.map" -static --specs=nano.specs  -mfloat-abi=soft -Wl,--gc-sections  -mthumb -Wl,--start-group -lc -lm -Wl,--end-group 


all: station.bin
../c/tempendpoint/ckey.bin:
	
station.bin: station.elf
	$(OBJCOPY) -O binary $^ $@
eprotocol.c: eprotocoltemp.c ../c/tempendpoint/ckey.bin
	cp eprotocoltemp.c eprotocol.c
	sed -i 's/$${chakey}/{$(shell hexdump  ../../c/tempendpoint/ckey.bin -e '"0x%x,"')}/g' eprotocol.c


station.elf: udma.o station.o delay.o gpio.o stm32_startup.o uart.o eutils.o sysmem.o syscalls.o timer.o lcd.o wire1.o clock.o motion.o nvic.o uartsim.o commands.o lcdlogs.o ../../c/sim800client/circbuf.o tempstates.o eprotocol.o ../../c/base64/ebase64.o ../../c/chacha20/src/poly1305.o ../../c/chacha20/src/chacha.o tempstream.o ../../c/simparser/src/tokenize.c
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm *.o *.elf

.PHONY: flash
flash: all
	st-flash write station.bin 0x08000000

.PHONY: reset
reset:
	st-flash reset

.PHONY: debughost
debughost:
	st-util

.PHONY: gdb
gdb: station.elf
	arm-none-eabi-gdb station.elf
