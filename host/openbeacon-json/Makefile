CC      :=gcc
TARGET  :=openbeacon-rx
SOURCES :=src/main.c src/crypto.c
LIBS    :=

# determine program version
PROGRAM_VERSION:=$(shell git describe --tags --abbrev=4 --dirty 2>/dev/null | sed s/^v//)
ifeq ("$(PROGRAM_VERSION)","")
    PROGRAM_VERSION:='unknown'
endif

# determine if we have custom encryption keys
ifeq ("$(wildcard src/custom-encryption-keys.h)","")
    ENCRYPTION_KEYS:=
else
    ENCRYPTION_KEYS:=-DCUSTOM_ENCRYPTION_KEYS
endif

INCLUDE :=-I../../firmware/nRF51/tag-proximity/inc/
CFLAGS:=-Werror -Wall -D_REENTRANT -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\" -DPROGRAM_NAME=\"$(TARGET)\" $(ENCRYPTION_KEYS) $(INCLUDE)
LDFLAGS :=$(LIBS)
OBJS    :=$(SOURCES:%.c=%.o)
PREFIX  :=/usr/local

all: $(TARGET)

.PHONY: all version run install indent cleanall clean

version:
	@echo "$(TARGET) version $(PROGRAM_VERSION)"

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install $(TARGET) $(PREFIX)/bin/

indent: $(SOURCES)
	find src -iname '*\.[cph]*' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*~

$(TARGET): .depend $(OBJS)
	$(CC) $(LDOPT) $(OBJS) $(LDFLAGS) -o $@

.depend: $(SOURCES)
	$(CC) $(CFLAGS) -MM $^ > $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

cleanall: clean
	rm -f .depend

clean:
	rm -f $(TARGET) $(OBJS) *~

include .depend
