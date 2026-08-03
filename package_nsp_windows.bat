@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "APP_NAME=Xbox"
set "APP_VERSION=2.0"
set "TITLE_ID=0100A5B0C0DE0000"
set "FINAL_DIR=final_nsp"
set "FINAL_NAME=%APP_NAME%-%APP_VERSION%-%TITLE_ID%.nsp"

set "PACKER="
if exist "hacbrewpack.exe" set "PACKER=hacbrewpack.exe"
if exist "hacBrewPack.exe" set "PACKER=hacBrewPack.exe"

if not defined PACKER (
    echo [ERROR] hacbrewpack.exe was not found in this folder.
    echo Put a locally obtained hacBrewPack executable beside this BAT file.
    goto :fail
)

if not exist "keys.dat" (
    echo [ERROR] keys.dat was not found in this folder.
    echo Keep your own keyset local. Never upload it to GitHub or share it.
    goto :fail
)

for %%F in (
    "exefs\main"
    "exefs\main.npdm"
    "control\control.nacp"
    "control\icon_AmericanEnglish.dat"
    "control\icon_Korean.dat"
) do (
    if not exist "%%~F" (
        echo [ERROR] Missing required file: %%~F
        goto :fail
    )
)

if not exist "romfs\" (
    echo [ERROR] Missing romfs folder.
    goto :fail
)

rmdir /s /q hacbrewpack_temp 2>nul
rmdir /s /q hacbrewpack_nca 2>nul
rmdir /s /q hacbrewpack_nsp 2>nul
rmdir /s /q "%FINAL_DIR%" 2>nul
mkdir "%FINAL_DIR%" || goto :fail

echo.
echo [1/2] Packing NSP...
"%PACKER%" -k keys.dat ^
  --exefsdir exefs ^
  --romfsdir romfs ^
  --controldir control ^
  --nologo ^
  --tempdir hacbrewpack_temp ^
  --ncadir hacbrewpack_nca ^
  --nspdir hacbrewpack_nsp

if errorlevel 1 (
    echo [ERROR] NSP packaging failed.
    goto :fail
)

set "FOUND_NSP="
for %%F in (hacbrewpack_nsp\*.nsp) do (
    if exist "%%~fF" set "FOUND_NSP=%%~fF"
)

if not defined FOUND_NSP (
    echo [ERROR] hacBrewPack completed but no NSP file was found.
    goto :fail
)

echo [2/2] Renaming final package...
copy /y "!FOUND_NSP!" "%FINAL_DIR%\%FINAL_NAME%" >nul || goto :fail

echo.
echo [DONE] %FINAL_DIR%\%FINAL_NAME%
echo Do not upload keys.dat or temporary key-related files.
pause
exit /b 0

:fail
echo.
echo NSP build was not completed.
pause
exit /b 1
