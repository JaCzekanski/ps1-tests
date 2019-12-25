BASEDIR = $(dir $(lastword $(MAKEFILE_LIST)))
include $(BASEDIR)/common.mk

INCLUDE	    += -I$(BASEDIR)/common
LIBDIRS		+= -L$(BASEDIR)/common
LIBS        += -lcommon -lpsxetc -lpsxgpu -lpsxapi -lc

TARGET_EXE  := $(TARGET:.elf=.exe)

$(TARGET): $(OFILES)
	@echo "LD     $(TARGET)"
	@$(LD) $(LDFLAGS) $(LIBDIRS) $(OFILES) $(LIBS) -o $(TARGET)

$(TARGET_EXE): $(TARGET)
	@echo "ELF2X  $(TARGET:.elf=.exe)"
	@elf2x -q $(TARGET)

all: $(TARGET_EXE)

clean:
	rm -rf build $(TARGET) $(TARGET_EXE)

.PHONY: all clean
.DEFAULT_GOAL := $(TARGET_EXE)
