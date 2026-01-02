# Testing on Holidays/Weekends

## The Problem

When running automated tests like `AutoTradeTest.c` on holidays or weekends, Zorro will exit with:

```
Market closed on 01-01 16:52:11
```

Even though:
- ? NinjaTrader is providing simulated data
- ? Connection is working
- ? Prices are flowing
- ? `BrokerTime()` returns 2 (market open)

## Why This Happens

Zorro has **multiple layers** of market-hours checking:

1. **Plugin Level** (`BrokerTime()`) - ? We return "market open"
2. **Asset Level** (`MarketOpen`/`MarketClose`) - ? Can be set to 24/7
3. **Exchange Calendar** - ? Can be disabled with `Market=SIM`
4. **Global Holiday Check** - ? **Cannot be disabled via code in Zorro 2.70**

January 1st is a global holiday in Zorro's internal calendar.

## Solutions

### Option 1: Test on Regular Trading Days (Recommended)
Wait until a regular weekday (Monday-Friday, not a holiday) to run automated tests.

### Option 2: Use Manual Tests
Use `TradeTest.c` (the manual test that comes with Zorro) which allows you to click Buy/Sell buttons manually, even on holidays.

### Option 3: Use NinjaTrader Playback Mode
See [PLAYBACK_TESTING.md](../private-docs/PLAYBACK_TESTING.md) for how to use NinjaTrader's Playback connection with historical data, which has its own time control.

## What We Tried (That Didn't Work)

Based on extensive testing, these do **NOT** bypass the holiday check:

- ? `NOWEEKEND` define
- ? `NOHOLIDAY` flag (doesn't exist in Zorro 2.70)
- ? `TESTNOW` flag (doesn't exist in Zorro 2.70)  
- ? `MarketZone = 0` (not a valid variable)
- ? `StartDate`/`EndDate = 0`
- ? `MarketOpen = 0; MarketClose = 24; Weekend = 1;` (undeclared identifiers)
- ? `assetMarket()` function (doesn't exist in Zorro 2.70)
- ? Asset list `Holidays=0` column
- ? Asset list `Market=SIM` or `Market=FX`

## Current Status

The basic connection and data tests work fine on holidays:
- `test-zorro.ps1` - ? Passes (connection, data, account info)
- `AutoTradeTest.c` - ? Blocked on holidays
- `TradeTest.c` (manual) - ? Works on holidays (manual button clicks)

## Recommendation

**Just test during market hours.** The plugin is working correctly - Zorro's holiday protection is intentional and cannot be bypassed in version 2.70.
