===================
WRAPPER FOR HOSTAPD
===================

Requirements
------------

* patch
* hostapd-1.1

Install
-------

* copy content of src to hostapd-1.1/src
* apply hostapd-capwap.patch

Sample
------

  cp hostapd-1.1
  cp -a ../opencapwap.git/hostapd_wrapper/src .
  patch -p1 < ../opencapwap.git/hostapd_wrapper/hostapd-capwap.patch

Build
-----

You can either the build the AC wrapper or the WTP wrapper, but not both
at the same time!

Check hostapd's build instructions on how to configure and build it.

Adjust hostapd-1.1/hostapd/.config and enable either CONFIG_DRIVER_CAPWAP_WTP or
CONFIG_DRIVER_CAPWAP

Running WTP
-----------

* copy hostapd_wtp.conf to /etc/hostapd
* copy hostapd.conf-wtp to /etc/hostapd/hostapd.conf
* adjust above files for your setup
* create an AP interface, e.g.: /usr/sbin/iw phy phy0 interface add wlan0 type managed
* start WTP deamon *first*
* start hostapd

Running AP
-----------

* copy hostapd_ap.conf to /etc/hostapd
* copy hostapd.conf-ap to /etc/hostapd/hostapd.conf
* adjust above files for your setup
* start AP deamon *first*
* start hostapd

Known Problems
--------------

The communication protocol between WTP/AC and hostapd is not stable. It does
not detect communication failures and therefor has no reconnect behavior.
A WTP/AC restart always requires a hostapd restart as well. It also does not
handle delayed connects (i.e. hostapd started before WTP/AC is ready). Therefore
make sure the WTP/AC deamons are always started before hostapd.

The WTP does not handle AC connection lost and AC restart. The WTP needs to be
restarted to recover from such a situation.

