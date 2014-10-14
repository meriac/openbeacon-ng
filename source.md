---
layout: page
title: Development Environment
permalink: /source/
---

### Setting up Development ###
Our code is hosted on [github.com](https://github.com/meriac/openbeacon-ng). Software development is officially supported for MAC OSX and Linux. Please use the latest [gcc-arm-none-eabi](https://launchpad.net/gcc-arm-embedded) toolchain from launchpad. Thanks a lot to ARM for maintaining [GNU Tools for ARM Embedded Processor](https://launchpad.net/gcc-arm-embedded).

### Compiling the Tag Firmware ###

For compiling and flashing the firmware source you need to get the [source code](https://github.com/meriac/openbeacon-ng):
<script type="syntaxhighlighter" class="brush: bash">
git clone https://github.com/meriac/openbeacon-ng
cd openbeacon-ng/firmware/nRF51/tag-physical-web
make clean flash
</script>
As you can see the makefile directly supports flashing from command line (under OSX and Linux) by using a [Segger J-Link debug probe](https://www.segger.com/jlink-software.html) from the [Nordic nRF51822 development kit](http://uk.mouser.com/Search/Refine.aspx?Keyword=949-NRF51822-DK).

### Support ###

Please sign up to our [OpenBeacon Group](https://groups.google.com/forum/#!forum/openbeacon) or contact us at [developer@openbeacon.org](mailto:developer@openbeacon.org?subject=Developer%20Support).

### Firmware Source Code ###
You can find the main loop in [firmware/nRF51/tag-physical-web/entry.c](https://github.com/meriac/openbeacon-ng/blob/master/firmware/nRF51/tag-physical-web/entry.c).

<script type="syntaxhighlighter" class="brush: c"><![CDATA[
{% include src/tag-physical-web-entry.c %}
]]></script>
