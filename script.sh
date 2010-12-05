#!/bin/sh
# zlib-1.2.5.sh by Dan Peori (danpeori@oopo.net)

#newlib doesn't define endian.h that is needed by SDL

if [ ! -f $PS3DEV/ppu/ppu/include/endian.h ]
then
	cat > $PS3DEV/ppu/ppu/include/endian.h << EOF
#ifndef	_ENDIAN_H
#define	_ENDIAN_H

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321

/* PPC is big endian */
#define __BYTE_ORDER __BIG_ENDIAN

#endif

EOF

fi

if [ -f Makefile ]
then
	make clean
fi

./autogen.sh

## Configure the build.
AR="ppu-ar" CC="ppu-gcc" CFLAGS="-O2 -Wall" RANLIB="ppu-ranlib" ./configure \
	--prefix="$PS3DEV/ppu" --host=ppu-psl1ght \
	--includedir=$PSL1GHT/include --libdir=$PSL1GHT/lib \
	--enable-atomic=yes --enable-video-psl1ght=yes \
	|| { exit 1; }

## Compile and install.
#make -j4 && make install || { exit 1; }

