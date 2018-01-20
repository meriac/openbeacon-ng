TARGET=reader-prox
ARCH=nrf5
FORMAT=bin
CPU=$(ARCH)1822
VARIANT=xxAB
DEBUG=-g
OPTIM=-Os -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS $(FEATURE)
APP_LDFLAGS=-lm

APP_SRC= \
  src/radio.c \
  src/timer.c \
  src/main.c

APP_SRC+=$(IMAGES_C)

all: tracker_tag

tracker_tag: $(TARGET).$(FORMAT)

prox_tag: FEATURE=-D TAG_PROX
prox_tag: tracker_tag

testing_tag: FEATURE=-D RSSI_FILTERING=-70 -D TAG_PROX
testing_tag: tracker_tag

publish: clean $(TARGET).bin
	scp $(TARGET).bin openbeacon.org:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/nrf5/

app_clean:
	rm -f $(TARGET).$(FORMAT)
	find src -name '*.o' -exec rm \{\} \;

clean_all: clean
	rm -f README $(CUSTOM_KEY)

include ../core/Makefile.rules
