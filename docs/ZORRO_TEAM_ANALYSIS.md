# Zorro 2.70 Debugging - Final Analysis for Zorro Team

**Date:** 2026-01-21  
**Plugin:** NT8.dll v1.0.0  
**Zorro Version:** 2.70  
**Issue:** Script quits immediately without executing trade logic

---

## Executive Summary

### Problem Statement
DirectBuyTest.c script connects successfully to NinjaTrader but **quits immediately with "Test complete"** without ever calling `enterLong()` or `BrokerBuy2()`.

### Evidence
All three key indicators work correctly:
- ? **BrokerLogin** succeeds (connects to Sim101 account)
- ? **BrokerAsset** succeeds (subscribes to MES 03-26, receives price data)
- ? **BrokerAccount** succeeds (retrieves account balance)

But:
- ? **Script bar-by-bar logic NEVER executes**
- ? **No printf() output from state machine code**
- ? **Quits during initialization phase**

---

## Detailed Findings

### 1. Log Analysis

**DirectBuyTest_demo.log** shows:
```
=== DirectBuyTest - Testing BrokerBuy2 Directly ===  ? INITRUN block
Asset: MES 03-26
Waiting for market data...

V 2.700 on Wed 26-01-21 08:51:04
LookBack set to 0 bars
MES 03-26 6822.25000 0.25000  0.000 0.000  0.25000 5.00000 25.0000  1.0 203678.0

Trade: DirectBuyTest MES 03-26 2026-01-21
Loading Data\DirectBuyTest_d.trd
Data 12  Algos 2  Trades 0 (V2.700)  ? Key line!
Test complete  ? Immediate quit
Logout.. ok
```

**Key observation:** `Data 12  Algos 2  Trades 0`
- **Data 12** - Zorro loaded 12 data points from `.trd` file
- **Algos 2** - Detected 2 algorithms (possibly from previous runs)
- **Trades 0** - No trades in saved data

**diag.txt** shows:
```
quit(Test complete)  ? Happens during initialization
!  V  .3f on  s
!  LookBack set to  i bars
```

The quit happens BEFORE any bar-by-bar processing begins.

---

### 2. Script Structure

**DirectBuyTest.c** uses a state machine:
```c
int g_TestPhase = 0;  // Global state variable

function run()
{
    // DEBUG logging at entry (SHOULD appear in log)
    printf("\n[DEBUG] run() called - Bar:%d Phase:%d...", ...);
    
    if(is(INITRUN)) {
        // Initialization (DOES appear in log)
        printf("\n=== DirectBuyTest - Testing BrokerBuy2 Directly ===");
    }
    
    // State 0: Wait for price data
    if(g_TestPhase == 0) {
        // SHOULD appear but DOESN'T
        printf("\nBar %d: Market data received...");
    }
    
    // State 1: Place order
    else if(g_TestPhase == 1) {
        // SHOULD appear but DOESN'T
        printf("\nBar %d: PLACING ORDER");
        enterLong(1);
    }
    
    // State 3: Quit
    else if(g_TestPhase == 3) {
        printf("\n[DEBUG] About to quit...");  // SHOULD appear but DOESN'T
        quit("Test complete");
    }
}
```

**Problem:** NONE of the state machine printf() statements appear in the log, yet the script quits with "Test complete" (from state 3).

---

### 3. Theories Investigated

#### Theory 1: NumBars Confusion ?
**Original thought:** Script checks `NumBars == 5` and quits if true.  
**Fixed:** Changed to state machine using `g_TestPhase`.  
**Result:** Still quits immediately.

#### Theory 2: Wrong File Location ?
**Original thought:** Editing wrong file location.  
**Fixed:** Copied to `zorro\Strategy\DirectBuyTest.c`.  
**Result:** Still quits immediately.

#### Theory 3: Zorro Caches Compiled Scripts ??
**New theory:** Zorro compiles scripts to `.zc` files and may not recompile on changes.  
**Evidence:** 
- Debug printf() exists in source file
- Debug printf() does NOT appear in log
- This suggests Zorro is running OLD compiled version

**AutoCompile setting in Zorro.ini:**
```ini
AutoCompile = 0 // 0 = compile always, 1 = compile only modified scripts
```

Setting is 0 (compile always), yet script doesn't recompile!

#### Theory 4: .trd File Replay ?? **MOST LIKELY**
**New theory:** Zorro replays the 12 bars from `.trd` file during initialization, advancing `g_TestPhase` with each bar.  
**Evidence:**
- Log shows `Data 12  Algos 2  Trades 0`
- Each bar calls `run()` ? advances state
- After 12 bars: `g_TestPhase == 3` ? quit
- By the time live trading starts, we're already in quit state!

**Critical flaw in state machine design:**
```c
// State 0: First call
g_TestPhase = 0;

// State 0: Sees price, advances to state 1
g_TestPhase = 1;

// State 1: Places order, advances to state 2
g_TestPhase = 2;

// State 2: Checks status, advances to state 3
g_TestPhase = 3;

// State 3: QUITS
quit("Test complete");
```

If `run()` is called 12 times during `.trd` replay:
- Call 1: State 0 ? 1
- Call 2: State 1 ? 2  
- Call 3: State 2 ? 3
- Call 4-12: State 3 ? QUIT!

---

### 4. Why Debug Printf Doesn't Appear

**Two possibilities:**

1. **Zorro script compiler issue**
   - `.zc` file not regenerated
   - Old compiled version still runs
   - Need to manually delete `.zc` files

2. **Printf suppression during .trd replay**
   - Zorro might suppress output during historical replay
   - Only shows INITRUN output
   - Bar-by-bar output hidden

---

## Recommended Solutions for Zorro Team

### Solution 1: Delete Compiled Cache
```bash
# Force recompilation
Remove-Item "zorro\Strategy\*.zc" -Force
```

### Solution 2: Fix State Machine Design
**Problem:** Global `g_TestPhase` persists across ALL `run()` calls, including `.trd` replay.

**Fix:** Only advance states during LIVE trading:
```c
function run()
{
    if(is(INITRUN)) {
        g_TestPhase = 0;  // Reset on init
        return;
    }
    
    // CRITICAL: Only execute in LIVE mode
    if(is(LOOKBACK)) {
        return;  // Skip historical replay
    }
    
    // Now safe to use state machine
    if(g_TestPhase == 0) {
        // Wait for price...
    }
}
```

### Solution 3: Use Bar Counter Instead
```c
// Instead of persistent state variable
static int g_BarsSinceStart = 0;

function run()
{
    if(is(INITRUN)) {
        g_BarsSinceStart = 0;
        return;
    }
    
    if(is(LOOKBACK)) return;  // Skip historical
    
    g_BarsSinceStart++;
    
    if(g_BarsSinceStart == 1) {
        // First live bar
        printf("\nFirst bar, waiting...");
    }
    else if(g_BarsSinceStart == 3) {
        // Third live bar
        printf("\nPlacing order...");
        enterLong(1);
    }
}
```

### Solution 4: Disable SaveMode
```c
// At top of script
SaveMode = 0;  // Don't save/restore state
```

---

## Files for Zorro Team Analysis

### 1. DirectBuyTest.c (Current Version)
**Location:** `C:\Users\bigal\source\repos\ninjatrader-zorro-plugin\zorro\Strategy\DirectBuyTest.c`

**Key characteristics:**
- Uses state machine with `g_TestPhase`
- Has debug printf() at start of `run()`
- Should print "[DEBUG] run() called..." but doesn't
- Quits via `quit("Test complete")` in state 3

### 2. Log Files
**DirectBuyTest_demo.log:**
- Shows INITRUN output only
- No state machine output
- Immediate "Test complete" quit

**diag.txt:**
- Shows `quit(Test complete)` during init phase
- Shows `Data 12  Algos 2  Trades 0`

### 3. .trd File
**Location:** `zorro\Data\DirectBuyTest_d.trd`

**Contains:** 12 data points that might be causing replay

---

## Questions for Zorro Team

1. **Does Zorro replay .trd file bars during initialization?**
   - If yes, are global variables preserved across these calls?
   - If yes, how to detect replay vs live trading?

2. **Why don't printf() statements appear in log?**
   - Are they suppressed during .trd replay?
   - Do we need `VERBOSE` flag for state machine output?

3. **Script compilation:**
   - Does `AutoCompile = 0` always recompile?
   - Should we manually delete `.zc` files?
   - Where are compiled scripts cached?

4. **Recommended pattern for test scripts:**
   - How to write a simple "wait 3 bars then place order" script?
   - What's the proper way to handle INITRUN vs live bars?

---

## Plugin Status

### What Works ?
- **BrokerLogin** - Connects to NT8 successfully
- **BrokerAsset** - Subscribes and receives prices
- **BrokerAccount** - Retrieves account data
- **TCP Communication** - Plugin ? AddOn works perfectly

### What Doesn't Work ?
- **Trade execution** - Can't test because script quits early
- **BrokerBuy2** - Never gets called due to script issue

### Next Steps After Zorro Team Response
1. Fix script pattern based on guidance
2. Test order placement
3. Verify fills and position tracking
4. Complete full integration test

---

## Contact Information

**Developer:** [Your name/contact]  
**Plugin:** NT8.dll for NinjaTrader 8.1+  
**Repository:** https://github.com/bigale/ninjatrader-zorro-plugin  
**Test Environment:**
- NinjaTrader 8.1+
- Zorro 2.70
- Windows 11
- TCP Bridge Architecture

---

**Status:** Awaiting Zorro team guidance on proper script structure for live testing.
