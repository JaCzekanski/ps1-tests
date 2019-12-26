TARGET = mdec.elf

BINFILES    = $(notdir $(wildcard *.bin))
MDECFILES   = $(notdir $(wildcard *.mdec))
OFILES      = $(addprefix build/,$(BINFILES:.bin=.o) $(MDECFILES:.mdec=.o))

build/%.o: %.mdec
	@mkdir -p $(dir $@)
	@echo "LD     $<"
	@$(LD) -s -r -b binary  $< -o $@

build/%.o: %.bin
	@mkdir -p $(dir $@)
	@echo "LD     $<"
	@$(LD) -s -r -b binary  $< -o $@

include ../common-test.mk
