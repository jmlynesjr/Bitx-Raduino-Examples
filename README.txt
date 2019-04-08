Bitx-Raduino-Examples
=====================

The Bitx40 is an inexpensive($59 USD) single band 40 meter Amateur Radio transceiver.

The uBitx is an inexpensive($119 USD) multi-band 80-10 meter Amateur Radio transceiver.

The Raduino is the digital processing board that provides the user interface
and control for these transceivers. The Raduino is based on an Arduino Nano
break-out board and a Silicon Labs si5351. The si5351 chip can provide up to 3
clock outputs. It generates the VFO and BFO clocks for the tranceiver, drives a
16x2 LCD, and reads assorted switches and a rotary encoder(or pot).

The Bitx40 and uBitx were developed by Ashhar Farhan, VU2ESE, as an inexpensive way for Amateur
Radio operators in India to get into the hobby. It is also an excellent experimenters platform
with more than 20,000 units shipped world-wide to date. Hfsignals.com is the manufacturer and
marketer for these transceivers. There are many hardware and software upgrades available
for these devices. Support is provided by an active group of Hams at https://groups.io/g/bitx20.

The code is complied and downloaded via the Arduino Integrated Development Environment(IDE). See
arduino.cc for more information.

This is my collection of customized Raduino software versions.

James M. Lynes, Jr. April 8, 2019

-----------------------------------------------------------------------------------------------

si5351VFO.ino - Stripped down version of Bitx40 Raduino code developed by Allard Munters, PE1NWL.
                    Based on Allard's v1.26, this code provides a simple digital VFO with a
                    16x2 LCD display. The standard tuning range is 7.000 MHz to 7.3000 MHz. This
                    is easily changed by modifying several defines in the code. I use this version
                    as a VFO for my Four States QRP Club(4SQRP) ZZRX-40 direct conversion receiver.
                    Others are using it to add a digital VFO capability to various vintage
                    tube based transceivers.


