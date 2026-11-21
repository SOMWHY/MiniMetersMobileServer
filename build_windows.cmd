@echo off
setlocal enabledelayedexpansion

:: Version is read from CMakeLists.txt so the zip name cannot drift from the binaries.
for /f "tokens=3" %%v in ('findstr /r "^project(MiniMetersMobile VERSION" CMakeLists.txt') do set "VERSION=%%v"
set "VERSION=%VERSION:)=%"
if "%VERSION%"=="" (
    echo Could not read the project version from CMakeLists.txt
    exit /b 1
)
echo Building MiniMetersMobile %VERSION%

if not exist .\build mkdir .\build
cd .\build

cmake ..\ || exit /b 1
cmake --build . --config Release || exit /b 1

cd ..
echo Staging release artefacts

set "STAGE=.\releases\MiniMetersMobile-%VERSION%-windows"
if exist "%STAGE%" rmdir /s /Q "%STAGE%"
mkdir "%STAGE%" || exit /b 1

:: VST3 is a bundle (directory), CLAP and the standalone are plain files.
Xcopy /E /I /Q .\build\MiniMetersMobile_artefacts\Release\VST3\MiniMetersMobile.vst3 "%STAGE%\MiniMetersMobile.vst3" >nul || exit /b 1
copy /Y .\build\MiniMetersMobile_artefacts\Release\CLAP\MiniMetersMobile.clap "%STAGE%" >nul || exit /b 1
copy /Y .\build\MiniMetersMobile_artefacts\Release\Standalone\MiniMetersMobile.exe "%STAGE%" >nul || exit /b 1

:: Distribution paperwork: required by the GPL and by every user who hits SmartScreen.
for %%f in (INSTALL.md LICENSE THIRD_PARTY_NOTICES.md) do (
    if exist "%%f" copy /Y "%%f" "%STAGE%" >nul || exit /b 1
)
> "%STAGE%\SOURCE.txt" echo MiniMetersMobile %VERSION% - corresponding source
>> "%STAGE%\SOURCE.txt" echo https://github.com/SOMWHY/MiniMetersMobileServer
>> "%STAGE%\SOURCE.txt" echo.
>> "%STAGE%\SOURCE.txt" echo This build is distributed under the GNU General Public License v3.
>> "%STAGE%\SOURCE.txt" echo JUCE and clap-juce-extensions are used unmodified as git submodules;
>> "%STAGE%\SOURCE.txt" echo their pinned commits are recorded in .gitmodules of the repository above.

echo Packing
cd "%STAGE%"
tar.exe -a -c -f "..\MiniMetersMobile-%VERSION%-windows.zip" * || exit /b 1
cd ..\..

echo Done: releases\MiniMetersMobile-%VERSION%-windows.zip
endlocal
