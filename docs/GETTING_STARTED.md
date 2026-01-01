# Getting Started with NT8 Zorro Plugin

Quick guide to get trading with the NinjaTrader 8 Zorro Plugin.

## Prerequisites

Before you begin, ensure you have:
- Installed the plugin ([Installation Guide](INSTALLATION.md))
- NinjaTrader 8.1+ running with AddOn active
- Zorro configured with NT8 account
- Connected to data feed in NinjaTrader (or using Sim)

---

## Your First Trade Test

### Step 1: Create a Simple Test Script

Create `FirstTest.c` in `C:\Zorro\Strategy\`:

```c
// FirstTest.c - Your first NT8 plugin test
function run()
{
    BarPeriod = 1;
    LookBack = 0;  // Live-only, no historical data
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 1);  // Enable logging
        printf("\n=== First Test with NT8 ===");
    }
    
    // Set the asset - MES March 2026
    asset("MESH26");
    
    // Display account info
    printf("\nAccount Balance: $%.2f", Balance);
    printf("\nCurrent Price: %.2f", priceClose());
    printf("\nSpread: %.4f", Spread);
    
    // Check position
    int pos = brokerCommand(GET_POSITION, (long)Asset);
    printf("\nCurrent Position: %d contracts", pos);
    
    quit("Test complete!");
}
```

### Step 2: Run the Test

1. **Start NinjaTrader**
   - Connect to data (or use Sim)
   - Verify Output: `[Zorro ATI] listening on port 8888`

2. **Start Zorro**
   - Select account: **NT8-Sim**
   - Select script: **FirstTest**
   - Click **Trade** (not Test - we want live connection)

3. **Expected Output**:
   ```
   === First Test with NT8 ===
   Account Balance: $100000.00
   Current Price: 6047.50
   Spread: 0.2500
   Current Position: 0 contracts
   Test complete!
   ```

---

## Understanding Live-Only Trading

This plugin provides **live trading only** (no historical data):

```c
// Required for live-only trading
LookBack = 0;  // Don't request historical bars
```

**Why?**
- NinjaTrader's API doesn't expose historical data
- Focus on live trading with real-time data
- Use separate data source for backtesting if needed

---

## Symbol Formats

The plugin automatically handles multiple symbol formats:

### Futures Contracts

```c
// All these work for MES March 2026:
asset("MESH26");      // Month code format (recommended)
asset("MES 03-26");   // NinjaTrader direct format
asset("MES 0326");    // Alternative format

// Month codes:
// H = March, M = June, U = September, Z = December
```

### Common Futures Symbols

| Zorro Code | NT8 Format | Description |
|------------|------------|-------------|
| MESH26 | MES 03-26 | Micro E-mini S&P March 2026 |
| ESH26 | ES 03-26 | E-mini S&P March 2026 |
| MNQH26 | MNQ 03-26 | Micro Nasdaq March 2026 |
| NQH26 | NQ 03-26 | E-mini Nasdaq March 2026 |

---

## Basic Trading Script

### Simple Moving Average Crossover

```c
// SMA_Cross.c - Basic trading strategy
function run()
{
    BarPeriod = 60;  // 1-hour bars
    LookBack = 0;    // Live-only
    
    asset("MESH26");
    
    // Get current price
    var price = priceClose();
    
    // Check position
    int pos = brokerCommand(GET_POSITION, (long)Asset);
    
    // Simple logic: buy on upticks if flat
    if(pos == 0 && price > priceClose(1)) {
        printf("\nBuying 1 contract at %.2f", price);
        enterLong(1);  // Buy 1 contract
    }
    
    // Exit if have position and price falls
    if(pos > 0 && price < priceClose(1)) {
        printf("\nSelling position at %.2f", price);
        exitLong();
    }
}
```

**WARNING**: This is example code only. Test thoroughly with simulation before live trading!

---

## Account Management

### Check Account Status

```c
printf("\nBalance: $%.2f", Balance);
printf("\nMargin: $%.2f", MarginVal);
```

### Check Position

```c
int pos = brokerCommand(GET_POSITION, (long)Asset);
if(pos > 0)
    printf("\nLong %d contracts", pos);
else if(pos < 0)
    printf("\nShort %d contracts", -pos);
else
    printf("\nFlat (no position)");
```

### Check Average Entry

```c
if(pos != 0) {
    var entry = brokerCommand(GET_AVGENTRY, (long)Asset);
    printf("\nAverage Entry: %.2f", entry);
    printf("\nCurrent P&L: $%.2f", (priceClose() - entry) * pos * 5);
}
```

---

## Order Placement

### Market Orders

```c
// Buy 1 contract at market
enterLong(1);

// Sell 1 contract at market
enterShort(1);

// Close position
if(pos > 0)
    exitLong();   // Close long
else if(pos < 0)
    exitShort();  // Close short
```

### Limit Orders

```c
// Buy limit at 6000.00
OrderLimit = 6000.00;
enterLong(1);

// Sell limit at 6100.00
OrderLimit = 6100.00;
exitLong();
```

---

## Real-Time Data Access

### Price Data

```c
var last = priceClose();      // Last traded price
var bid = last - Spread/2;    // Approximate bid
var ask = last + Spread/2;    // Approximate ask
```

### Market State

```c
// Check if we have data
if(priceClose() > 0) {
    printf("\nMarket data available");
    printf("\nPrice: %.2f", priceClose());
} else {
    printf("\nNo market data yet");
}
```

---

## Best Practices

### 1. Always Use Simulation First

```c
// In accounts.csv, use:
NT8-Sim,Sim101,,Demo

// NOT your live account initially!
```

### 2. Enable Diagnostics

```c
if(is(INITRUN)) {
    brokerCommand(SET_DIAGNOSTICS, 1);
}
```

### 3. Check Connection

```c
if(is(INITRUN)) {
    if(!Live) {
        printf("\nNot connected to broker!");
        quit("Connection failed");
    }
}
```

### 4. Handle Market Hours

```c
// Markets closed message will appear automatically
// Your script stops when market closes
```

### 5. Start Small

```c
// Test with 1 contract first
enterLong(1);  // NOT enterLong(10) initially!
```

---

## Common Patterns

### Buy and Hold

```c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    asset("MESH26");
    
    // Buy once at start
    if(is(INITRUN)) {
        enterLong(1);
    }
    
    // Monitor position
    int pos = brokerCommand(GET_POSITION, (long)Asset);
    printf("\nPosition: %d", pos);
}
```

### Scalping

```c
function run()
{
    BarPeriod = 1;  // 1-minute bars
    LookBack = 0;
    
    asset("MESH26");
    
    var price = priceClose();
    int pos = brokerCommand(GET_POSITION, (long)Asset);
    
    // Enter on specific conditions
    if(pos == 0 && <your_entry_condition>) {
        enterLong(1);
    }
    
    // Quick exit on target or stop
    if(pos > 0) {
        var entry = brokerCommand(GET_AVGENTRY, (long)Asset);
        var profit = (price - entry) * 5;  // MES = $5/point
        
        if(profit >= 25 || profit <= -12.50) {  // $25 target, $12.50 stop
            exitLong();
        }
    }
}
```

---

## Debugging Tips

### Check Connection

```c
if(is(INITRUN)) {
    printf("\n--- Connection Check ---");
    printf("\nLive: %d", Live);
    printf("\nConnection: %s", Live ? "OK" : "FAILED");
}
```

### Monitor Orders

```c
// Zorro will log order placement automatically
// Watch Zorro message window for:
// "# Order 1000 (ID): BUY 1 MESH26 @ MARKET"
```

### View Detailed Logs

- **Zorro logs**: `C:\Zorro\Log\<ScriptName>_demo.log`
- **NinjaTrader logs**: Check Output window
  - Shows: connections, orders, fills

---

## Next Steps

1. Run FirstTest.c successfully
2. Understand live-only trading
3. Test basic order placement (sim account!)
4. Monitor positions and P&L
5. Read [API Reference](API_REFERENCE.md)
6. Build your strategy
7. Test extensively in simulation
8. Go live (start small!)

---

## Quick Reference Card

```c
// Essential commands:
BarPeriod = 1;              // Bar period in minutes
LookBack = 0;               // Live-only (no historical)
asset("MESH26");            // Select asset

Balance                     // Account balance
MarginVal                   // Available margin
priceClose()               // Current price
Spread                     // Bid-ask spread

enterLong(n)               // Buy n contracts
enterShort(n)              // Sell n contracts
exitLong()                 // Close long position
exitShort()                // Close short position

// Broker commands:
brokerCommand(GET_POSITION, (long)Asset)   // Get position
brokerCommand(GET_AVGENTRY, (long)Asset)   // Get avg entry
brokerCommand(SET_DIAGNOSTICS, 1)          // Enable logging
```

---

**Ready to trade?** Make sure you've tested thoroughly with simulation accounts before going live!

For detailed API documentation, see [API Reference](API_REFERENCE.md).
