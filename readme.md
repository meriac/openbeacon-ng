[![CircleCI](https://circleci.com/gh/meriac/openbeacon-ng.svg?style=svg)](https://circleci.com/gh/meriac/openbeacon-ng)

### Introduction

The [OpenBeacon.org](https://www.openbeacon.org/) project provides Active 2.4 GHz RFID Realtime Proximity and Position Tracking using the BlueTooth Low Energy (BLE) compatible [nRF51822](https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822) chip from Nordic Semiconductors.

Location tracking using [OpenBeacon tags](https://www.openbeacon.org/device.html) can be implemented by running an [OpenBeacon proximity tag firmware](https://www.openbeacon.org/source/) that regularly transmits beacon packets. Such packets can then be received by OpenBeacon base stations in the vicinity. Signals received by one or more base stations can then be used to determine the position of each tag in real time.

The proximity feature additionally allows to resolve human contacts by sending detecting other tags in a range of up to 6 meters and a resolution of approximately 0.5 meters. Once a contact is detected, that information is forwarded by the surrounding PoE Ethernet readers to the server infrastructure. Due to the shielding effect of the body, face-to-face contacts can be accurately detected. Optional encryption can make sure, that only base stations inside the system can read the packet information.

On this page you can find the OpenBeacon proximity tag [hardware design](https://github.com/meriac/openbeacon-ng/tree/master/hardware) and the [firmware source code](https://github.com/meriac/openbeacon-ng). You can find more information here:

- [QuickStart for OpenBeacon Tracking](quickstart.md) - this covers serial logging only. For networked live logging, please [contact us](mailto:info@bitmanufactory.com) for more information on our Linux-based network readers (Wifi & Ethernet).

- [OpenBeacon.org](https://www.openbeacon.org/device.html) Active 2.4GHz RFID Proximity TAG based on the nRF51822 chip from Nordic Semiconductors.

- [SocioPatterns.org](http://www.sociopatterns.org) research project on High-Resolution Social Networks from Wearable Proximity Sensors

- [Contact us](mailto:info@bitmanufactory.com) for further information.


### Acknowledgements

Although the code contributions are acknowledged in the source code headers and history [log files](https://github.com/meriac/openbeacon-ng/commits/master) of the [github repository](https://github.com/meriac/openbeacon-ng), this project lives from invaluable user contributions like rigorous testing, constant suggestion of new features and regular real-life deployments in the field.

The OpenBeacon Team wants to thank [Ciro Cattuto](http://www.cirocattuto.info/) from the [ISI Foundation](http://www.isi.it/) for the great collaboration during the joint development, the ongoing moral support and innovative deployments of our systems.

We especially want to thank Ciro and the [SocioPatterns collaboration](http://www.sociopatterns.org) for providing the scientific background for our platform, the media exposure and the deployments needed for the survival of our project.


### License Information

Please [contact us](mailto:license@bitmanufactory.com) for [obtaining a dual-licensed **closed source license**](mailto:license@bitmanufactory.com?subject=Alternative%20License).

#### Hardware

If you wish, you are free to manufacture the tags yourself. For that purpose you can find the schematics and hardware designs in the directory **hardware**.  The hardware design files in that folder are licensed under the Attribution-ShareAlike 4.0 International ([CC BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/)). We specifically allow commercial usage - as long as you mention the origin of our original design openly.

You can choose to use our services for bulk orders and for custom hardware development based on this and other designs we created in the past. Your financial support allows us to continue working on this awesome project - any help is highly appreciated!

#### Software

The files in the folders **firmware** and **host** are free software. You can redistribute it and/or modify them under the terms of the GNU General Public License as published by the Free Software Foundation (version 2). This program is distributed in the hope that it will be useful, but **without any warranty**; without even the implied warranty of merchantibility or fitness for a particular purpose. See the [GNU General Public License v2](http://www.gnu.org/licenses/gpl-2.0.html) for more details.


### Background Information

#### OpenBeacon.org

The [OpenBeacon.org](https://www.openbeacon.org) project was founded by [Milosch Meriac](https://www.meriac.com) and [Brita Meriac](mailto:brita@bitmanufaktory.com) in 2006 at the Berlin located company Bitmanufaktur GmbH as an open platform for active RFID applications operating in the license free 2.4GHz ISM band. OpenBeacon is based on Open Source software and a very flexible and a open design of a reprogrammable low cost Open Source Active 2.4GHz RFID hardware module. The RFID tag and reader firmware sources are available under GPL license the and tag hardware schematics are available under the open [Creative Commons license](https://creativecommons.org/). We encourage the development of new ideas for OpenBeacon setups and firmware improvements.

#### SocioPatterns

[SocioPatterns](http://www.sociopatterns.org) is an international research collaboration that uses wearable proximity sensors to map human spatial behavior in a variety of environments such as schools, hospitals, offices and social gatherings. During the last 6 years, the SocioPatterns researchers have designed and supervised over 25 data collection campaigns in 10 countries, involving over 50,000 subjects. The resulting high-resolution social networks, [shared with the public](http://www.sociopatterns.org/datasets/) whenever possible, have been used to investigate human mobility, time-varying social networks, infectious disease dynamics, location-based services, and more. The results are reported in 30+ [peer-reviewed scientific publications](http://www.sociopatterns.org/publications/) and have been featured by the [international press](http://www.sociopatterns.org/press/).

The SocioPatterns collaboration was founded by researchers and developers from the following institutions and companies:

- [ISI Foundation](http://www.isi.it/) -- Turin, Italy

- [CNRS - Centre de Physique Theorique](http://www.cpt.univ-mrs.fr) -- Marseille, France

- [ENS Lyon - Laboratoire de Physique](http://www.ens-lyon.fr/PHYSIQUE/) -- Lyon, France

- [Bitmanufactory](https://www.bitmanufactory.com/about/) -- Cambridge, United Kingdom
