### Introduction

The [OpenBeacon.org](http://www.openbeacon.org/) project provides Active 2.4 GHz RFID Realtime Proximity and Position Tracking using the BlueTooth Low Energy (BLE) compatible [nRF51822](https://www.nordicsemi.com/eng/Products/Bluetooth-Smart-Bluetooth-low-energy/nRF51822) chip from Nordic Semiconductors.

Location tracking using [OpenBeacon tags](/device.html) can be implemented by running an [OpenBeacon proximity tag firmware](/source#github) that regularly transmits beacon packets. Such packets can then be received by [OpenBeacon base stations](http://www.openbeacon.org/EasyReader) in the vicinity. Signals received by one or more base stations can then be used to determine the [position of each tag in real time](http://www.openbeacon.org/API).

The proximity feature additionally allows to resolve human contacts by sending detecting other tags in a range of up to 6 meters and a resolution of approximately 0.5 meters. Once a contact is detected, that information is forwarded by the surrounding [PoE Ethernet readers](http://www.openbeacon.org/EasyReader) to the server infrastructure. Due to the shielding effect of the body, face-to-face contacts can be accurately detected. Optional encryption can make sure, that only base stations inside the system can read the packet information.

On this page you can find the OpenBeacon proximity tag [hardware design](/device.html#download) and the [firmware source code](/source#github). You can find more information here:

- [OpenBeacon.org](http://www.openbeacon.org) Active 2.4GHz RFID Proximity TAG based on the nRF51822 chip from Nordic Semiconductors.

- [SocioPatterns.org](http://www.sociopatterns.org) research project on High-Resolution Social Networks from Wearable Proximity Sensors

- [OpenBeacon EasyReader](http://openbeacon.org/EasyReader) Active 2.4GHz RFID Ethernet Reader

- [Contact us](mailto:info@bitmanufactory.com) for further information.


### Acknowledgements

Although the code contributions are acknowledged in the source code headers and history [log files](https://github.com/meriac/openbeacon-ng/commits/master) of the [github repository](https://github.com/meriac/openbeacon-ng), this project lives from invaluable user contributions like rigorous testing, constant suggestion of new features and regular real-life deployments in the field.

The OpenBeacon Team wants to thank [Ciro Cattuto](http://www.cirocattuto.info/) from the [ISI Foundation](http://www.isi.it/) for the great collaboration during the joint development, the ongoing moral support and innovative deployments of our systems.

We especially want to thank Ciro and the [SocioPatterns collaboration](http://www.sociopatterns.org) for providing the scientific background for our platform, the media exposure and the deployments needed for the survival of our project.


### License Information

Please [contact us](mailto:license@bitmanufactory.com) for [obtaining a dual-licensed **closed source license**](mailto:license@bitmanufactory.com?subject=Alternative%20License).

This program is free software. You can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation (version 2). This program is distributed in the hope that it will be useful, but **without any warranty**; without even the implied warranty of merchantibility or fitness for a particular purpose. See the [GNU General Public License v2](http://www.gnu.org/licenses/gpl-2.0.html) for more details.


### Background Information

#### OpenBeacon.org

The [OpenBeacon.org](http://openbeacon.org) project was founded by [Milosch Meriac](https://www.meriac.com) and [Brita Meriac](mailto:brita@bitmanufaktory.com) in 2006 at the Berlin located company Bitmanufaktur GmbH as an open platform for active RFID applications operating in the license free 2.4GHz ISM band. OpenBeacon is based on Open Source software and a very flexible and a open design of a reprogrammable low cost Open Source Active 2.4GHz RFID hardware module. The RFID tag and reader firmware sources are available under GPL license the and tag hardware schematics are available under the open [Creative Commons license](https://creativecommons.org/). We encourage the development of new ideas for OpenBeacon setups and firmware improvements.

#### SocioPatterns

[SocioPatterns](http://www.sociopatterns.org) is an international research collaboration that uses wearable proximity sensors to map human spatial behavior in a variety of environments such as schools, hospitals, offices and social gatherings. During the last 6 years, the SocioPatterns researchers have designed and supervised over 25 data collection campaigns in 10 countries, involving over 50,000 subjects. The resulting high-resolution social networks, [shared with the public](http://www.sociopatterns.org/datasets/) whenever possible, have been used to investigate human mobility, time-varying social networks, infectious disease dynamics, location-based services, and more. The results are reported in 30+ [peer-reviewed scientific publications](http://www.sociopatterns.org/publications/) and have been featured by the [international press](http://www.sociopatterns.org/press/).

The SocioPatterns collaboration was founded by researchers and developers from the following institutions and companies:

- [ISI Foundation](http://www.isi.it/) -- Turin, Italy

- [CNRS - Centre de Physique Theorique](http://www.cpt.univ-mrs.fr) -- Marseille, France

- [ENS Lyon - Laboratoire de Physique](http://www.ens-lyon.fr/PHYSIQUE/) -- Lyon, France

- [Bitmanufactory](http://bitmanufactory.com) -- Cambridge, United Kingdom
