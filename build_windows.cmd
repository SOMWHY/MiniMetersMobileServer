echo "Building"
:: make build folder
if not exist .\build mkdir .\build
cd .\build

:: build
cmake ..\ || exit /b
cmake --build . --config Release || exit /b

cd ..
echo "Copying to zip"

if not exist .\releases mkdir .\releases

Xcopy /E /I .\build\MiniMetersMobile_artefacts\Release\VST3\MiniMetersMobile.vst3 .\releases\MiniMetersMobile.vst3
copy .\build\MiniMetersMobile_artefacts\Release\CLAP\MiniMetersMobile.clap .\releases
cd releases
tar.exe -a -c -f plugin-windows.zip MiniMetersMobile.clap MiniMetersMobile.vst3
cd ..

if exist .\releases\MiniMetersMobile.vst3 rmdir /s /Q .\releases\MiniMetersMobile.vst3
if exist .\releases\MiniMetersMobile.clap del .\releases\MiniMetersMobile.clap
