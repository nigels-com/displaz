#!/bin/bash

files=""
for width in 16 32 48 128 256 ; do
    name=tmp_icon_$(printf "%.3d" $width).png
    inkscape \
        --export-png=$name \
        --export-area-page \
        --export-width=$width \
        icon.svg
    files="$files $name"
done

convert $files displaz.ico

cp icon.svg         ../src/resource/icons/hicolor/scalable/apps/displaz.svg
cp tmp_icon_016.png ../src/resource/icons/hicolor/16x16/apps/displaz.png
cp tmp_icon_032.png ../src/resource/icons/hicolor/32x32/apps/displaz.png
cp tmp_icon_048.png ../src/resource/icons/hicolor/48x48/apps/displaz.png
cp tmp_icon_128.png ../src/resource/icons/hicolor/128x128/apps/displaz.png
cp tmp_icon_256.png ../src/resource/icons/hicolor/256x256/apps/displaz.png

rm $files
