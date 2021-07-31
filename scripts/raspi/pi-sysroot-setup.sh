#!/bin/sh

# Use Google DNS for a consistant name solution 
echo 'nameserver 8.8.8.8' > /etc/resolv.conf
echo 'nameserver 8.8.4.4' >> /etc/resolv.conf

# Add archive.raspberrypi.org to APT sources
echo 'deb http://archive.raspberrypi.org/debian/ buster main' > /etc/apt/sources.list.d/raspi.list
# Trust source from above repository
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 82B129927FA3303E

# Install dependencies
apt-get -y update

# Install symlinks tool
apt-get install symlinks

# Install libraries and headers
apt-get install -y libsdl2-dev libsdl2-image-dev libopus-dev uuid-dev         \
     libcurl4-openssl-dev libavcodec-dev libavutil-dev libexpat1-dev          \
     libmbedtls-dev libfontconfig1-dev libraspberrypi-dev libconfig-dev

symlinks -cr /usr/lib