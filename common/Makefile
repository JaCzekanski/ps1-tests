include ../common.mk

TARGET	= libcommon.a

all: $(TARGET)

$(TARGET): $(OFILES)
	@echo "AR     $(TARGET)"
	@$(AR) cr $(TARGET) $(OFILES)
	@echo "RANLIB $(TARGET)"
	@$(RANLIB) $(TARGET)

clean:
	rm -Rf build $(TARGET)
