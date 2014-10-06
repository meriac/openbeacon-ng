## Yay!
The internet of things finally has its [official Bluetooth LE URL beacons](https://google.github.io/physical-web/)! We sure could not wait till some official Hardware is around, so added support for the [physical web packet protocol](https://github.com/google/physical-web/blob/master/documentation/technical_overview.md) to existing OpenBeacon tags.

### Open Hardware for Open Beacons!
Get our [hardware design files](/device.html) to build your own physical web tags.

#### Hardware Specification
- [3D accelerometer](http://www.st.com/web/catalog/sense_power/FM89/SC444/PF250725) for real-time movement detection
- 32-bit ARM Cortex M0 CPU based on the [nRF51822 SoC](https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822) from Nordic Semiconductors)
- 256KB flash 16KB RAM
- Bluetooth low energy protocol


#### Hello World Firmware
Grab your [OpenBeacon tag](/device.html) and [reflash it](/source#reflash) with the [latest firmware](/source#github). This firmware just sits there and sends the URL [http://get.OpenBeacon.org](http://get.OpenBeacon.org).

<script type="syntaxhighlighter" class="brush: c"><![CDATA[
{% include tag-physical-web-entry.c %}
]]></script>

## World
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam sodales urna non odio egestas tempor. Nunc vel vehicula ante. Etiam bibendum iaculis libero, eget molestie nisl pharetra in. In semper consequat est, eu porta velit mollis nec. Curabitur posuere enim eget turpis feugiat tempor.
