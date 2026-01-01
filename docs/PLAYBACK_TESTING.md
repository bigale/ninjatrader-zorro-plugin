# Using Playback/Market Replay for Testing

This guide explains how to use NinjaTrader's Playback Connection (Market Replay) to test your trading strategies without waiting for live market hours.

## Overview

**Playback Connection** allows you to:
- ? Test strategies with historical data as if it were live
- ? Speed up or slow down time
- ? Test 24/7 without waiting for market open
- ? Replay specific market conditions
- ? Practice manual trading

**Works with our plugin:** The ZorroBridge AddOn sees playback data as live data, so everything works identically!

---

## Setup Playback Connection

### Step 1: Enable Market Replay in NinjaTrader

1. **Open NinjaTrader 8**

2. **Go to Connections**
   - Click **Connections** in the Control Center
   - Or: Control Center ? Tools ? Connections

3. **Add Playback Connection**
   - Click **Add...**
   - Select **Playback Connection**
   - Click **OK**

4. **Configure (if needed)**
   - Default settings work fine
   - Click **Connect**

5. **Verify Connection**
   - Status should show "Connected"
   - You'll see "Playback101" in account list

---

## Using Playback with Zorro

### Method 1: Quick Playback Test

**1. Configure Zorro Account**

Edit `C:\Zorro\History\accounts.csv`:
```csv
NT8-Playback,Playback101,,Demo
```

**2. Start Playback in NinjaTrader**

- **Connect to Playback** (see above)
- **Open a Chart:**
  - Right-click chart ? Data Series
  - Select your instrument (e.g., MES 03-26)
  - Click OK

- **Start Market Replay:**
  - Click **Market Replay** button on chart
  - Or: Right-click chart ? Market Replay ? Controller
  - Click **Play** ?

**3. Run Your Zorro Script**

```c
// PlaybackTest.c
function run()
{
    BarPeriod = 1;
    LookBack = 0;  // Live-only
    
    asset("MESH26");
    
    printf("\nTime: %s", strdate(WDate, 0));
    printf("\nPrice: %.2f", priceClose());
    
    // Your strategy logic here
    int pos = brokerCommand(GET_POSITION, (long)Asset);
    if(pos == 0 && priceClose() > priceClose(1))
        enterLong(1);
}
```

**4. Start Trading in Zorro**
- Select **NT8-Playback** account
- Select your script
- Click **Trade**

**The script will execute as playback time advances!**

---

### Method 2: Automated Playback Testing

For automated testing that runs at high speed:

**1. Set Playback Speed**

In Market Replay Controller:
- 1x = Real-time
- 10x = 10 times faster
- 100x = Very fast (be careful with order placement!)
- Max = As fast as possible

**2. Configure Script for Playback**

```c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    asset("MESH26");
    
    // Log current playback time
    printf("\n[%s] Price: %.2f Pos: %d", 
        strdate(WDate, 0), priceClose(), 
        brokerCommand(GET_POSITION, (long)Asset));
    
    // Your strategy
    if(/* entry condition */)
        enterLong(1);
    
    if(/* exit condition */)
        exitLong();
}
```

**3. Run Test Session**
- Start playback in NinjaTrader
- Set desired speed (e.g., 10x)
- Start your Zorro script
- Watch it execute historical trades rapidly!

---

## Playback Account Configuration

### accounts.csv Setup

```csv
# Playback account
NT8-Playback,Playback101,,Demo

# Can also use with your sim account name
NT8-Sim,Sim101,,Demo
```

**Note:** Playback uses the same account structure as Sim101.

---

## Best Practices for Playback Testing

### 1. Start with Recent Data

```
Use recent market data (last few weeks/months)
- More relevant conditions
- Faster to load
- Better for strategy validation
```

### 2. Test Specific Scenarios

**Example: Test a volatile session**
```
1. Load data from a known volatile day
2. Set playback to that date/time
3. Run your strategy
4. Observe behavior under stress
```

### 3. Use Appropriate Speed

| Speed | Use Case |
|-------|----------|
| 1x | Manual testing, order placement practice |
| 5-10x | Strategy validation, watching behavior |
| 50-100x | Quick backtesting, parameter testing |
| Max | Rapid testing (watch for missed orders!) |

**?? Warning:** At very high speeds (100x+), orders might not fill as expected due to timing.

### 4. Record Results

```c
function run()
{
    // Log trades to file
    if(/* trade executed */) {
        char msg[256];
        sprintf(msg, "%s,%.2f,%d\n", 
            strdate(WDate, 0), priceClose(), TradeProfit);
        file_append("playback_results.csv", msg);
    }
}
```

---

## Playback vs Live Trading Differences

| Aspect | Playback | Live |
|--------|----------|------|
| Data Source | Historical replay | Real-time feed |
| Time Control | You control speed | Real-time only |
| Order Fills | Simulated | Real fills |
| Slippage | None/minimal | Real slippage |
| Market Impact | None | Can affect price |
| Testing Speed | Can accelerate | Real-time only |

**Important:** Playback is great for logic testing but doesn't perfectly simulate real market conditions.

---

## Troubleshooting Playback

### "No bars generated" in Playback

**Solutions:**
1. **Make sure playback is running:**
   - Click Play ? in Market Replay Controller
   - Time should be advancing

2. **Check symbol is loaded:**
   - Open chart with the instrument
   - Verify data appears on chart

3. **Verify account:**
   - Confirm "Playback101" in accounts.csv
   - Check NinjaTrader shows Playback connected

### Orders Not Executing

**Check:**
1. **Playback speed:** Too fast can miss fills
2. **Market hours:** Even in playback, some checks apply
3. **Order type:** Limit orders need price to reach level

### Script Runs Too Slow

**Solutions:**
1. Increase playback speed in NT8
2. Reduce `BarPeriod` frequency
3. Optimize strategy logic
4. Use faster machine/fewer indicators

---

## Example: Full Playback Test Session

### Setup (One-time)

**1. Install Plugin & AddOn** (if not done)
```bash
# See INSTALLATION.md
```

**2. Configure Playback Account**
```csv
# C:\Zorro\History\accounts.csv
NT8-Playback,Playback101,,Demo
```

**3. Create Test Script**
```c
// QuickPlaybackTest.c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    asset("MESH26");
    
    static int tradeCount = 0;
    
    // Simple test strategy
    int pos = brokerCommand(GET_POSITION, (long)Asset);
    
    if(pos == 0 && NumBars > 10) {
        // Enter every 10 bars for testing
        if(NumBars % 10 == 0) {
            enterLong(1);
            tradeCount++;
            printf("\n[%s] Trade #%d: BUY @ %.2f", 
                strdate(WDate, 0), tradeCount, priceClose());
        }
    }
    
    if(pos > 0 && NumBars > 5) {
        // Exit after 5 bars
        exitLong();
        printf("\n[%s] EXIT @ %.2f P&L: $%.2f", 
            strdate(WDate, 0), priceClose(), TradeProfit);
    }
}
```

### Run Test

**1. Start NinjaTrader:**
   - Connect to Playback
   - Open MES chart
   - Start Market Replay
   - Set speed to 10x

**2. Start Zorro:**
   - Select NT8-Playback account
   - Load QuickPlaybackTest
   - Click **Trade**

**3. Observe:**
   - Trades execute as playback advances
   - Orders appear in NinjaTrader
   - P&L updates in real-time
   - Logs show trade details

**4. Stop Test:**
   - Click **Stop** in Zorro
   - Pause playback in NinjaTrader
   - Review results

---

## Advanced: Scripted Playback Testing

For automated test runs:

```c
// AutomatedPlaybackTest.c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    // Test runs from specific date/time
    static var startTime = 0;
    static var endTime = 0;
    
    if(is(INITRUN)) {
        startTime = NOW;  // Record start
        printf("\n=== Playback Test Started ===");
        printf("\nStart Time: %s", strdate(NOW, 0));
    }
    
    asset("MESH26");
    
    // Your strategy logic
    // ...
    
    // Auto-stop after X hours of playback time
    if(NOW - startTime > 24) {  // 24 hours of playback
        endTime = NOW;
        printf("\n=== Test Complete ===");
        printf("\nEnd Time: %s", strdate(NOW, 0));
        printf("\nTotal Trades: %d", NumTrades);
        printf("\nWin Rate: %.1f%%", WinRate);
        quit("Playback test finished");
    }
}
```

---

## Summary

**Playback with our plugin:**
- ? Works exactly like live trading
- ? No code changes needed
- ? Great for strategy testing
- ? Can run 24/7 at any speed
- ? Uses same account structure

**Key Points:**
1. Connect to Playback in NinjaTrader
2. Use "Playback101" account in Zorro
3. Start Market Replay
4. Run your normal trading script
5. Control speed and time in NinjaTrader

**Playback is perfect for:**
- Strategy development
- Parameter optimization
- Testing edge cases
- Practice trading
- Demo/presentation

---

## See Also

- [Getting Started Guide](GETTING_STARTED.md)
- [Installation Guide](INSTALLATION.md)
- [Troubleshooting Guide](TROUBLESHOOTING.md)
- [NinjaTrader Playback Documentation](https://ninjatrader.com/support/helpGuides/nt8/market_replay.htm)
