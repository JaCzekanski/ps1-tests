include $(PSN00BSDK)/sdk-common.mk

TARGET = timers.elf
LIBS  += -lpsxsio -lpsxapi

disasm:
	mipsel-unknown-elf-objdump -d $(TARGET) > timers.S