The internet of things finally has its [official Bluetooth LE URL beacons](https://google.github.io/physical-web/)! We sure could not wait till some official Hardware is around, so added support for the [physical web packet protocol](https://github.com/google/physical-web/blob/master/documentation/technical_overview.md) to existing OpenBeacon tags.

## Open Hardware for Open Beacons!
Get our [hardware design files](/device.html#download) to build your own [physical web](https://google.github.io/physical-web/) tags.

### Hardware Specification
- [3D accelerometer](http://www.st.com/web/catalog/sense_power/FM89/SC444/PF250725) for real-time movement detection
- OpenBeacon proximity & tracking protocol
- 8MB of external flash for offline-logging of [tag-to-tag proximity encounters](http://www.sociopatterns.org/deployments/infectious-sociopatterns/)
- Bluetooth low energy protocol
- 32-bit ARM Cortex M0 CPU based on the [nRF51822 SoC](https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822) from Nordic Semiconductors)
- 256KB flash 16KB RAM

### Hello World Firmware
Grab your [OpenBeacon tag](/device.html#download) and [reflash it](/source#reflash) with the [latest source code](/source#github). The firmware just sits there and continuously transmits the URL [http://get.OpenBeacon.org](http://get.OpenBeacon.org). There is so much [so much more your can do](http://www.openbeacon.org) with OpenBeacon.

<script type="syntaxhighlighter" class="brush: c"><![CDATA[
{% include src/tag-physical-web-entry.c %}
]]></script>
