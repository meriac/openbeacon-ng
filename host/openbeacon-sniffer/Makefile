CC       := gcc
CPPFLAGS := -D_THREAD_SAFE -D_REENTRANT
CFLAGS    = -O3 -W -Wall -Werror -I. $(CFLAGS_BAUD_RATE)
LDLIBS   := 

TARGET   := openbeacon_sniffer
SOURCES  := $(TARGET).c crypto.c
OBJECTS  := $(patsubst %.c,%.o,$(SOURCES))

ifneq ("$(BAUDRATE)","")
    CFLAGS_BAUD_RATE:=-DTAG_UART_BAUD_RATE=$(BAUDRATE)
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) *.core *~
