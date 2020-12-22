BASEDIR = $(dir $(lastword $(MAKEFILE_LIST)))
include $(BASEDIR)/common.mk

INCLUDE	    += -I$(realpath $(BASEDIR)/common)
LIBDIRS		+= -L$(realpath $(BASEDIR)/common)
LIBS        += -lcommon -lpsxgpu -lpsxetc -lpsxsio -lpsxapi -lpsxgte -lc

TARGET_EXE  := $(TARGET:.elf=.exe)

build/$(TARGET): $(OFILES)
	@echo "LD     $(TARGET)"
	@$(LD) $(LDFLAGS) $(LIBDIRS) $(OFILES) $(LIBS) -o build/$(TARGET)

$(TARGET_EXE): build/$(TARGET)
	@echo "ELF2X  $(TARGET:.elf=.exe)"
	@elf2x -q build/$(TARGET) $(TARGET_EXE)

all: $(TARGET_EXE)

clean:
	rm -rf build $(TARGET) $(TARGET_EXE)

.PHONY: all clean
.DEFAULT_GOAL := $(TARGET_EXE)
