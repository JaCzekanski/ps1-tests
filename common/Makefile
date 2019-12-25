include ../common.mk

TARGET	= libcommon.a

$(TARGET): $(OFILES)
	@echo "AR     $(TARGET)"
	@$(AR) cr $(TARGET) $(OFILES)
	@echo "RANLIB $(TARGET)"
	@$(RANLIB) $(TARGET)

all: $(TARGET)

clean:
	rm -Rf build $(TARGET)

.PHONY: all clean
.DEFAULT_GOAL := $(TARGET)