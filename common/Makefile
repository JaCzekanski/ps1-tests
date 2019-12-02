include ../common.mk

TARGET	= libcommon.a

all: $(TARGET)

$(TARGET): $(OFILES)
	$(AR) cr $(TARGET) $(OFILES)
	$(RANLIB) $(TARGET)

clean:
	rm -Rf build $(TARGET)
