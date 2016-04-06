#!/bin/bash
set -e

PLATFORM="sun7i"

show_help()
{
	printf "\nbuild.sh - Top level build scritps\n"
	echo "Valid Options:"
	echo "  -h  Show help message"
	echo "  -p <platform> platform, e.g. sun7i"
	printf "\n\n"
}

build_uboot()
{
	cd u-boot-2011.09/
	make distclean
	make ${PLATFORM}_config
	make -j16


	cd - 1>/dev/null
}

while getopts p: OPTION
do
	case $OPTION in
	p) PLATFORM=$OPTARG
	;;
	*) show_help
	;;
esac
done


build_uboot




