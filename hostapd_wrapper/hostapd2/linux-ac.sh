#/bin/sh
cp -f hostapd_linux.conf /etc/hostapd.conf
cp -f hostapd_linux_ac.conf /etc/hostapd_ac.conf
apt-get -y purge libnl1 libnl2 hostapd
apt-get -y autoremove
rm -rf /tmp/hostapd-linux/
mkdir -p /tmp/hostapd-linux
cp -f *.tar.* .patch * /tmp/hostapd-linux
cd /tmp/hostapd-linux
tar zxvf nl-tiny.tar.gz
mv src nl-tiny
cd nl-tiny/
rm -rf *.o
make
cp libnl-tiny.so /usr/lib
cp -rf include/* /usr/include/
ldconfig -v | grep tiny
cd ..
rm -rf nl-tiny
tar jxvf hostapd-20130302.tar.bz2
cd hostapd-20130302
patch -p1 < ../hostapd-20130302-linux.patch
cd hostapd
CFLAGS="-DCONFIG_LIBNL20 -D_GNU_SOURCE -DCONFIG_MSG_MIN_PRIORITY=3 -Os -pipe -fno-caller-saves -ffunction-sections -fdata-sections -DNETUDP" make CONFIG_DRIVER_NL80211=y CONFIG_DRIVER_MADWIFI= CONFIG_DRIVER_HOSTAP= CONFIG_IEEE80211N=y CONFIG_IEEE80211W=y CONFIG_DRIVER_WEXT=  LIBS="-lm -lnl-tiny" BCHECK=  hostapd hostapd_cli
cp -f hostapd /usr/sbin/hostapd
apt-get -y install iw
cd ~
rm -rf /tmp/hostapd-linux
