@echo off
REM test-runner.bat - Automated Zorro C++ test execution
REM Builds test DLLs and runs them in Zorro

setlocal enabledelayedexpansion

echo ========================================
echo    Automated Zorro C++ Test Runner
echo ========================================
echo.

REM Configuration
set ZORRO_PATH=C:\Zorro_2.66
set ZORRO_EXE=%ZORRO_PATH%\Zorro.exe
set BUILD_DIR=build
set LOG_DIR=%ZORRO_PATH%\Log

REM Check if Zorro exists
if not exist "%ZORRO_EXE%" (
    echo ERROR: Zorro not found at %ZORRO_EXE%
    echo Please update ZORRO_PATH in this script
    exit /b 1
)

REM Step 1: Build test DLLs
echo.
echo [1/4] Building test DLLs...
echo ----------------------------------------
cd %BUILD_DIR% 2>nul || (
    echo ERROR: Build directory not found. Run cmake first!
    exit /b 1
)

cmake --build . --config Release --target PositionTest
if errorlevel 1 (
    echo ERROR: Failed to build PositionTest
    exit /b 1
)

cmake --build . --config Release --target LimitOrderTest
if errorlevel 1 (
    echo ERROR: Failed to build LimitOrderTest
    exit /b 1
)

cd ..

echo.
echo ? Test DLLs built successfully
echo.

REM Step 2: Check if NinjaTrader is running
echo [2/4] Checking prerequisites...
echo ----------------------------------------
tasklist /FI "IMAGENAME eq NinjaTrader.exe" 2>NUL | find /I /N "NinjaTrader.exe">NUL
if not "%ERRORLEVEL%"=="0" (
    echo WARNING: NinjaTrader is not running!
    echo Please start NinjaTrader with ZorroBridge AddOn before running tests
    echo.
    set /p CONTINUE="Continue anyway? (y/n): "
    if /i not "!CONTINUE!"=="y" exit /b 1
)

echo ? NinjaTrader is running
echo.

REM Step 3: Run tests
echo [3/4] Running tests...
echo ========================================
echo.

REM Test 1: PositionTest
echo Running PositionTest.dll...
echo ----------------------------------------
start /wait "" "%ZORRO_EXE%" -run PositionTest

REM Check log for results
set TEST1_LOG=%LOG_DIR%\PositionTest_demo.log
if exist "%TEST1_LOG%" (
    echo.
    echo Last 10 lines of PositionTest log:
    powershell -Command "Get-Content '%TEST1_LOG%' -Tail 10"
    echo.
    
    REM Check for pass/fail
    findstr /C:"All position tests PASSED" "%TEST1_LOG%" >nul
    if not errorlevel 1 (
        echo ? PositionTest PASSED
        set /a PASSED_TESTS+=1
    ) else (
        echo ? PositionTest FAILED
        set /a FAILED_TESTS+=1
    )
) else (
    echo WARNING: PositionTest log not found
)

echo.
echo.

REM Test 2: LimitOrderTest
echo Running LimitOrderTest.dll...
echo ----------------------------------------
start /wait "" "%ZORRO_EXE%" -run LimitOrderTest

REM Check log for results
set TEST2_LOG=%LOG_DIR%\LimitOrderTest_demo.log
if exist "%TEST2_LOG%" (
    echo.
    echo Last 10 lines of LimitOrderTest log:
    powershell -Command "Get-Content '%TEST2_LOG%' -Tail 10"
    echo.
    
    REM Check for pass/fail
    findstr /C:"All limit order tests PASSED" "%TEST2_LOG%" >nul
    if not errorlevel 1 (
        echo ? LimitOrderTest PASSED
        set /a PASSED_TESTS+=1
    ) else (
        echo ? LimitOrderTest FAILED
        set /a FAILED_TESTS+=1
    )
) else (
    echo WARNING: LimitOrderTest log not found
)

echo.
echo.

REM Step 4: Final summary
echo [4/4] Test Summary
echo ========================================
echo.
echo Tests Passed: %PASSED_TESTS%
echo Tests Failed: %FAILED_TESTS%
echo.

if %FAILED_TESTS% EQU 0 (
    echo ? All tests PASSED!
    exit /b 0
) else (
    echo ? Some tests FAILED
    exit /b 1
)
