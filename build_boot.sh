#!/bin/bash
cd ./lichee
./build.sh -p sun7i_android
cd ..
cd ./android
ls
source build/envsetup.sh
lunch bpi_lcd7_lvds-eng
extract-bsp
make -j8 bootimage
pack
cd ../lichee/tools/pack
ls -l
