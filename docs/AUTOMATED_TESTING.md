# Automated Zorro Testing - Quick Reference

## Overview
Two automated test scripts that run Zorro, capture output, and analyze results.

---

## ?? Quick Start

### PowerShell (Recommended)
```powershell
# Basic test
.\automated-zorro-test.ps1

# Delete compiled cache before test (fixes recompilation issues)
.\automated-zorro-test.ps1 -DeleteCache

# Auto-open logs after test
.\automated-zorro-test.ps1 -OpenLogs

# Custom timeout (default 30 seconds)
.\automated-zorro-test.ps1 -TimeoutSeconds 60

# All options combined
.\automated-zorro-test.ps1 -DeleteCache -OpenLogs -TimeoutSeconds 45
```

### Batch File
```cmd
automated-zorro-test.bat
```

---

## ?? What The Scripts Do

### 1. Cleanup Phase
- Deletes old `diag.txt`
- Deletes old log files (`DirectBuyTest_test.log`, `DirectBuyTest_demo.log`)
- Deletes `BrokerAsset_debug.log`
- **Optional:** Deletes `.zc` compiled cache files

### 2. Prerequisites Check
- Verifies Zorro.exe exists
- Checks if NinjaTrader is running
- Warns if NT is not running (allows continuation)

### 3. Execution Phase
- Runs: `zorro.exe -diag -run DirectBuyTest`
- Captures process ID
- Monitors execution
- Enforces timeout (default 30s)
- Kills process if timeout exceeded

### 4. Analysis Phase
Automatically checks for:

**From diag.txt:**
- ? BrokerLogin called
- ? BrokerAsset called  
- ? BrokerBuy2 called ? **KEY INDICATOR**
- ?? quit() location

**From log file:**
- ? INITRUN block executed
- ? DEBUG output present
- ? State machine executed
- ?? Warning 054 present
- ?? .trd file info (Data/Algos/Trades)

**From BrokerAsset_debug.log:**
- ?? Subscribe mode calls
- ?? Query mode calls
- ?? Contract specs written

### 5. Summary Phase
- **SUCCESS:** BrokerBuy2 called ?
- **FAILURE:** BrokerBuy2 NOT called ?
  - Lists possible causes
  - Suggests next steps

### 6. Interactive Phase
- Offers to open log files in Notepad
- Returns exit code (0 = success, 1 = failure)

---

## ?? Success Criteria

The test **PASSES** if:
```
[PASS] BrokerLogin called
[PASS] BrokerAsset called
[PASS] BrokerBuy2 called          ? This is the goal!
[PASS] INITRUN block executed
[PASS] DEBUG output found
[PASS] State machine executed
```

The test **FAILS** if:
```
[FAIL] BrokerBuy2 NOT called      ? Script quit early
[FAIL] DEBUG output NOT found     ? Script didn't recompile
[FAIL] State machine NOT executed ? Bar logic never ran
```

---

## ?? Troubleshooting

### Script Says "BrokerBuy2 NOT called"

**Cause 1: Script didn't recompile**
```powershell
# Solution: Delete cache and retry
.\automated-zorro-test.ps1 -DeleteCache
```

**Cause 2: .trd file replay advancing state**
```powershell
# Solution: Delete .trd file
Remove-Item "zorro\Data\DirectBuyTest_d.trd" -Force
.\automated-zorro-test.ps1
```

**Cause 3: Script has logic error**
```powershell
# Solution: Check last 30 lines of log
Get-Content "zorro\Log\DirectBuyTest_demo.log" -Tail 30
```

### Script Says "DEBUG output NOT found"

**This means Zorro is running OLD compiled version!**

```powershell
# Force recompilation
.\automated-zorro-test.ps1 -DeleteCache

# Or manually delete cache
Remove-Item "zorro\Strategy\*.zc" -Force
```

### Script Times Out

**Zorro is hanging (usually waiting for user input)**

```powershell
# Increase timeout
.\automated-zorro-test.ps1 -TimeoutSeconds 60

# Check if Zorro window is visible (might need manual click)
```

---

## ?? Example Output

### Successful Test
```
========================================
   Automated Zorro Test Runner
========================================

[1/6] Cleaning up old log files...
  - Deleted diag.txt
  - Deleted DirectBuyTest_demo.log

[2/6] Checking prerequisites...
  [OK] NinjaTrader is running

[3/6] Running Zorro test...
  Command: zorro.exe -diag -run DirectBuyTest
  [INFO] Zorro started with PID: 12345

[4/6] Waiting for Zorro to complete...
  [OK] Zorro completed after 5.2 seconds

[5/6] Analyzing results...
========================================

[LOG ANALYSIS]
========================================

--- DIAG.TXT Analysis ---

[PASS] BrokerLogin called
[PASS] BrokerAsset called
[PASS] BrokerBuy2 called          ? SUCCESS!

--- LOG FILE Analysis ---

[PASS] INITRUN block executed
[PASS] DEBUG output found
[PASS] State machine executed

========================================
[6/6] FINAL SUMMARY
========================================

[SUCCESS] BrokerBuy2 was called!
  The plugin function executed successfully.
```

### Failed Test
```
[5/6] Analyzing results...
========================================

--- DIAG.TXT Analysis ---

[PASS] BrokerLogin called
[PASS] BrokerAsset called
[FAIL] BrokerBuy2 NOT called      ? PROBLEM!

[INFO] Script quit detected

--- LOG FILE Analysis ---

[PASS] INITRUN block executed
[FAIL] DEBUG output NOT found     ? Script didn't recompile!

.trd file info:
  Data 12  Algos 2  Trades 0      ? .trd replay issue

========================================
[6/6] FINAL SUMMARY
========================================

[FAILURE] BrokerBuy2 was NOT called!
  The script quit before placing orders.

POSSIBLE CAUSES:
  1. Script did not recompile (run with -DeleteCache)
  2. .trd file replay advancing state machine
  3. Global variable persistence across runs

NEXT STEPS:
  1. Delete compiled cache and retry:
     .\automated-zorro-test.ps1 -DeleteCache
```

---

## ?? Workflow Integration

### Before Sending to Zorro Team
```powershell
# Run automated test to confirm issue
.\automated-zorro-test.ps1 -DeleteCache -OpenLogs

# If BrokerBuy2 NOT called:
# - Capture output
# - Include in bug report
# - Send logs to Zorro team

# If BrokerBuy2 called:
# - Plugin works!
# - Issue was script/cache related
```

### After Code Changes
```powershell
# Rebuild plugin
.\rebuild-debug.bat

# Copy fixed script to Strategy folder
Copy-Item "private-scripts\DirectBuyTest.c" "zorro\Strategy\" -Force

# Run automated test with cache deletion
.\automated-zorro-test.ps1 -DeleteCache
```

---

## ?? Advanced Usage

### Test Different Scripts
```powershell
# Edit the script to change $TestScript variable
# Or modify the script to accept parameter:
.\automated-zorro-test.ps1 -TestScript "MyTestScript"
```

### Continuous Testing
```powershell
# Run test in loop until success
while ($LASTEXITCODE -ne 0) {
    Write-Host "Test failed, retrying in 5 seconds..."
    Start-Sleep 5
    .\automated-zorro-test.ps1 -DeleteCache
}
```

### Log Collection
```powershell
# Collect all logs for bug report
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$logDir = "bug-report-$timestamp"

New-Item -ItemType Directory -Path $logDir
Copy-Item "zorro\diag.txt" "$logDir\" -ErrorAction SilentlyContinue
Copy-Item "zorro\Log\DirectBuyTest_*.log" "$logDir\" -ErrorAction SilentlyContinue
Copy-Item "zorro\Log\BrokerAsset_debug.log" "$logDir\" -ErrorAction SilentlyContinue
Copy-Item "zorro\Strategy\DirectBuyTest.c" "$logDir\"

Compress-Archive -Path $logDir -DestinationPath "$logDir.zip"
Write-Host "Logs collected in: $logDir.zip"
```

---

## ? Benefits

1. **Automated** - No manual log checking
2. **Consistent** - Same tests every time
3. **Fast** - Completes in seconds
4. **Clear** - Color-coded output
5. **Actionable** - Suggests next steps
6. **Reusable** - Works for any test script

---

**Status:** Ready to use  
**Location:** Repository root  
**Usage:** Run before/after code changes or when debugging
