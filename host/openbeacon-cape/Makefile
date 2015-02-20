CC       := g++
CPPFLAGS := -D_THREAD_SAFE -D_REENTRANT
CFLAGS   := -O3 -W -Wall -Werror
LDLIBS   := 

TARGET   := openbeacon_forwarder
SOURCES  := $(TARGET).c
OBJECTS  := $(patsubst %c,%o,$(SOURCES))

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) *.core *~
