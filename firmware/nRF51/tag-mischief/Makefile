TARGET=tag-mischief
ARCH=nrf5
CPU=$(ARCH)1822
DEBUG=-g
OPTIM=-Os -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/acc.c \
  src/adc.c \
  src/flash.c \
  src/radio.c \
  src/timer.c \
  src/name.c \
  src/main.c

APP_SRC+=$(IMAGES_C)
DB_FILE=inc/db.h
all: $(TARGET).bin

publish: clean $(TARGET).bin
	scp $(TARGET).bin openbeacon.org:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/nrf5/

app_clean:
	find src -name '*.o' -exec rm \{\} \;

$(DB_FILE): /usr/share/dict/words
	./db-update.php $^ > $@

clean_all: clean
	rm -f README $(CUSTOM_KEY) $(DB_FILE)

include ../core/Makefile.rules
