#!/bin/sh


prepare() {
	for source in test*.c
	do
		project=`echo $source | sed -e 's/.c$//'` 
		echo "preparing $project"
		mkdir -p $project/source $project/include
		cp Makefile.psl1ght $project/Makefile
		cp common.h $project/include
		cp common.c $project/source
		cp $source $project/source
	done 
	cp picture.xbm testbitmap/include/
}

build() {
	cwd=`pwd`
	for i in test*
	do
		if [ -d $i ]
		then
			echo Building $i
			cd $i
			make || return 1
			cd $cwd 
		fi
	done
}

clean () {
	for i in test*
	do
		if [ -d $i ]
		then
			echo Cleaning $i
			rm -Rf $i
		fi
	done
}

case $1 in
	build) build ;;
	clean) clean ;;
	prep) prepare ;;
	all) clean && prepare && build ;;
	*) echo "usage $0 build|clean|prep|all"
esac


