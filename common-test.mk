BASEDIR = $(dir $(lastword $(MAKEFILE_LIST)))
include $(BASEDIR)/common.mk

INCLUDE	    += -I$(BASEDIR)/common
LIBDIRS		+= -L$(BASEDIR)/common
LIBS        += -lcommon -lpsxetc -lpsxgpu -lpsxapi -lc

all: $(OFILES)
	@echo "LD     $(TARGET)"
	@$(LD) $(LDFLAGS) $(LIBDIRS) $(OFILES) $(LIBS) -o $(TARGET)
	@echo "ELF2X  $(TARGET:.elf=.exe)"
	@elf2x -q $(TARGET)
	
clean:
	rm -rf build $(TARGET) $(TARGET:.elf=.exe)

.PHONY: all clean
