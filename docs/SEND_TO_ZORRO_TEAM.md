# Files to Send to Zorro Team

## Summary
NinjaTrader 8 broker plugin successfully connects and receives data, but test scripts quit immediately without executing trade logic.

---

## ?? Files to Include

### 1. Analysis Document ? **START HERE**
**File:** `docs\ZORRO_TEAM_ANALYSIS.md`  
**Description:** Comprehensive analysis of the issue with evidence, theories, and questions

### 2. Test Script Source
**File:** `zorro\Strategy\DirectBuyTest.c`  
**Description:** Lite-C script that should place a market order but quits early

### 3. Log Files
**Files:**
- `zorro\Log\DirectBuyTest_demo.log` - Script execution log
- `zorro\diag.txt` - Diagnostic output (search for "quit")

### 4. Configuration Files
**Files:**
- `zorro\History\AssetsFix.csv` - Asset configuration
- `zorro\History\Accounts.csv` - Account configuration

### 5. Trade Data (Optional)
**File:** `zorro\Data\DirectBuyTest_d.trd` - Contains 12 data points that might cause replay

---

## ?? Email Template

```
Subject: Zorro 2.70 Script Quits Immediately - Need Guidance

Dear Zorro Team,

I'm developing a NinjaTrader 8 broker plugin and encountering an issue where 
test scripts quit immediately during initialization without executing trade logic.

**The Issue:**
- Plugin connects successfully to broker ?
- Script loads and initializes ?  
- Market data is received ?
- But script quits with "Test complete" before placing any trades ?

**Evidence:**
Log shows: "Data 12  Algos 2  Trades 0" followed by immediate quit.

I believe Zorro may be replaying the .trd file during initialization, causing 
the state machine to advance prematurely.

**Attached Files:**
1. ZORRO_TEAM_ANALYSIS.md - Detailed analysis with evidence
2. DirectBuyTest.c - Test script source
3. DirectBuyTest_demo.log - Execution log
4. diag.txt - Diagnostic output

**Questions:**
1. Does Zorro replay .trd bars during initialization?
2. How to write a test script that executes only in live mode?
3. Why don't printf() statements appear in the log?

I've spent considerable time investigating and would greatly appreciate 
guidance on the proper script structure for live testing.

Thank you for your assistance.

Best regards,
[Your Name]
```

---

## ?? Key Evidence to Highlight

### In Email Body
Point the Zorro team to these specific log lines:

1. **From DirectBuyTest_demo.log:**
```
Data 12  Algos 2  Trades 0 (V2.700)
Test complete
```

2. **From diag.txt:**
```
quit(Test complete)
!  V  .3f on  s
!  LookBack set to  i bars
```

3. **Script behavior:**
- INITRUN output appears (printf from initialization block)
- NO state machine output appears (printf from bar logic)
- Immediate quit before live trading begins

---

## ?? How to Package

### Option 1: ZIP Archive
```powershell
# Create archive with all files
$files = @(
    "docs\ZORRO_TEAM_ANALYSIS.md",
    "zorro\Strategy\DirectBuyTest.c",
    "zorro\Log\DirectBuyTest_demo.log",
    "zorro\diag.txt",
    "zorro\History\AssetsFix.csv",
    "zorro\History\Accounts.csv"
)

Compress-Archive -Path $files -DestinationPath "ZorroIssue_DirectBuyTest.zip"
```

### Option 2: Email Individual Files
Attach files in this order:
1. ZORRO_TEAM_ANALYSIS.md (most important)
2. DirectBuyTest.c
3. DirectBuyTest_demo.log
4. diag.txt
5. AssetsFix.csv (if needed)

---

## ? Expected Response

The Zorro team should clarify:

1. **If .trd replay is the issue:**
   - How to detect LOOKBACK vs live mode
   - Proper use of `is(LOOKBACK)` flag
   - How to skip state machine during replay

2. **If compiled script caching is the issue:**
   - Where .zc files are stored
   - How to force recompilation
   - Why AutoCompile = 0 doesn't always recompile

3. **Recommended script pattern:**
   - Template for "wait N bars then trade" test
   - Proper handling of INITRUN vs bar processing
   - How global variables persist across runs

---

## ?? Goal

Get a working example from Zorro team showing:
```c
// Working pattern for live-only test script
function run()
{
    if(is(INITRUN)) {
        // Setup
        return;
    }
    
    if(is(LOOKBACK)) {
        // Skip historical replay?
        return;
    }
    
    // Live bar processing - how to count bars?
    // How to place order on bar 3?
}
```

---

**Status:** Ready to send to Zorro team  
**Priority:** HIGH - Blocking plugin testing  
**Next Step:** Email files and await response
