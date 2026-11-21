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
for %%f in (INSTALL.md LICENSE NOTICE THIRD_PARTY_NOTICES.md) do (
    if exist "%%f" copy /Y "%%f" "%STAGE%" >nul || exit /b 1
)

:: Installer: install.cmd is the double-click entry point, the .ps1 does the work.
for %%f in (install.cmd Install-MiniMetersMobile.ps1) do (
    if exist "installer\%%f" copy /Y "installer\%%f" "%STAGE%" >nul || exit /b 1
)
> "%STAGE%\SOURCE.txt" echo MiniMetersMobile %VERSION% - corresponding source
>> "%STAGE%\SOURCE.txt" echo https://github.com/SOMWHY/MiniMetersMobileServer/tree/v%VERSION%
>> "%STAGE%\SOURCE.txt" echo Tag: v%VERSION%
>> "%STAGE%\SOURCE.txt" echo.
>> "%STAGE%\SOURCE.txt" echo This build is distributed under the GNU General Public License v3.
>> "%STAGE%\SOURCE.txt" echo Full licence: LICENSE. Copyright and modification statements: NOTICE.
>> "%STAGE%\SOURCE.txt" echo Third-party components: THIRD_PARTY_NOTICES.md.
>> "%STAGE%\SOURCE.txt" echo.
>> "%STAGE%\SOURCE.txt" echo MiniMetersMobile is a fork of MiniMetersServer and retains the upstream
>> "%STAGE%\SOURCE.txt" echo commit history. Upstream: Copyright (C) 2022 Joseph Lyncheski,
>> "%STAGE%\SOURCE.txt" echo https://github.com/direct-audio/MiniMetersServer .
>> "%STAGE%\SOURCE.txt" echo Modifications: Copyright (C) 2026 SOMWHY.
>> "%STAGE%\SOURCE.txt" echo.
>> "%STAGE%\SOURCE.txt" echo JUCE and clap-juce-extensions are used unmodified as git submodules;
>> "%STAGE%\SOURCE.txt" echo their pinned commits are recorded in .gitmodules of the repository above.

echo Packing
cd "%STAGE%"
:: Must call the Windows bsdtar explicitly: a GNU tar earlier on PATH ignores -a and
:: writes a plain POSIX tar, which Windows Explorer cannot open.
"%SystemRoot%\System32\tar.exe" -a -c -f "..\MiniMetersMobile-%VERSION%-windows.zip" * || exit /b 1
cd ..\..

echo Done: releases\MiniMetersMobile-%VERSION%-windows.zip
endlocal
