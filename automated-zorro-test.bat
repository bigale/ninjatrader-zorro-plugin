@echo off
REM automated-zorro-test.bat - Automated Zorro Testing Script
REM Runs Zorro test, captures output, and analyzes results

setlocal enabledelayedexpansion

echo ========================================
echo    Automated Zorro Test Runner
echo ========================================
echo.

REM Configuration
set ZORRO_DIR=C:\Users\bigal\source\repos\ninjatrader-zorro-plugin\zorro
set ZORRO_EXE=%ZORRO_DIR%\Zorro.exe
set TEST_SCRIPT=DirectBuyTest
set DIAG_FILE=%ZORRO_DIR%\diag.txt
set LOG_FILE=%ZORRO_DIR%\Log\%TEST_SCRIPT%_test.log
set DEMO_LOG=%ZORRO_DIR%\Log\%TEST_SCRIPT%_demo.log
set ASSET_DEBUG=%ZORRO_DIR%\Log\BrokerAsset_debug.log
set TIMEOUT_SECONDS=30

REM Check if Zorro exists
if not exist "%ZORRO_EXE%" (
    echo [ERROR] Zorro not found at %ZORRO_EXE%
    exit /b 1
)

REM Step 1: Clean up old files
echo [1/6] Cleaning up old log files...
if exist "%DIAG_FILE%" (
    del /f /q "%DIAG_FILE%" 2>nul
    echo   - Deleted diag.txt
)
if exist "%LOG_FILE%" (
    del /f /q "%LOG_FILE%" 2>nul
    echo   - Deleted %TEST_SCRIPT%_test.log
)
if exist "%DEMO_LOG%" (
    del /f /q "%DEMO_LOG%" 2>nul
    echo   - Deleted %TEST_SCRIPT%_demo.log
)
if exist "%ASSET_DEBUG%" (
    del /f /q "%ASSET_DEBUG%" 2>nul
    echo   - Deleted BrokerAsset_debug.log
)
echo.

REM Step 2: Check if NinjaTrader is running
echo [2/6] Checking prerequisites...
tasklist /FI "IMAGENAME eq NinjaTrader.exe" 2>NUL | find /I /N "NinjaTrader.exe">NUL
if not "%ERRORLEVEL%"=="0" (
    echo [WARNING] NinjaTrader is not running!
    echo   Please start NinjaTrader with ZorroBridge AddOn
    set /p CONTINUE="Continue anyway? (y/n): "
    if /i not "!CONTINUE!"=="y" exit /b 1
) else (
    echo   [OK] NinjaTrader is running
)
echo.

REM Step 3: Run Zorro with diagnostics
echo [3/6] Running Zorro test...
echo   Command: zorro.exe -diag -run %TEST_SCRIPT%
echo   Timeout: %TIMEOUT_SECONDS% seconds
echo.

REM Start Zorro in background and get PID
start /B "" "%ZORRO_EXE%" -diag -run %TEST_SCRIPT%

REM Get the PID of the Zorro process
for /f "tokens=2" %%a in ('tasklist /FI "IMAGENAME eq Zorro.exe" /FO LIST ^| find "PID:"') do set ZORRO_PID=%%a

if not defined ZORRO_PID (
    echo [ERROR] Failed to get Zorro process ID
    exit /b 1
)

echo   [INFO] Zorro started with PID: %ZORRO_PID%
echo.

REM Step 4: Wait for Zorro to complete (with timeout)
echo [4/6] Waiting for Zorro to complete...
set ELAPSED=0

:wait_loop
timeout /t 1 /nobreak >nul
set /a ELAPSED+=1

REM Check if process still exists
tasklist /FI "PID eq %ZORRO_PID%" 2>NUL | find /I "%ZORRO_PID%">NUL
if "%ERRORLEVEL%"=="1" (
    echo   [OK] Zorro completed after %ELAPSED% seconds
    goto :check_logs
)

REM Check for timeout
if %ELAPSED% GEQ %TIMEOUT_SECONDS% (
    echo   [TIMEOUT] Zorro still running after %TIMEOUT_SECONDS% seconds
    echo   [INFO] Killing Zorro process...
    taskkill /PID %ZORRO_PID% /F >nul 2>&1
    goto :check_logs
)

REM Show progress
set /a MOD=%ELAPSED% %% 5
if %MOD%==0 (
    echo   Still running... (%ELAPSED%s / %TIMEOUT_SECONDS%s^)
)

goto :wait_loop

:check_logs
echo.

REM Step 5: Analyze log files
echo [5/6] Analyzing results...
echo ========================================
echo.

REM Check if diag.txt exists
if not exist "%DIAG_FILE%" (
    echo [ERROR] diag.txt not found!
    echo   Expected: %DIAG_FILE%
    goto :final_summary
)

REM Check if log file exists (try both _test and _demo)
set FOUND_LOG=0
if exist "%LOG_FILE%" (
    set ACTIVE_LOG=%LOG_FILE%
    set FOUND_LOG=1
) else if exist "%DEMO_LOG%" (
    set ACTIVE_LOG=%DEMO_LOG%
    set FOUND_LOG=1
) else (
    echo [ERROR] No log file found!
    echo   Expected: %LOG_FILE%
    echo   Or: %DEMO_LOG%
    goto :final_summary
)

echo [LOG ANALYSIS]
echo ========================================
echo.

REM Analyze diag.txt for key events
echo --- DIAG.TXT Analysis ---
echo.

REM Check for BrokerLogin
findstr /C:"BrokerLogin" "%DIAG_FILE%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] BrokerLogin called
) else (
    echo [FAIL] BrokerLogin NOT called
)

REM Check for BrokerAsset
findstr /C:"BrokerAsset" "%DIAG_FILE%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] BrokerAsset called
) else (
    echo [FAIL] BrokerAsset NOT called
)

REM Check for BrokerBuy2
findstr /C:"BrokerBuy2" "%DIAG_FILE%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] BrokerBuy2 called
    set BUY2_CALLED=1
) else (
    echo [FAIL] BrokerBuy2 NOT called
    set BUY2_CALLED=0
)

REM Check for quit
findstr /C:"quit" "%DIAG_FILE%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [INFO] Script quit detected
    
    REM Extract quit line with context
    echo.
    echo Quit context from diag.txt:
    powershell -Command "Get-Content '%DIAG_FILE%' | Select-String -Pattern 'quit' -Context 2,2"
) else (
    echo [INFO] No quit detected in diag.txt
)

echo.
echo --- LOG FILE Analysis (%TEST_SCRIPT%_*.log) ---
echo Using: %ACTIVE_LOG%
echo.

REM Check for INITRUN output
findstr /C:"DirectBuyTest" "%ACTIVE_LOG%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] INITRUN block executed
) else (
    echo [FAIL] INITRUN block NOT found
)

REM Check for DEBUG output
findstr /C:"[DEBUG]" "%ACTIVE_LOG%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] DEBUG output found
    echo.
    echo Debug lines from log:
    findstr /C:"[DEBUG]" "%ACTIVE_LOG%"
) else (
    echo [FAIL] DEBUG output NOT found (script may not have recompiled)
)

REM Check for state machine output
findstr /C:"Bar" "%ACTIVE_LOG%" | findstr /C:"Market data" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] State machine executed
) else (
    echo [FAIL] State machine NOT executed
)

REM Check for Warning 054
findstr /C:"Warning 054" "%ACTIVE_LOG%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [WARN] Warning 054 detected
    findstr /C:"Warning 054" "%ACTIVE_LOG%"
) else (
    echo [INFO] No Warning 054
)

REM Check for "Test complete"
findstr /C:"Test complete" "%ACTIVE_LOG%" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo [INFO] Script completed normally
) else (
    echo [WARN] Script did not complete normally
)

REM Check for Data/Algos/Trades line
findstr /C:"Data" "%ACTIVE_LOG%" | findstr /C:"Algos" >nul 2>&1
if %ERRORLEVEL%==0 (
    echo.
    echo .trd file info:
    findstr /C:"Data" "%ACTIVE_LOG%" | findstr /C:"Algos"
)

echo.

REM Check BrokerAsset_debug.log if exists
if exist "%ASSET_DEBUG%" (
    echo --- BROKERASSET_DEBUG.LOG Analysis ---
    echo.
    
    findstr /C:"SUBSCRIBE MODE" "%ASSET_DEBUG%" >nul 2>&1
    if %ERRORLEVEL%==0 (
        echo [INFO] BrokerAsset subscribe mode detected
    )
    
    findstr /C:"QUERY MODE" "%ASSET_DEBUG%" >nul 2>&1
    if %ERRORLEVEL%==0 (
        echo [INFO] BrokerAsset query mode detected
    )
    
    findstr /C:"WRITING pPip" "%ASSET_DEBUG%" >nul 2>&1
    if %ERRORLEVEL%==0 (
        echo [INFO] Contract specs written
    )
    echo.
)

:final_summary
echo ========================================
echo [6/6] FINAL SUMMARY
echo ========================================
echo.

if %BUY2_CALLED%==1 (
    echo [SUCCESS] BrokerBuy2 was called!
    echo   The plugin function executed successfully.
    echo   Check diag.txt for full details.
) else (
    echo [FAILURE] BrokerBuy2 was NOT called!
    echo   The script quit before placing orders.
    echo.
    echo POSSIBLE CAUSES:
    echo   1. Script did not recompile (delete .zc files)
    echo   2. .trd file replay advancing state machine
    echo   3. Global variable persistence across runs
    echo.
    echo NEXT STEPS:
    echo   1. Check last 30 lines of log:
    echo      powershell -Command "Get-Content '%ACTIVE_LOG%' -Tail 30"
    echo.
    echo   2. Check for quit in diag.txt:
    echo      powershell -Command "Get-Content '%DIAG_FILE%' | Select-String -Pattern 'quit|BrokerBuy2' -Context 2,2"
    echo.
    echo   3. Delete compiled cache:
    echo      del /f /q "%ZORRO_DIR%\Strategy\*.zc"
)

echo.
echo ========================================
echo Test completed at %TIME%
echo ========================================
echo.

endlocal
