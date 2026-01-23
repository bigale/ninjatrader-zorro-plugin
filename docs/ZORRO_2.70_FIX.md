# Zorro 2.70 Debugging - Process Documentation

## Current Status

**FINAL STATUS - Sending to Zorro Team for Analysis**

### Problem Summary
DirectBuyTest.c script connects successfully to NinjaTrader but **quits immediately during initialization** without executing any trade logic.

**What Works:**
- ? BrokerLogin (connects to Sim101)
- ? BrokerAsset (subscribes to MES 03-26, receives prices)
- ? BrokerAccount (retrieves account data)
- ? TCP communication (plugin ? AddOn)

**What Doesn't Work:**
- ? Script bar-by-bar logic never executes
- ? printf() statements don't appear in log (except INITRUN)
- ? enterLong() never called
- ? BrokerBuy2() never called

### Root Cause (Most Likely)
**Zorro replays .trd file (12 bars) during initialization**, advancing the state machine with each bar. By the time live trading starts, the script is already in state 3 (quit state).

**Evidence:**
- Log shows: `Data 12  Algos 2  Trades 0`
- Log shows: `quit(Test complete)` during init phase
- Debug printf() exists in source but doesn't appear in log
- Only INITRUN output appears

### Files for Zorro Team
?? **Comprehensive analysis:** `docs\ZORRO_TEAM_ANALYSIS.md`  
?? **Script source:** `zorro\Strategy\DirectBuyTest.c`  
?? **Test logs:** `zorro\Log\DirectBuyTest_demo.log`  
?? **Diagnostic log:** `zorro\diag.txt`

---

## Action Plan

1. ? Fix DirectBuyTest.c to use state machine instead of NumBars
2. ? **Copy fixed script to `zorro\Strategy\` folder**
3. ? **Add debug logging to see when run() is called**
4. ? **CREATE AUTOMATED TEST SCRIPT** ?? **NEW!**
5. ? Run automated test and interpret results
6. ? If BrokerBuy2() is called: Success!
7. ? If NOT called: Use automated test output for diagnosis

**AUTOMATED TESTING NOW AVAILABLE!** ??

See: `docs\AUTOMATED_TESTING.md` for complete guide

---

## Quick Test Commands

### PowerShell (Recommended)
```powershell
# Run automated test (handles everything!)
.\automated-zorro-test.ps1 -DeleteCache -OpenLogs

# This will:
# - Delete old logs
# - Delete compiled cache (fixes recompilation issues)
# - Run Zorro test
# - Analyze output automatically
# - Report if BrokerBuy2 was called
# - Open logs for inspection
```

### Batch File
```cmd
automated-zorro-test.bat
```

### Manual Testing (Old Way)
```powershell
# 0. **CRITICAL**: Copy fixed script to Zorro Strategy folder!
Copy-Item "private-scripts\DirectBuyTest.c" "zorro\Strategy\DirectBuyTest.c" -Force

# 1. Close Zorro 2.70 completely

# 2. Rebuild plugin with latest fixes (optional if no plugin changes)
# .\rebuild-debug.bat

# 3. Start NinjaTrader (if not running)

# 4. Start Zorro 2.70

# 5. Load DirectBuyTest in Zorro
# 6. Select NT8-Sim account
# 7. Click Trade
# 8. Watch console for detailed output

# 9. After test completes, capture all logs:
Get-Content "zorro\diag.txt" | Select-String -Pattern "BrokerBuy2"
Get-Content "zorro\Log\DirectBuyTest_demo.log" -Tail 30
Get-Content "zorro\Log\BrokerAsset_debug.log"
```

---
