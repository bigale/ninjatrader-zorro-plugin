@echo off
echo ========================================
echo Updating NinjaTrader ZorroBridge AddOn
echo ========================================
echo.

set "ADDON_SOURCE=%~dp0ninjatrader-addon\ZorroBridge.cs"
set "ADDON_DEST=C:\Users\%USERNAME%\Documents\NinjaTrader 8\bin\Custom\AddOns\ZorroBridge.cs"

echo Source: %ADDON_SOURCE%
echo Destination: %ADDON_DEST%
echo.

if not exist "%ADDON_SOURCE%" (
    echo ERROR: Source file not found!
    echo %ADDON_SOURCE%
    pause
    exit /b 1
)

echo Copying AddOn file...
copy /Y "%ADDON_SOURCE%" "%ADDON_DEST%"
if errorlevel 1 (
    echo.
    echo ERROR: Failed to copy AddOn file
    echo Check that NinjaTrader 8 is installed and path is correct
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS! AddOn file copied
echo ========================================
echo.
echo Next steps:
echo 1. Open NinjaTrader 8
echo 2. Press F5 to recompile the AddOn
echo 3. Verify "listening on port 8888" in Output window
echo 4. If errors, check Output window for compilation errors
echo.
pause
