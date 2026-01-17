# Zorro 2.70 vs 2.66 Debugging

## Problem Summary

**Zorro 2.70** does NOT send orders to NinjaTrader, while **Zorro 2.66** works fine with the same plugin.

## Diagnostic File Analysis

### Working (Zorro 2.66)
```
Call(1,1,0.0000)
```
- Clean call execution
- No warnings
- Orders reach NinjaTrader

### NOT Working (Zorro 2.70)
```
Call(1,1,0.0000)
...
Call(1,3,0.0000)
!  Warning 073 ( s  s): Can't close  d of  d in  d trades
```

**Warning 073** means: "Can't close X of Y in Z trades"
- Zorro tried to close a position
- But the position doesn't exist (or has wrong quantity)

## Root Cause

**Zorro 2.70 thinks a position is open when it's not.**

The sequence:
1. `Call(1,1,0.0000)` - User clicks "Buy Long"
2. Plugin calls `BrokerBuy2` - Should place order
3. **PROBLEM:** Zorro thinks order filled, but it didn't
4. `Call(1,3,0.0000)` - Zorro tries to close non-existent position
5. **Warning 073** - Can't close what doesn't exist

## Likely Causes

### 1. BrokerBuy2 Return Value Issue

**Your plugin currently returns:**
```cpp
// For market orders that fill immediately
if (filled > 0) {
    return numericId;  // Positive = filled
}

// For pending orders (limit/stop)
return -numericId;  // Negative = pending
```

**Zorro 2.70 might interpret this differently than 2.66!**

### 2. Position Tracking Issue

**Your plugin uses cached positions:**
```cpp
case GET_POSITION:
    int cachedPosition = g_state.positions[symbol];
    return (double)abs(cachedPosition);  // Returns absolute value
```

**Zorro 2.70 might be querying position BEFORE it updates the cache.**

### 3. Order ID Tracking Issue

**Zorro might be:**
- Assigning trade IDs differently in 2.70
- Not recognizing pending orders the same way
- Expecting different return codes from `BrokerBuy2`

## Recommended Debugging Steps

### Step 1: Add Plugin Logging

Add this to `BrokerBuy2` in `NT8Plugin.cpp`:

```cpp
DLLFUNC int BrokerBuy2(char* Asset, int Amount, double StopDist, double Limit,
    double* pPrice, int* pFill)
{
    LogMessage("# BrokerBuy2 CALLED: Asset=%s Amount=%d StopDist=%.2f Limit=%.2f",
        Asset, Amount, StopDist, Limit);
    
    // ... existing code ...
    
    // Before return
    LogMessage("# BrokerBuy2 RETURNING: ID=%d filled=%d price=%.2f",
        returnValue, filled, fillPrice);
    
    return returnValue;
}
```

### Step 2: Add Position Logging

Add to `GET_POSITION` case:

```cpp
case GET_POSITION: {
    const char* symbol = (const char*)dwParameter;
    int cachedPos = g_state.positions[symbol];
    
    // Query actual position from NT8
    int ntPos = g_bridge->MarketPosition(symbol, g_state.account.c_str());
    
    LogMessage("# GET_POSITION: symbol=%s cached=%d actual=%d", 
        symbol, cachedPos, ntPos);
    
    // IMPORTANT: Return actual position, not cached!
    return (double)abs(ntPos);
}
```

### Step 3: Test with Diagnostic Script

Create a simpler test script:

```c
// SimpleOrderTest.c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 2);
        asset("MESH26");
        printf("\n=== Simple Order Test ===\n");
    }
    
    // Wait for market data
    if(NumBars < 5) return;
    
    // Single test: Buy 1 contract
    if(NumBars == 5) {
        printf("\n[TEST] Placing BUY order...");
        int id = enterLong(1);
        printf("\n[TEST] enterLong returned: %d", id);
    }
    
    // Check position
    if(NumBars == 10) {
        int pos = brokerCommand(GET_POSITION, (long)Asset);
        printf("\n[TEST] Position: %d contracts", pos);
        printf("\n[TEST] NumOpenLong: %d", NumOpenLong);
        
        if(NumOpenLong > 0) {
            printf("\n[TEST] Closing position...");
            exitLong();
        }
    }
    
    if(NumBars > 15) {
        quit("Test complete");
    }
}
```

### Step 4: Compare Plugin Behavior

Run this test in **BOTH** Zorro versions and compare the debug output.

## Key Questions to Answer

1. **Is `BrokerBuy2` even being called in Zorro 2.70?**
   - If YES: What's it returning?
   - If NO: Why is Zorro not calling it?

2. **What does `GET_POSITION` return?**
   - Before order: Should be 0
   - After order fills: Should be 1
   - After close: Should be 0

3. **Are the order IDs consistent?**
   - Positive ID = filled
   - Negative ID = pending
   - Zero = failed

## Expected vs Actual

### Expected Flow (2.66)
1. User clicks "Buy Long"
2. Zorro calls `BrokerBuy2(Asset, 1, 0, 0, &price, &fill)`
3. Plugin places MARKET order in NT8
4. Order fills immediately
5. Plugin returns **positive ID** (e.g., `42`)
6. Zorro queries `GET_POSITION` ? returns `1`
7. User clicks "Close Long"
8. Zorro calls `BrokerSell2(42, 1, 0, ...)`
9. Plugin closes position
10. Success!

### Actual Flow (2.70)
1. User clicks "Buy Long"
2. **UNKNOWN** - Is `BrokerBuy2` called?
3. Zorro immediately tries to close
4. **Warning 073** - No position to close

## Potential Fixes

### Fix 1: Return Positive ID Only When FILLED

```cpp
// In BrokerBuy2
if (strcmp(orderType, "MARKET") == 0) {
    // Wait for fill (up to 1 second)
    for (int i = 0; i < 10; i++) {
        Sleep(100);
        int filled = g_bridge->Filled(ntOrderId);
        if (filled > 0) {
            *pPrice = fillPrice;
            *pFill = filled;
            
            LogMessage("# Market order FILLED: ID=%d qty=%d price=%.2f",
                numericId, filled, fillPrice);
            
            return numericId;  // ? Positive = filled
        }
    }
    
    // TIMEOUT - order didn't fill
    LogMessage("# Market order TIMEOUT: ID=%d", numericId);
    
    // Zorro 2.70 might need this to return 0 instead of negative
    return 0;  // ? Failed to fill
}
```

### Fix 2: Always Query Actual Position (Don't Cache)

```cpp
case GET_POSITION: {
    const char* symbol = (const char*)dwParameter;
    
    // ALWAYS query NT8 for real position
    int ntPosition = g_bridge->MarketPosition(symbol, g_state.account.c_str());
    
    // Update cache to match reality
    g_state.positions[symbol] = ntPosition;
    
    LogMessage("# GET_POSITION: %s = %d contracts", symbol, ntPosition);
    
    return (double)abs(ntPosition);
}
```

### Fix 3: Check Zorro Version in Plugin

```cpp
DLLFUNC int BrokerOpen(char* Name, FARPROC fpMessage, FARPROC fpProgress)
{
    // ... existing code ...
    
    // Check Zorro version
    double version = brokerCommand(GET_VERSION, 0);
    LogMessage("# Zorro version: %.2f", version);
    
    if (version >= 2.70) {
        LogMessage("# Using Zorro 2.70+ compatibility mode");
        g_state.zorro270Mode = true;
    }
    
    return PLUGIN_VERSION;
}
```

## Next Steps

1. **Enable plugin logging** (SET_DIAGNOSTICS)
2. **Add debug output** to BrokerBuy2 and GET_POSITION
3. **Run SimpleOrderTest.c** in BOTH Zorro versions
4. **Compare the logs** to see where they diverge
5. **Report findings** back for further analysis

## Expected Output

When working correctly, you should see:

```
# BrokerBuy2 CALLED: Asset=MES 03-26 Amount=1 StopDist=0.00 Limit=0.00
[TcpBridge] Sending: PLACEORDER:BUY:MES 03-26:1:MARKET:0.00
[TcpBridge] Response: ORDER:NT12345
# BrokerBuy2 RETURNING: ID=42 filled=1 price=6989.50

# GET_POSITION: symbol=MES 03-26 cached=1 actual=1
```

If `BrokerBuy2` is NOT being called, that's a **Zorro 2.70 issue**, not a plugin issue!

---

**Created:** 2026-01-17  
**Status:** Debugging in progress  
**Priority:** HIGH - Blocking Zorro 2.70 compatibility
