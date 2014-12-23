TARGET=tag-ipv6
ARCH=nrf5
CPU=$(ARCH)1822
DEBUG=-g
OPTIM=-Os -mword-relocations

#
# import IoT SDK - get from "Downloads Tab" at
# https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822
#
# see: "nRF51-IoT-SDK-zip" link
#
nRF51_SDK=nrf51_iot_sdk_0.7.0.zip
SOFTDEVICE=Nordic/nrf51/components/softdevice/s1xx_iot/s1xx-iot-prototype2_softdevice.hex

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  lib/softdevice.s \
  src/main.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

lib/softdevice.bin: $(nRF51_SDK)
	unzip -d lib $<
	$(OBJCOPY) -Iihex -Obinary lib/$(SOFTDEVICE) $@

lib/softdevice.o : lib/softdevice.s lib/softdevice.bin

app_clean:
	find src lib -name '*.o' -exec rm \{\} \;

clean_all: clean
	rm -rf lib/Nordic

include ../core/Makefile.rules
