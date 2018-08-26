CC       := gcc
CPPFLAGS := -D_THREAD_SAFE -D_REENTRANT
CFLAGS   := -O3 -W -Wall -Werror -I.
LDLIBS   := 

TARGET   := openbeacon_sniffer
SOURCES  := $(TARGET).c crypto.c crc32.c
OBJECTS  := $(patsubst %.c,%.o,$(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) *.core *~
