#!/bin/sh

resources="icon.bmp sail.bmp sample.bmp sample.wav utf8.txt player.bmp"

for source in *.c
do
	# is a test if it's executable (and not testnative cause it isn't ported)
	if grep -q main $source && ! echo $source | grep testnative
	then
		local project=`echo $source | sed -e 's/.c$//'` 
		tests=`echo $tests $project`
	fi
done

echo "Tests supported : $tests"

prepare() {
	#patches 
	for project in $tests
	do
		source="$project.c"
		#patch the source if needed 
		perl dir_patch.pl $source $resources
		
		echo "Creating project $project"
		mkdir -p "$project/source" "$project/include" "$project/data_bin"
		cp -v "Makefile.psl1ght" "$project/Makefile"

		if grep -q "common.h" $source
		then
			cp -v "common.h" "$project/include"
			cp -v "common.c" "$project/source"
			cp -v "icon.bmp" "$project/data_bin"
		fi

		if grep -q "picture.xbm" $source
		then
			cp -v "picture.xbm" "$project/include"
		fi

		cp "$source" "$project/source"

		#copy resources if needed
		for res in $resources 
		do
			grep -q "$res" "$source" && cp -v "$res" "$project/data_bin"
		done
	done 
	return 0
}

build() {
	cwd=`pwd`
	for i in $tests
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
	for i in $tests
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


