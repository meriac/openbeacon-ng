# QuickStart for OpenBeacon Tracking

## Offline Logging

When using the [Proximity Tag](../firmware/nRF51/tag-proximity) firmware,
tags can store many days of tag sightings in their proximity inside their 8MB external flash storage by using data compression.

For retrieving the stored data, we made the [tag-dumper](../host/tag-dumper) host software available. An [OpenBeacon Programmer](https://www.openbeacon.org/device.html) is used to connecting tag to a host computer for data extraction. **Always remove batteries from tags before placing them into OpenBeacon Programmer. You might destroy the reader or the tag otherwise.**

For each observation of different tags, the proximity tag stores the local time when observing the
opposite tag, the power level of the received data packet in dBm, the current angle of the tag and the
remote tag id **tag_them**. For convenience in the data processing, [tag-dumper](../host/tag-dumper) also
displays the tag ID of the currently connected tag.

> In case the tag is power-cycled, the tag time starts counting from zero again. To protect
> previous data, it is not erased unless explicitely wanted. To show the user that the tag is now counting
> at a different time offset, the rumber in **group** is incremented.
>
> For simplicity we therefore encourage to erase the tag before deployment using the programmer -
> and not to insert batteries before tags are handed out to users. These tags should therefore only
> ever display a group number of '1' and multiple time offsets can be therefore ignored.

### Retreving Offline Data

Below you can find a quick example on how to extract tag data on an Apple MacBook.
First you need to connect the [OpenBeacon Programmer](https://www.openbeacon.org/device.html) to your
laptop by using a MicroUSB cable. The nRF51-DK development kit does not need to be connected for extracting data - the development kit is only required for updating tag firmware.

> Please [update the  firmware](firmware-update.md) of your tag to **tag-proximity-log-tag.bin**
> for using the offline logging mode.

In case you see errors trying running the commands ```git``` or ```make``` below,
you might have to install them first by running ```xcode-select --install```.
```bash
# Download Sources
git clone https://github.com/meriac/openbeacon-ng
# Change into the tag-dumper host software directory
cd openbeacon-ng/host/tag-dumper/
# Compile software
make
# Run software - note that the '*' is automatically expanded to the device name of your
# OpenBeacon programmer. Each had a different name - mine is for example /dev/tty.usbserial-AK0535TL
./tag-dumper /dev/tty.usbserial-*
#  Received Tag ID=0x4A1A13B5
#  Received Tag ID=0x4A1A13B5
#  Received Tag ID=0x4A1A13B5
#  Received Tag ID=0x4A1A13B5
```
After running tag-dumper, it will print once per second *Received Tag ID=0x.....*.

If you now press the button on the tag for one second, it will start dumping stored data. Here's an
single example dataset similar to the ones you will see:
```JSON
{
	"tag_me": "0x4A1A13B5",
	"tag_them": "0x1AFF5471",
	"time_local_s": 3027,
	"time_remote_s": 3033,
	"rssi": -58,
	"angle": 90,
	"group": 1
}
```
For writing the collected tag data into a file, you simply redirect into a log file:
```bash
./tag-dumper /dev/tty.usbserial-* >>logfile.json
#  Received Tag ID=0x4A1A13B5
#  Found new log group[0] at page 0
#  Received Tag ID=0x4A1A13B5
```
Please note that '>>' keeps on appending to **logfile.json** when you run tag-dumper multiple times.
This way you can combine data from multiple runs in case you have to replaunch the tag-dumper.

### Erasing the tag

After dumping the data, you might want to erase the tag again to prime it for collecting new data.
For erasing the tag, you need to press the button on the tag for more than 3 seconds till the LED
starts blinking very quickly. Erasing the tag takes 10's of seconds. Once the erasing stopped,
the tag will go back into idle mode.

## Live Wireless Sniffing of OpenBeacon Tags

The same hardware as used can be used to observe tags in operation. By using the [reader-prox](../firmware/nRF51/reader-prox) firmware and the [openbeacon-sniffer](../host/openbeacon-sniffer) software, a tag sitting
in the [OpenBeacon Programmer](https://www.openbeacon.org/device.html) sniffs and decodes tag
transmissions off the air.

For every tag in range of the reader, one line of the data below is printed. You can see below, that the 
tag with ID 0x4A1A13B5 is in very close proximity to three other tags with the IDs 0x7EA7D3ED, 0x3DCFB9B4
and 0x1AFF5471. The reader picks up the reporting tag at -82dBm. Additionally  for each of the tags in
sight of the reporting tag, RSSI-values are reported as well (-63dBm, -66dBm, 58dBm).

The tag also reports its angle, battery voltage and its internal time stamp **time_remote_s**
(seconds after battery insertion). The key here is the **time_local_s** field - which represents the time
receiving this sighting at the host running [openbeacon-sniffer](../host/openbeacon-sniffer).
The time unit for the local time is [Unix Epoch Time](https://www.epochconverter.com/) - which is the number of seconds that have elapsed since January 1, 1970 (midnight UTC/GMT).
By combining the local time and the remote tags time, the log files for each tag can be converted into the common local time.

```JSON
{
	"uid": "0x4A1A13B5",
	"time_local_s": 1535269163,
	"time_remote_s": 567,
	"rssi": -82,
	"angle": 90,
	"voltage": 2.8,
	"tx_power": 4,
	"sighting": [{
		"uid": "0x7EA7D3ED",
		"rssi": -63
	}, {
		"uid": "0x3DCFB9B4",
		"rssi": -66
	}, {
		"uid": "0x1AFF5471",
		"rssi": -58
	}]
}
```

### Retreving Live Data
Below you can find a quick example on how to extract tag data on an Apple MacBook.
First you need to connect the [OpenBeacon Programmer](https://www.openbeacon.org/device.html) to your
laptop by using a MicroUSB cable. The nRF51-DK development kit does not need to be connected for extracting data - the development kit is only required for updating tag firmware.

> Please [update the firmware](firmware-update.md) of your tag to **reader-prox.bin**
> for running live wireless sniffing.

In case you see errors trying running the commands ```git``` or ```make``` below,
you might have to install them first by running ```xcode-select --install```.
```bash
# Download Sources
git clone https://github.com/meriac/openbeacon-ng
# Change into the openbeacon-sniffer host software directory
cd openbeacon-ng/host/openbeacon-sniffer/

# Compile software
make
#  gcc -O3 -W -Wall -Werror -I. -D_THREAD_SAFE -D_REENTRANT  -c -o openbeacon_sniffer.o openbeacon_sniffer.c
#  gcc -O3 -W -Wall -Werror -I. -D_THREAD_SAFE -D_REENTRANT  -c -o crypto.o crypto.c
#  gcc -O3 -W -Wall -Werror -I. -D_THREAD_SAFE -D_REENTRANT  -c -o crc32.o crc32.c
#  gcc -O3 -W -Wall -Werror -I. -D_THREAD_SAFE -D_REENTRANT openbeacon_sniffer.o crypto.o crc32.o -o openbeacon_sniffer

# Run software - note that the '*' is automatically expanded to the device name of your
# OpenBeacon programmer. Each had a different name - mine is for example /dev/tty.usbserial-AK0535TL
./openbeacon_sniffer /dev/tty.usbserial-*
#  fd=3
#  { "uid":"0x4A1A13B5", "time_local_s":1535276483, "time_remote_s":     382, "rssi":-84, "angle": 90, "voltage":2.7, "tx_power":4}
#  { "uid":"0x1AFF5471", "time_local_s":1535276484, "time_remote_s":    7894, "rssi":-71, "angle": 90, "voltage":2.8, "tx_power":4}
#  { "uid":"0x7EA7D3ED", "time_local_s":1535276484, "time_remote_s":    7897, "rssi":-78, "angle": 90, "voltage":2.8, "tx_power":4, "sighting": [{"uid":"0x4A1A13B5","rssi":-66},{"uid":"0x1AFF5471","rssi":-57}]}
#  { "uid":"0x4A1A13B5", "time_local_s":1535276484, "time_remote_s":     383, "rssi":-83, "angle": 90, "voltage":2.8, "tx_power":4}
#  { "uid":"0x7EA7D3ED", "time_local_s":1535276484, "time_remote_s":    7898, "rssi":-82, "angle": 90, "voltage":2.8, "tx_power":4}
#  { "uid":"0x1AFF5471", "time_local_s":1535276485, "time_remote_s":    7895, "rssi":-74, "angle": 90, "voltage":2.8, "tx_power":4, "sighting": [{"uid":"0x4A1
```
For writing the collected tag data into a file, you simply redirect into a log file:
```bash
./openbeacon_sniffer /dev/tty.usbserial-* >>logfile.json
```
Please note that '>>' keeps on appending to **logfile.json** when you run openbeacon_sniffer multiple
times. This way you can combine data from multiple runs in case you have to replaunch openbeacon_sniffer.
