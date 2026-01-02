@echo off
echo ========================================
echo Rebuilding NT8 Plugin with Debug Output
echo ========================================
echo.

cd /d "%~dp0build"

echo Building plugin...
cmake --build . --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Installing to Zorro...
copy /Y Release\NT8.dll C:\Zorro\Plugin\NT8.dll
if errorlevel 1 (
    echo.
    echo ERROR: Failed to copy DLL
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS! Plugin rebuilt and installed
echo ========================================
echo.
echo Next steps:
echo 1. Copy ninjatrader-addon\ZorroBridge.cs to NT AddOns folder
echo 2. Press F5 in NinjaTrader to recompile
echo 3. Verify "listening on port 8888" in NT Output
echo 4. Run TradeTest in Zorro
echo 5. Watch BOTH Zorro console and NT Output window
echo.
pause
