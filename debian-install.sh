#!/bin/sh
autoreconf -fi
CFLAGS="-Os -pipe -fno-caller-saves -DCW_NO_DTLS -DWRITE_STD_OUTPUT -DCW_DEBUGGING -DNETUDP -DAC -fgnu89-inline" ./configure
make
cp AC /usr/sbin/AC
mkdir /etc/capwap
cp -f config.ac /etc/capwap
cp -f settings.ac.txt /etc/capwap
cd hostapd_wrapper/hostapd2
./linux-ac.sh
