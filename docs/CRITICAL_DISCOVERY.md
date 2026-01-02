# CRITICAL DISCOVERY - Zorro 2.66 vs 2.70 Account Format

## The Problem Found

Orders weren't executing in Zorro 2.70, but the same plugin works in Zorro 2.66.

## Root Cause

**Accounts.csv format difference between what works and what doesn't:**

### ? What DOESN'T Work (Our Initial Config)
```csv
NT8-Sim,NT8,0,Sim101,,Assets,USD,0,1,NT8.dll
                        ^^^^^^              ^^^^^^^
                        No .csv         With .dll extension
```

### ? What WORKS (Zorro 2.66 Format)
```csv
NT8-Sim,NT8,0,Sim101,,AssetsRithmic.csv,USD,0,1,NT8
                        ^^^^^^^^^^^^^^^^^           ^^^
                        With .csv           No .dll extension!
```

## Key Differences

| Column | Working (2.66) | Not Working | Notes |
|--------|----------------|-------------|-------|
| Assets | `AssetsRithmic.csv` | `Assets` | Must include `.csv` extension |
| Plugin | `NT8` | `NT8.dll` | Must NOT include `.dll` extension |

## Why This Matters

In Zorro 2.66, when the account loads:
1. Shows "Rithmic connection screen" 
2. Loads plugin correctly
3. Orders execute

In our Zorro 2.70 setup:
1. NO connection screen shown
2. Plugin may not be fully initialized
3. Orders fail silently

## The Fix Applied

1. Changed `Assets` ? `AssetsRithmic.csv`
2. Changed `NT8.dll` ? `NT8`
3. Copied Assets.csv to AssetsRithmic.csv

## Files Updated

- `zorro/History/Accounts.csv` - Plugin column now just "NT8"
- `zorro/Log/AssetsRithmic.csv` - Created (copy of Assets.csv)

## Testing Now Required

1. Restart Zorro 2.70
2. Load TradeTest
3. **Look for connection screen** (like 2.66 shows)
4. Try Buy Long order
5. Check if it executes

## Hypothesis

The `.dll` extension or missing `.csv` extension may cause Zorro to:
- Not properly recognize the plugin
- Skip plugin initialization steps
- Fail to call BrokerBuy2 (which is why we saw no debug output)

This would explain why:
- Connection appears to work (BrokerLogin succeeds)
- Market data works (prices update)
- But orders never reach BrokerBuy2

## Next Test

Run TradeTest in Zorro 2.70 with the corrected Accounts.csv format and see if:
1. Connection screen appears
2. Debug output from BrokerBuy2 appears
3. Orders execute

---

**Date:** 2026-01-01  
**Status:** Fix applied, ready for testing  
**Expected:** Should now match working Zorro 2.66 behavior
