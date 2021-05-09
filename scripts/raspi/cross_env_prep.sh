#!/bin/sh

SYSROOT=/opt/pi-sysroot

# Use qemu to support cross-architecture debootstrap
sudo apt-get install qemu-user-static

# Create a fake rootfs
sudo qemu-debootstrap --arch=armhf --no-check-gpg buster ${SYSROOT} http://raspbian.raspberrypi.org/raspbian/

# Run installation command in chroot environment
cat << CHROOT_SCRIPT | sudo chroot $SYSROOT

# Use Google DNS for a consistant name solution 
echo 'nameserver 8.8.8.8' > /etc/resolv.conf
echo 'nameserver 8.8.4.4' >> /etc/resolv.conf

# Add archive.raspberrypi.org to APT sources
echo 'deb http://archive.raspberrypi.org/debian/ buster main' > /etc/apt/sources.list.d/raspi.list
# Trust source from above repository
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 82B129927FA3303E

# Install dependencies
apt-get -y update

# Install libraries and headers
apt-get -y install libavcodec-dev libavutil-dev libc6-dev libcurl4-openssl-dev \
    libexpat1-dev libmbedtls-dev libopus-dev libraspberrypi-dev libsdl2-dev    \
    libsdl2-image-dev uuid-dev

# Don't know why but following files are included, and they should be removed
rm -f /lib/arm-linux-gnueabihf/*.a
CHROOT_SCRIPT