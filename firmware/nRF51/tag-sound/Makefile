TARGET=tag-sound
ARCH=nrf5
CPU=$(ARCH)1822
VARIANT=xxAB
DEBUG=-g
OPTIM=-Os -mword-relocations
CUSTOM_KEY=inc/custom-encryption-key.h

SAMPLING_RATE=8000

# determine if we have custom encryption keys
ifeq ("$(wildcard $(CUSTOM_KEY))","")
	ENCRYPTION_KEY:=
else
	ENCRYPTION_KEY:=-DCUSTOM_ENCRYPTION_KEY
endif

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS -DSAMPLING_RATE=$(SAMPLING_RATE)
APP_LDFLAGS=-lm

APP_SRC= \
  src/sound.s \
  src/timer.c \
  src/main.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

publish: clean $(TARGET).bin
	scp $(TARGET).bin openbeacon.org:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/nrf5/

app_clean:
	find src -name '*.[os]' -exec rm \{\} \;
	rm -f sound.raw

clean_all: clean
	rm -f README $(CUSTOM_KEY)

src/sound.S: sound.raw

sound.raw: sound.wav
	sox $^ -c1 -b8 -r$(SAMPLING_RATE) -e unsigned-integer $@

play: sound.raw
	play   -c1 -b8 -r$(SAMPLING_RATE) -e unsigned-integer $^

indent:
	find src inc -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*.[ch]~ inc/*.[ch]~

include ../core/Makefile.rules
