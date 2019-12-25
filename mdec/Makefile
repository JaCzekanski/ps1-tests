
include Makefile.cfg

CFLAGS = -g -O3 -Wall -Werror -mtune=R2000 -march=mips1 -msoft-float -mno-gpopt -ffunction-sections -fdata-sections

all:
	mkdir -p build
	psx-gcc $(CFLAGS) -o build/mdec.elf src/mdec.c src/main.c src/chrono/tables.c src/chrono/frame.c
	elf2exe build/mdec.elf build/mdec.exe
	mipsel-unknown-elf-objdump -d build/mdec.elf > build/mdec.S