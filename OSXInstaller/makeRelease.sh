#!/bin/sh

echo "Make sure all the binaries"
echo "Make sure all the display drivers"
echo "Make sure all the shaders are compiled"

rm -rf liquidmaya
rm -rf liquidmaya_pixie
rm -rf liquidmaya_3delight

mkdir -p ./liquidmaya/plugin/7.0/
cp -R ../bin/OSX/ ./liquidmaya/plugin/7.0/

mkdir -p ./liquidmaya/devkit
cp -R ../devkit ./liquidmaya/

cp -R ../displayDrivers ./liquidmaya/

mkdir -p ./liquidmaya/html
cp -R ../html ./liquidmaya/

mkdir -p ./liquidmaya/mel
cp -R ../mel ./liquidmaya/
rm ./liquidmaya/mel/Makefile

mkdir -p ./liquidmaya/previewRibFiles
cp -R ../previewRibFiles ./liquidmaya/

mkdir -p ./liquidmaya/renderers
cp -R ../renderers ./liquidmaya/
rm ./liquidmaya/renderers/Makefile

mkdir -p ./liquidmaya/scripts
cp -R ../scripts ./liquidmaya/

mkdir -p ./liquidmaya/shaders
cp -R ../shaders ./liquidmaya/
rm ./liquidmaya/shaders/Makefile

cp ../LICENSE.txt ./liquidmaya/

find liquidmaya -type d -name 'CVS' -exec rm -rf {} \; >&/dev/null
find liquidmaya -type f -name '\.#.*' -exec rm {} \; >&/dev/null

cp -r ./liquidmaya ./liquidmaya_pixie
rm ./liquidmaya_pixie/shaders/*.sdl		>&/dev/null
rm ./liquidmaya_pixie/shaders/*.slo		>&/dev/null
rm ./liquidmaya_pixie/plugin/7.0/liquid_3delight.lib
rm -rf ./liquidmaya_pixie/displayDrivers/3Delight

mv ./liquidmaya ./liquidmaya_3delight
rm ./liquidmaya_3delight/shaders/*.sdr		>&/dev/null
rm ./liquidmaya_3delight/shaders/*.slo		>&/dev/null
rm ./liquidmaya_3delight/plugin/7.0/liquid_pixie.lib
rm -rf ./liquidmaya_3delight/displayDrivers/Pixie

#/Developer/Tools/packagemaker -build -p ./installers -proj ./liquid_installer_pixie.pmproj -v
#tar zcvf ./liquidmaya_pixie.pkg.tgz ./liquidmaya_pixie.pkg
#/Developer/Tools/packagemaker -build -p ./installers -proj ./liquid_installer_3delight.pmproj -v
#tar zcvf ./liquidmaya_3delight.pkg.tgz ./liquidmaya_3delight.pkg
