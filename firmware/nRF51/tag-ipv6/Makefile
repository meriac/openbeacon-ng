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
  src/main.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

$(SOFTDEVICE): $(nRF51_SDK)
	unzip $<
	touch $@

lib/softdevice.bin: $(SOFTDEVICE)
	arm-none-eabi-objcopy -Iihex -Obinary $^ $@

app_clean:
	find src -name '*.o' -exec rm \{\} \;

clean_all: clean
	rm -f Nordic

include ../core/Makefile.rules
