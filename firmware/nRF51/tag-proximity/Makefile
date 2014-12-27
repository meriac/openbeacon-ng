TARGET=tag-proximity
ARCH=nrf5
CPU=$(ARCH)1822
DEBUG=-g
OPTIM=-Os -mword-relocations
CUSTOM_KEY=inc/custom-encryption-key.h

# determine if we have custom encryption keys
ifeq ("$(wildcard $(CUSTOM_KEY))","")
	ENCRYPTION_KEY:=
else
	ENCRYPTION_KEY:=-DCUSTOM_ENCRYPTION_KEY
endif

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS $(ENCRYPTION_KEY)
APP_LDFLAGS=-lm

APP_SRC= \
  src/rng.c \
  src/acc.c \
  src/adc.c \
  src/aes.c \
  src/flash.c \
  src/log.c \
  src/heatshrink_encoder.c \
  src/radio.c \
  src/timer.c \
  src/main.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

publish: clean $(TARGET).bin
	scp $(TARGET).bin openbeacon.org:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/nrf5/

app_clean:
	find src -name '*.o' -exec rm \{\} \;

key:
	printf "static const TAES g_default_key = {\n\t`od -v -t x1 -N 16 /dev/urandom | grep 0000000 | sed 's/^[0 ]* /0x/g' | sed 's/  */, 0x/g'`\n};\n" > $(CUSTOM_KEY)

clean_all: clean
	rm -f README $(CUSTOM_KEY)

indent:
	find src inc -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*.[ch]~ inc/*.[ch]~

set_time: $(TARGET).elf
	G_TIME_ADDR=`$(NM) -C $(TARGET).elf | awk '{if ($$3 == "g_time") print $$1;}'`; \
	CTIME=`date +%s`; \
	CTIME_HEX=`printf "%x\n" $$CTIME`; \
	echo w4 $$G_TIME_ADDR $$CTIME_HEX "\n" exit | $(JLINK)

include ../core/Makefile.rules
