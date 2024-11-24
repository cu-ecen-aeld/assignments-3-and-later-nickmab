C_COMPILER=$(CROSS_COMPILE)gcc

CFLAGS=-std=c89
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition

TARGET = writer
SRC_FILES = writer.c

all: clean default

default: $(SRC_FILES)
	$(C_COMPILER) $(CFLAGS) $(SRC_FILES) -o $(TARGET)

clean:
	rm -rf $(TARGET) *.o