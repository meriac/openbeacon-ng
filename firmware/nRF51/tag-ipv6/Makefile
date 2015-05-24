TARGET=tag-ipv6
ARCH=nrf5
CPU=$(ARCH)1822
VARIANT=xxAB
DEBUG=-g
OPTIM=-Os -mword-relocations

#
# import IoT SDK - get from "Downloads Tab" at
# https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822
#
# see: "nRF51-IoT-SDK-zip" link
#
nRF51_SDK=nrf51_iot_sdk_0.7.0.zip
nRF51_SDK_URL=http://www.nordicsemi.com/eng/nordic/download_resource/41601/6/71011617
SOFTDEVICE=Nordic/nrf51/components/softdevice/s1xx_iot/s1xx-iot-prototype2_softdevice.hex

COMP=lib/Nordic/nrf51/components
INCLUDES= \
	-I$(COMP)/softdevice/s1xx_iot/headers \
	-I$(COMP)/ble/ble_6lowpan \
	-I$(COMP)/iot/include

APP_CFLAGS=$(INCLUDES) -Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS -Wno-unused-function -Wno-unused-parameter
APP_LDFLAGS=-lm $(COMP)/ble/ble_6lowpan/lib/ble_6lowpan.a

APP_SRC= \
  lib/softdevice.s \
  src/main.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

$(nRF51_SDK):
	curl -o $@ $(nRF51_SDK_URL)

lib/softdevice.bin: $(nRF51_SDK)
	rm -rf lib/Nordic
	unzip -d lib $<
	find lib/Nordic -iname *.[ch] -exec dos2unix \{\} \;
	$(OBJCOPY) -Iihex -Obinary lib/$(SOFTDEVICE) $@

lib/softdevice.o : lib/softdevice.s lib/softdevice.bin

app_clean:
	find src lib -name '*.o' -exec rm \{\} \;

clean_all: clean
	rm -rf lib/Nordic lib/*.bin

include ../core/Makefile.rules
