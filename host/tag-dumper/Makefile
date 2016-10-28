TARGET   := tag-dumper
PROX_TAG := ../../firmware/nRF51/tag-proximity
COMP_LIB := $(PROX_TAG)/lib/heatshrink
SOURCES  := \
	$(TARGET).c \
	$(COMP_LIB)/heatshrink_decoder.c \
	helper.c \
	crc32.c
OBJECTS  := $(patsubst %.c,%.o,$(SOURCES))

CC       := gcc

CPPFLAGS := -D_THREAD_SAFE -D_REENTRANT
CFLAGS   := -O3 -W -Wall -Werror -I. -I$(PROX_TAG) -I$(COMP_LIB)
LDLIBS   :=

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) *.core *~
