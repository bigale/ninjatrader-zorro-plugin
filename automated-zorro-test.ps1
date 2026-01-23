# automated-zorro-test.ps1 - Automated Zorro Testing Script
# Runs Zorro test, captures output, and analyzes results

param(
    [string]$TestScript = "DirectBuyTest",
    [int]$TimeoutSeconds = 30,
    [switch]$DeleteCache,
    [switch]$OpenLogs
)

# Configuration
$ZORRO_DIR = "C:\Users\bigal\source\repos\ninjatrader-zorro-plugin\zorro"
$ZORRO_EXE = Join-Path $ZORRO_DIR "Zorro.exe"
$DIAG_FILE = Join-Path $ZORRO_DIR "diag.txt"
$LOG_FILE = Join-Path $ZORRO_DIR "Log\$TestScript`_test.log"
$DEMO_LOG = Join-Path $ZORRO_DIR "Log\$TestScript`_demo.log"
$ASSET_DEBUG = Join-Path $ZORRO_DIR "Log\BrokerAsset_debug.log"
$STRATEGY_DIR = Join-Path $ZORRO_DIR "Strategy"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Automated Zorro Test Runner" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if Zorro exists
if (-not (Test-Path $ZORRO_EXE)) {
    Write-Host "[ERROR] Zorro not found at $ZORRO_EXE" -ForegroundColor Red
    exit 1
}

# Step 1: Clean up old files
Write-Host "[1/6] Cleaning up old log files..." -ForegroundColor Yellow

$filesToDelete = @($DIAG_FILE, $LOG_FILE, $DEMO_LOG, $ASSET_DEBUG)
foreach ($file in $filesToDelete) {
    if (Test-Path $file) {
        Remove-Item $file -Force
        Write-Host "  - Deleted $(Split-Path $file -Leaf)" -ForegroundColor Gray
    }
}

# Delete compiled cache if requested
if ($DeleteCache) {
    Write-Host "  [INFO] Deleting compiled script cache (.zc files)..." -ForegroundColor Cyan
    Get-ChildItem "$STRATEGY_DIR\*.zc" -ErrorAction SilentlyContinue | Remove-Item -Force
    Write-Host "  - Deleted .zc files" -ForegroundColor Gray
}

Write-Host ""

# Step 2: Check prerequisites
Write-Host "[2/6] Checking prerequisites..." -ForegroundColor Yellow

$ntRunning = Get-Process -Name "NinjaTrader" -ErrorAction SilentlyContinue
if (-not $ntRunning) {
    Write-Host "  [WARNING] NinjaTrader is not running!" -ForegroundColor Yellow
    Write-Host "  Please start NinjaTrader with ZorroBridge AddOn" -ForegroundColor Yellow
    
    $continue = Read-Host "Continue anyway? (y/n)"
    if ($continue -ne 'y') {
        exit 1
    }
} else {
    Write-Host "  [OK] NinjaTrader is running" -ForegroundColor Green
}

Write-Host ""

# Step 3: Run Zorro
Write-Host "[3/6] Running Zorro test..." -ForegroundColor Yellow
Write-Host "  Command: zorro.exe -diag -trade $TestScript" -ForegroundColor Gray
Write-Host "  Timeout: $TimeoutSeconds seconds" -ForegroundColor Gray
Write-Host ""

# Start Zorro process
$processStartInfo = New-Object System.Diagnostics.ProcessStartInfo
$processStartInfo.FileName = $ZORRO_EXE
$processStartInfo.Arguments = "-diag -trade $TestScript"  # Changed from -run to -trade
$processStartInfo.WorkingDirectory = $ZORRO_DIR
$processStartInfo.UseShellExecute = $false
$processStartInfo.RedirectStandardOutput = $false
$processStartInfo.RedirectStandardError = $false

$process = [System.Diagnostics.Process]::Start($processStartInfo)
$zorroPid = $process.Id  # Use different variable name to avoid conflict with automatic $pid

Write-Host "  [INFO] Zorro started with PID: $zorroPid" -ForegroundColor Cyan
Write-Host ""

# Step 4: Wait for completion
Write-Host "[4/6] Waiting for Zorro to complete..." -ForegroundColor Yellow

$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$completed = $false
$lastProgressTime = 0

while ($stopwatch.Elapsed.TotalSeconds -lt $TimeoutSeconds) {
    if ($process.HasExited) {
        $elapsed = [math]::Round($stopwatch.Elapsed.TotalSeconds, 1)
        Write-Host "  [OK] Zorro completed after $elapsed seconds" -ForegroundColor Green
        $completed = $true
        break
    }
    
    # Show progress every 5 seconds
    $currentSeconds = [math]::Floor($stopwatch.Elapsed.TotalSeconds)
    if ($currentSeconds -ge ($lastProgressTime + 5) -and $currentSeconds -gt 0) {
        Write-Host "  Still running... ($currentSeconds`s / $TimeoutSeconds`s)" -ForegroundColor Gray
        $lastProgressTime = $currentSeconds
    }
    
    Start-Sleep -Milliseconds 500
}

$stopwatch.Stop()

if (-not $completed) {
    Write-Host "  [TIMEOUT] Zorro still running after $TimeoutSeconds seconds" -ForegroundColor Yellow
    Write-Host "  [INFO] Killing Zorro process..." -ForegroundColor Yellow
    $process.Kill()
    $process.WaitForExit(5000)
}

Write-Host ""

# Step 5: Analyze logs
Write-Host "[5/6] Analyzing results..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if files exist
if (-not (Test-Path $DIAG_FILE)) {
    Write-Host "[ERROR] diag.txt not found!" -ForegroundColor Red
    Write-Host "  Expected: $DIAG_FILE" -ForegroundColor Gray
    exit 1
}

# Find active log file
$activeLog = $null
if (Test-Path $LOG_FILE) {
    $activeLog = $LOG_FILE
} elseif (Test-Path $DEMO_LOG) {
    $activeLog = $DEMO_LOG
} else {
    Write-Host "[ERROR] No log file found!" -ForegroundColor Red
    Write-Host "  Expected: $LOG_FILE" -ForegroundColor Gray
    Write-Host "  Or: $DEMO_LOG" -ForegroundColor Gray
    exit 1
}

Write-Host "[LOG ANALYSIS]" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Analyze diag.txt
Write-Host "--- DIAG.TXT Analysis ---" -ForegroundColor Yellow
Write-Host ""

$diagContent = Get-Content $DIAG_FILE -Raw

# Check for broker calls
$checks = @{
    "BrokerLogin" = "BrokerLogin called"
    "BrokerAsset" = "BrokerAsset called"
    "BrokerBuy2" = "BrokerBuy2 called"
}

$brokerBuy2Called = $false

foreach ($check in $checks.GetEnumerator()) {
    if ($diagContent -like "*$($check.Key)*") {
        Write-Host "[PASS] $($check.Value)" -ForegroundColor Green
        if ($check.Key -eq "BrokerBuy2") {
            $brokerBuy2Called = $true
        }
    } else {
        Write-Host "[FAIL] $($check.Value) NOT called" -ForegroundColor Red
    }
}

# Check for quit
if ($diagContent -like "*quit*") {
    Write-Host "[INFO] Script quit detected" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Quit context from diag.txt:" -ForegroundColor Gray
    Get-Content $DIAG_FILE | Select-String -Pattern "quit" -Context 2,2 | Format-List
}

Write-Host ""
Write-Host "--- LOG FILE Analysis ($TestScript`_*.log) ---" -ForegroundColor Yellow
Write-Host "Using: $(Split-Path $activeLog -Leaf)" -ForegroundColor Gray
Write-Host ""

$logContent = Get-Content $activeLog -Raw

# Check for various indicators
$logChecks = @{
    "DirectBuyTest" = @{ Pattern = "DirectBuyTest"; Message = "INITRUN block executed"; Type = "PASS" }
    "DEBUG" = @{ Pattern = "[DEBUG]"; Message = "DEBUG output found"; Type = "PASS" }
    "MarketData" = @{ Pattern = "Market data"; Message = "State machine executed"; Type = "PASS" }
    "Warning054" = @{ Pattern = "Warning 054"; Message = "Warning 054 detected"; Type = "WARN" }
    "TestComplete" = @{ Pattern = "Test complete"; Message = "Script completed normally"; Type = "INFO" }
}

foreach ($check in $logChecks.GetEnumerator()) {
    $pattern = $check.Value.Pattern
    $message = $check.Value.Message
    $type = $check.Value.Type
    
    if ($logContent -like "*$pattern*") {
        switch ($type) {
            "PASS" { Write-Host "[PASS] $message" -ForegroundColor Green }
            "WARN" { Write-Host "[WARN] $message" -ForegroundColor Yellow }
            "INFO" { Write-Host "[INFO] $message" -ForegroundColor Cyan }
        }
        
        # Show relevant lines
        if ($check.Key -eq "DEBUG") {
            Write-Host ""
            Write-Host "Debug lines from log:" -ForegroundColor Gray
            Get-Content $activeLog | Select-String -Pattern "\[DEBUG\]" | ForEach-Object {
                Write-Host "  $_" -ForegroundColor DarkGray
            }
        }
        
        if ($check.Key -eq "Warning054") {
            Get-Content $activeLog | Select-String -Pattern "Warning 054" | ForEach-Object {
                Write-Host "  $_" -ForegroundColor DarkYellow
            }
        }
    } else {
        if ($type -eq "PASS") {
            Write-Host "[FAIL] $message NOT found" -ForegroundColor Red
        }
    }
}

# Check for Data/Algos/Trades line
$dataLine = Get-Content $activeLog | Select-String -Pattern "Data.*Algos.*Trades"
if ($dataLine) {
    Write-Host ""
    Write-Host ".trd file info:" -ForegroundColor Gray
    Write-Host "  $dataLine" -ForegroundColor DarkGray
}

Write-Host ""

# Check BrokerAsset debug log
if (Test-Path $ASSET_DEBUG) {
    Write-Host "--- BROKERASSET_DEBUG.LOG Analysis ---" -ForegroundColor Yellow
    Write-Host ""
    
    $assetContent = Get-Content $ASSET_DEBUG -Raw
    
    if ($assetContent -like "*SUBSCRIBE MODE*") {
        Write-Host "[INFO] BrokerAsset subscribe mode detected" -ForegroundColor Cyan
    }
    
    if ($assetContent -like "*QUERY MODE*") {
        Write-Host "[INFO] BrokerAsset query mode detected" -ForegroundColor Cyan
    }
    
    if ($assetContent -like "*WRITING pPip*") {
        Write-Host "[INFO] Contract specs written" -ForegroundColor Cyan
    }
    
    Write-Host ""
}

# Step 6: Final summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "[6/6] FINAL SUMMARY" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if ($brokerBuy2Called) {
    Write-Host "[SUCCESS] BrokerBuy2 was called!" -ForegroundColor Green
    Write-Host "  The plugin function executed successfully." -ForegroundColor Green
    Write-Host "  Check diag.txt for full details." -ForegroundColor Gray
} else {
    Write-Host "[FAILURE] BrokerBuy2 was NOT called!" -ForegroundColor Red
    Write-Host "  The script quit before placing orders." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "POSSIBLE CAUSES:" -ForegroundColor Yellow
    Write-Host "  1. Script did not recompile (run with -DeleteCache)" -ForegroundColor Gray
    Write-Host "  2. .trd file replay advancing state machine" -ForegroundColor Gray
    Write-Host "  3. Global variable persistence across runs" -ForegroundColor Gray
    Write-Host ""
    Write-Host "NEXT STEPS:" -ForegroundColor Yellow
    Write-Host "  1. Delete compiled cache and retry:" -ForegroundColor Gray
    Write-Host "     .\automated-zorro-test.ps1 -DeleteCache" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  2. Check last 30 lines of log:" -ForegroundColor Gray
    Write-Host "     Get-Content '$activeLog' -Tail 30" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  3. Check for quit in diag.txt:" -ForegroundColor Gray
    Write-Host "     Get-Content '$DIAG_FILE' | Select-String -Pattern 'quit|BrokerBuy2' -Context 2,2" -ForegroundColor DarkGray
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test completed at $(Get-Date -Format 'HH:mm:ss')" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Auto-open logs if -OpenLogs flag is set
if ($OpenLogs) {
    Write-Host "Opening log files..." -ForegroundColor Gray
    Start-Process notepad $activeLog
    Start-Process notepad $DIAG_FILE
}

# Return exit code based on success
if ($brokerBuy2Called) {
    exit 0
} else {
    exit 1
}
