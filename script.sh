#!/bin/sh
# zlib-1.2.5.sh by Dan Peori (danpeori@oopo.net)

if [ -f Makefile ]
then
	make clean
fi

./autogen.sh

## Configure the build.
CFLAGS="-O2 -Wall -I$PSL1GHT/ppu/include" LDFLAGS="-L$PSL1GHT/ppu/lib -lrt -llv2" ./configure \
	--prefix="$PS3DEV/portlibs/ppu" --host=powerpc64-ps3-elf \
	--enable-atomic=yes --enable-video-psl1ght=yes --enable-joystick=yes --enable-audio=yes\
	|| { exit 1; }

## Compile and install.
#make -j4 && make install || { exit 1; }

