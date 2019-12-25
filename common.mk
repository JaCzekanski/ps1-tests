PSN00BSDK  ?= /opt/psn00bsdk/
PREFIX		= mipsel-unknown-elf-
INCLUDE	   := -I$(PSN00BSDK)/libpsn00b/include
LIBDIRS	   := -L$(PSN00BSDK)/libpsn00b
GCC_VERSION	= 7.4.0
GCC_BASE	= /usr/local/mipsel-unknown-elf

CFLAGS	   ?= -g -msoft-float -O2 -fno-builtin -fdata-sections -ffunction-sections -Wall -Wextra -Wno-strict-aliasing -Wno-sign-compare
CPPFLAGS   ?= $(CFLAGS) -fno-exceptions -std=c++17
AFLAGS	   ?= -g -msoft-float
LDFLAGS	   ?= -g -Ttext=0x80010000 -gc-sections -T $(GCC_BASE)/mipsel-unknown-elf/lib/ldscripts/elf32elmip.x

# Toolchain programs
CC			= $(PREFIX)gcc
CXX			= $(PREFIX)g++
AS			= $(PREFIX)as
AR		    = $(PREFIX)ar
LD			= $(PREFIX)ld
RANLIB	    = $(PREFIX)ranlib

LIBS	   ?= 

CFILES		= $(notdir $(wildcard *.c))
CPPFILES 	= $(notdir $(wildcard *.cpp))
AFILES		= $(notdir $(wildcard *.s))
OFILES      = $(addprefix build/,$(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(AFILES:.s=.o))
	
build/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC     $<"
	@$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
	
build/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "CXX    $<"
	@$(CXX) $(CPPFLAGS) $(INCLUDE) -c $< -o $@
	
build/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "AS     $<"
	@$(AS) $(AFLAGS) $(INCLUDE) $< -o $@
