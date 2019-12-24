BASEDIR = $(dir $(lastword $(MAKEFILE_LIST)))
include $(BASEDIR)/common.mk

INCLUDE	    += -I$(BASEDIR)/common
LIBDIRS		+= -L$(BASEDIR)/common
LIBS        += -lcommon -lpsxetc -lpsxgpu -lpsxapi -lc

all: $(OFILES)
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OFILES) $(LIBS) -o $(TARGET)
	elf2x -q $(TARGET)
	
clean:
	rm -rf build $(TARGET) $(TARGET:.elf=.exe)
