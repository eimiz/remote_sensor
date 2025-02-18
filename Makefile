CC=arm-none-eabi-gcc
#CC=/home/eimis/d5/progs/stcube/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.12.3.rel1.linux64_1.1.0.202410170702/tools/bin/arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

MACH=cortex-m3
CFLAGS= -c -mcpu=$(MACH) -mthumb -std=gnu11 -Wall -g -O0 -ffunction-sections -fdata-sections 

LDFLAGS= -mcpu=cortex-m3 -T stm32_ls.ld  -Wl,-Map=station.map  -static --specs=nano.specs -Wl,--gc-sections -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
#LDFLAGS=-mcpu=cortex-m3 -T stcubo.ld  --specs=nosys.specs -Wl,-Map="station.map" -static --specs=nano.specs  -mfloat-abi=soft -Wl,--gc-sections  -mthumb -Wl,--start-group -lc -lm -Wl,--end-group 


all: station.bin

station.bin: station.elf
	$(OBJCOPY) -O binary $^ $@

station.elf: udma.o station.o delay.o gpio.o stm32_startup.o uart.o eutils.o sysmem.o syscalls.o timer.o lcd.o wire1.o clock.o
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
