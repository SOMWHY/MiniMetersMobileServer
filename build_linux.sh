#!/bin/bash
set -e

cmake -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --target MiniMetersMobile_VST3
cmake --build build-linux --target MiniMetersMobile_CLAP
#zip
echo "Zipping to plugin-linux.zip"
mkdir -p ./releases
rm -r -f ./releases/plugin-linux.zip
cp -r ./build-linux/MiniMetersMobile_artefacts/Release/VST3/MiniMetersMobile.vst3 ./MiniMetersMobile.vst3
cp -r ./build-linux/MiniMetersMobile_artefacts/Release/CLAP/MiniMetersMobile.clap ./
zip -r ./releases/plugin-linux.zip ./MiniMetersMobile.vst3 ./MiniMetersMobile.clap
rm -r ./MiniMetersMobile.vst3
rm -r ./MiniMetersMobile.clap
