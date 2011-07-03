#!/bin/sh
#
# Generate a header file with the current source revision

outdir=`pwd`
cd `dirname $0`
srcdir=..
header=$outdir/include/SDL_revision.h

if [ -f $header ]; then :; else
    cat >$header <<EOF
#define SDL_REVISION "hg-0:aaaaaaaaaaah"
#define SDL_REVISION_NUMBER 0
EOF
fi
rev=`sh showrev.sh 2>/dev/null`
if [ "$rev" != "" -a "$rev" != "hg-0:baadf00d" ]; then
    revnum=`echo $rev | sed 's,\(hg\|git\)-\([0-9]*\).*,\2,'`
    echo "#define SDL_REVISION \"$rev\"" >$header.new
    echo "#define SDL_REVISION_NUMBER $revnum" >>$header.new
    if diff $header $header.new >/dev/null 2>&1; then
        rm $header.new
    else
        mv $header.new $header
    fi
fi
