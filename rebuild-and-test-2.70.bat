@echo off
REM rebuild-and-test-2.70.bat - Rebuild plugin and copy to Zorro 2.70 for testing

setlocal

echo ========================================
echo   Rebuild Plugin for Zorro 2.70 Test
echo ========================================
echo.

REM Step 1: Build
echo [1/3] Building NT8.dll (Release)...
cd build
cmake --build . --config Release --target NT8Plugin
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)
cd ..

echo.
echo SUCCESS: Plugin built
echo.

REM Step 2: Copy to Zorro 2.70
echo [2/3] Copying to Zorro 2.70...
set ZORRO_270_PLUGIN=C:\Users\bigal\source\repos\ninjatrader-zorro-plugin\zorro\Plugin\NT8.dll

copy /Y build\Release\NT8.dll "%ZORRO_270_PLUGIN%"
if errorlevel 1 (
    echo.
    echo ERROR: Failed to copy DLL!
    pause
    exit /b 1
)

echo.
echo SUCCESS: DLL copied to Zorro 2.70
echo.

REM Step 3: Show file info
echo [3/3] Verification...
echo.
echo Plugin DLL info:
dir "%ZORRO_270_PLUGIN%" | findstr "NT8.dll"
echo.

REM Get file timestamp
powershell -Command "Get-Item '%ZORRO_270_PLUGIN%' | Select-Object Name, LastWriteTime, Length"

echo.
echo ========================================
echo   Plugin Ready for Testing
echo ========================================
echo.
echo NEXT STEPS:
echo   1. Make sure NinjaTrader is running
echo   2. Make sure ZorroBridge AddOn compiled successfully
echo   3. Run Zorro 2.70 with SimpleOrderTest.c
echo   4. Check for Warning 054 - should be GONE!
echo   5. Check for BrokerBuy2 calls - should appear!
echo.

pause
