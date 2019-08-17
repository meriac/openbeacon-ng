[permalink](/physicalweb.html)

The Internet of Things finally got [official Bluetooth LE URL beacons](https://github.com/google/physical-web/blob/master/documentation/introduction.md)! We sure could not wait till some official hardware is around, so we added support for the physical web packet protocol to existing OpenBeacon tags.

We turned our existing OpenBeacon tag design into the ultimate hacking, fuzzing and pentesting tool for Bluetooth Low Energy and released the [hardware schematics and the PCB layout](/device.html).

We strongly believe that the future of the Internet of Things can be privacy enabled and can work distributed without selling your soul to large cloud services. Please join our project for enabling that vision!



## Open Hardware for Open Beacons!

Join the development! Get our [hardware design files](/device.html#download) to build your own [physical web](https://google.github.io/physical-web/) tags.
During the course of this night we'll release all hardware design files for the tag and the 3D printed tag programmer.


Please stay tuned for exciting software in the spirit of our [OpenPCD.org](http://www.openpcd.org/OpenPCD_2_RFID_Reader_for_13.56MHz) RFID hacking project for this new tag to:

- simulate 100s of tags at the same time with different MAC adresses - see how your phones copes :-)
- sniff & replay beacon packets from other Beacons at random transmit strength: enable privacy in a radius of 30 meters
- our beaconing firmware is implemented on the bare SoC: no annoying SoftDevice that limits your possibilities

To whet your appetite, please have a look into our [proximity detection firmware](https://github.com/meriac/openbeacon-ng/tree/master/firmware/nRF51/tag-proximity) for:

- fully encrypted tag-to-tag interaction
- AES encryption and packet signing
- 3D accelerometer code for angular detection
- true random generation and 8MB flash support

## Hardware Specification
- [3D accelerometer](http://www.st.com/web/catalog/sense_power/FM89/SC444/PF250725) for real-time movement detection
- OpenBeacon proximity & tracking protocol
- 8MB of external flash for offline-logging of [tag-to-tag proximity encounters](http://www.sociopatterns.org/deployments/infectious-sociopatterns/)
- Bluetooth low energy protocol
- 32-bit ARM Cortex M0 CPU based on the [nRF51822 SoC](https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822) from Nordic Semiconductors)
- 256KB flash 16KB RAM

## Hello World Firmware
Grab your [OpenBeacon tag](/device.html#download) and [reflash it](/source#reflash) with the [latest source code](/source#github). The firmware just sits there and continuously transmits the URL [http://get.OpenBeacon.org](http://get.OpenBeacon.org) - but here is so much [more your can do](http://www.openbeacon.org) with OpenBeacon.

<script type="syntaxhighlighter" class="brush: c"><![CDATA[
{% include src/tag-physical-web-entry.c %}
]]></script>
