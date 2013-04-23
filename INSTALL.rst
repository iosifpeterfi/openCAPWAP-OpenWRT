=============================================
HOW TO BUILD AND RUN CAPWAP FOR LINUX SYSTEMS
=============================================

NOTE: To run WTP you must have a wireless card that has Linux driver based on the
      Generic IEEE 802.11 Networking Stack (mac80211).


HOW TO BUILD AC AND WTP
=======================

Requirements
------------

* automake 1.9 or newer
* autoconf
* libtool
* openssl

Build
-----

Run:

  autoreconf -f -i -s
  CFLAGS="-I/usr/include/openssl -DLOCALUDP" ./configure
  make
  make install

Use the CFLAGS setting "-DLOCALUDP" to use local UNIX sockets for
hostapd to AC/WTP communication. The openssl include path needs to
be adjusted to your local openssl installation directory.

HOW TO RUN WTP
==============

* build hostapd with capwap support, see hostapd_wrapper/README.rst for instructions
* make sure that your PCMCIA wireless card is working

* create a wireless interfaces on the WiFi card in "ap" mode,
  /usr/sbin/iw phy phy0 interface add wlan0 type managed
* adjust settings.wtp.txt and config.wtp
* run WTP in superuser mode
* run hostapd (see also hostapd_wrapper/README.rst)

HOW TO RUN AC
=============

* build hostapd with capwap support, see hostapd_wrapper/README.rst for instructions


* adjust settings.ac.txt and config.ac
* run AC in superuser mode
* run hostapd (see also hostapd_wrapper/README.rst)
