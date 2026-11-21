#!/bin/bash
set -e

cmake -B build-macos -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-macos --target MiniMetersMobile_VST3
cmake --build build-macos --target MiniMetersMobile_AU
cmake --build build-macos --target MiniMetersMobile_CLAP
mkdir -p ./releases

#cleanup
rm -r -f ./releases/plugin-macos.zip

#moving
cp -r ./build-macos/MiniMetersMobile_artefacts/RelWithDebInfo/VST3/MiniMetersMobile.vst3 ./releases
cp -r ./build-macos/MiniMetersMobile_artefacts/RelWithDebInfo/AU/MiniMetersMobile.component ./releases
cp -r ./build-macos/MiniMetersMobile_artefacts/RelWithDebInfo/CLAP/MiniMetersMobile.clap ./releases

#zipping
cd releases
echo "ZIP"
zip -r ./plugin-macos.zip ./MiniMetersMobile.vst3 ./MiniMetersMobile.component ./MiniMetersMobile.clap

#cleanup
rm -r -f ./MiniMetersMobile.vst3
rm -r -f ./MiniMetersMobile.component
rm -r -f ./MiniMetersMobile.clap
