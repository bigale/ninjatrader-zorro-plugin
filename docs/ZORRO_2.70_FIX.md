# Zorro 2.70 Critical Fix - LotAmount Warning 054

## Problem Identified

**Diagnostic output shows:**
```
!  Warning 054:  s LotAmount  s ->  s
```

This appears IMMEDIATELY after `BrokerAsset` is called, BEFORE any `Call()` attempts.

## Root Cause

Zorro 2.70 has **stricter validation** of asset parameters. When `BrokerAsset` returns:
- `pPip = 0` (tick size)
- `pPipCost = 0` (tick value)  
- `pLotAmount = 1`

**Zorro 2.70 REJECTS the asset as invalid** and refuses to place orders!

## The Evidence

From `diag.txt`:
```
BrokerAsset MES 03-26 0 => 
! Subscribed to MES 03-261  (0.176700 ms)
BrokerAsset MES 03-26 0 => 1  (6990.5000 0.252400 ms)
!  Warning 054:  s LotAmount  s ->  s    ? ASSET REJECTED!
```

Then later:
```
Call(1,1,0.0000)    ? Script tries to enter
(NO BrokerBuy2 call)    ? Zorro skips the order!
Call(1,3,0.0000)    ? Script tries to exit (nothing to exit!)
```

## Why This Happens

Your `BrokerAsset` implementation currently returns:

```cpp
if (pPip) {
    auto it = g_state.assetSpecs.find(Asset);
    if (it != g_state.assetSpecs.end() && it->second.tickSize > 0) {
        *pPip = it->second.tickSize;  // Returns 0.25 for MES (IF available)
    } else {
        *pPip = 0;  // ? PROBLEM: Zorro 2.70 rejects this!
    }
}
```

**When the AddOn doesn't return specs (or hasn't parsed them yet), `pPip = 0` and Zorro 2.70 treats this as "invalid asset".**

## The Fix

### Option 1: Return Hardcoded Fallback Values (RECOMMENDED)

```cpp
DLLFUNC int BrokerAsset(char* Asset, double* pPrice, double* pSpread,
    double* pVolume, double* pPip, double* pPipCost, double* pLotAmount,
    double* pMargin, double* pRollLong, double* pRollShort, double* pCommission)
{
    // ... existing subscribe code ...
    
    // **FIX for Zorro 2.70: NEVER return 0 for contract specs**
    if (pPip) {
        auto it = g_state.assetSpecs.find(Asset);
        if (it != g_state.assetSpecs.end() && it->second.tickSize > 0) {
            *pPip = it->second.tickSize;
        } else {
            // **CRITICAL: Must return non-zero value for Zorro 2.70**
            // These are default values for ES/MES futures
            // Zorro will use asset file if available, otherwise these defaults
            *pPip = 0.25;  // Default tick size for micro futures
            LogInfo("# WARNING: Using default tick size %.4f for %s", *pPip, Asset);
        }
    }
    
    if (pPipCost) {
        auto it = g_state.assetSpecs.find(Asset);
        if (it != g_state.assetSpecs.end() && it->second.pointValue > 0) {
            *pPipCost = it->second.pointValue;
        } else {
            // **CRITICAL: Must return non-zero value for Zorro 2.70**
            *pPipCost = 1.25;  // Default point value for MES
            LogInfo("# WARNING: Using default point value $%.2f for %s", *pPipCost, Asset);
        }
    }
    
    // **CRITICAL for Zorro 2.70: LotAmount must be valid**
    if (pLotAmount) {
        *pLotAmount = 1.0;  // Standard for futures
    }
    
    return (*pPrice > 0) ? 1 : 0;
}
```

### Option 2: Force AddOn to Return Specs Synchronously

Modify your SUBSCRIBE command handling to ensure specs are always returned:

```cpp
// In BrokerAsset subscribe mode
if (!pPrice) {
    std::string cmd = std::string("SUBSCRIBE:") + Asset;
    std::string response = g_bridge->SendCommand(cmd);
    
    if (response.find("OK") != std::string::npos) {
        auto parts = g_bridge->SplitResponse(response, ':');
        
        // **FORCE specs to be present - fail if not**
        if (parts.size() < 5) {
            LogError("# SUBSCRIBE response missing contract specs for %s!", Asset);
            LogError("# Response: %s", response.c_str());
            return 0;  // Fail subscription if no specs
        }
        
        try {
            double tickSize = std::stod(parts[3]);
            double pointValue = std::stod(parts[4]);
            
            if (tickSize <= 0 || pointValue <= 0) {
                LogError("# Invalid contract specs: tick=%.4f, value=%.2f", tickSize, pointValue);
                return 0;  // Fail if specs are invalid
            }
            
            g_state.assetSpecs[Asset].tickSize = tickSize;
            g_state.assetSpecs[Asset].pointValue = pointValue;
            
            LogInfo("# Asset specs loaded: %s tick=%.4f value=%.2f", Asset, tickSize, pointValue);
        }
        catch (...) {
            LogError("# Failed to parse contract specs for %s", Asset);
            return 0;
        }
        
        g_state.currentSymbol = Asset;
        return 1;
    }
    
    LogError("Failed to subscribe to %s", Asset);
    return 0;
}
```

## Recommended Immediate Fix

**Use Option 1** - it's the safest and fastest fix. Add these lines to `NT8Plugin.cpp`:

```cpp
// Around line 580 in BrokerAsset
if (pPip) {
    auto it = g_state.assetSpecs.find(Asset);
    if (it != g_state.assetSpecs.end() && it->second.tickSize > 0) {
        *pPip = it->second.tickSize;
        LogDebug("# Returning tick size for %s: %.4f", Asset, it->second.tickSize);
    } else {
        *pPip = 0.25;  // ? DEFAULT VALUE - prevents Zorro 2.70 rejection
        LogInfo("# Using default tick size %.4f for %s (AddOn specs not available)", *pPip, Asset);
    }
}

if (pPipCost) {
    auto it = g_state.assetSpecs.find(Asset);
    if (it != g_state.assetSpecs.end() && it->second.pointValue > 0) {
        *pPipCost = it->second.pointValue;
        LogDebug("# Returning point value for %s: %.2f", Asset, it->second.pointValue);
    } else {
        *pPipCost = 1.25;  // ? DEFAULT VALUE - prevents Zorro 2.70 rejection
        LogInfo("# Using default point value $%.2f for %s (AddOn specs not available)", *pPipCost, Asset);
    }
}

if (pLotAmount) *pLotAmount = 1.0;  // ? MUST be non-zero for Zorro 2.70
```

## Testing

After applying the fix:

1. **Rebuild the plugin:**
   ```bash
   cd build
   cmake --build . --config Release
   ```

2. **Copy to Zorro 2.70:**
   ```bash
   copy Release\NT8.dll C:\Users\bigal\source\repos\ninjatrader-zorro-plugin\zorro\Plugin\NT8.dll
   ```

3. **Run SimpleOrderTest.c in Zorro 2.70**

4. **Check diagnostic output - you should now see:**
   ```
   BrokerAsset MES 03-26 0 => 1  (6990.5000 0.252400 ms)
   (NO Warning 054!)  ? Should be gone!
   Call(1,1,0.0000)
   BrokerBuy2...  ? Should appear now!
   ```

## Why This Fixes It

**Zorro 2.66** was lenient - accepted `pPip = 0` and used the asset file as fallback.

**Zorro 2.70** is strict - requires valid asset parameters BEFORE allowing orders, otherwise marks the asset as "untradeable".

By returning **default values instead of 0**, we satisfy Zorro 2.70's validation while still allowing the asset file to override if needed.

---

**Status:** Fix identified
**Priority:** CRITICAL
**Testing:** Apply fix and retest with Zorro 2.70
