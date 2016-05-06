#!/bin/bash

cd ./lichee
./build.sh -p sun7i_android
cd ..
cd ./android
ls

echo "Setting CCACHE..."
export USE_CCACHE=1
export CCACHE_DIR=/home/dangku/mydroid/bananaPi/m1/source/android-4.4/ccache
prebuilts/misc/linux-x86/ccache/ccache -M 100G

#source build/envsetup.sh
#lunch bpi_lcd7-eng
#extract-bsp
#make -j8
#pack


source build/envsetup.sh
lunch bpi_m1plus_lcd7_lvds-userdebug
extract-bsp
make -j8
pack

cd ../lichee/tools/pack
ls -l
