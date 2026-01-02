# Stop Order Guide

## Overview

The NT8 Zorro plugin supports stop orders for automated entry and exit strategies. Stop orders trigger when the market reaches a specified price level, allowing you to enter breakouts or protect positions with stop-losses.

## Stop Order Types

### 1. Stop-Market Orders

Triggers at stop price, executes at market.

```c
// Buy stop (enter long above market)
Stop = 10 * PIP;  // Trigger 10 ticks above current price
enterLong(1);

// Sell stop (enter short below market)  
Stop = 10 * PIP;  // Trigger 10 ticks below current price
enterShort(1);
```

### 2. Stop-Limit Orders

Triggers at stop price, executes as limit order.

```c
// Buy stop-limit
Stop = 10 * PIP;      // Trigger price (above market)
OrderLimit = 6050.00; // Maximum buy price after trigger
enterLong(1);

// Sell stop-limit
Stop = 10 * PIP;      // Trigger price (below market)
OrderLimit = 6040.00; // Minimum sell price after trigger
enterShort(1);
```

## How It Works

### Entry Stop Orders

**Buy Stop (Breakout Long)**
- Placed **above** current market price
- Triggers when price rises to stop level
- Used for: Entering on upside breakouts

**Sell Stop (Breakout Short)**
- Placed **below** current market price
- Triggers when price falls to stop level
- Used for: Entering on downside breakouts

### Exit Stop Orders (Stop-Loss)

**Long Position Stop-Loss**
- Placed **below** entry price
- Triggers when price falls to stop level
- Protects long positions from losses

**Short Position Stop-Loss**
- Placed **above** entry price
- Triggers when price rises to stop level
- Protects short positions from losses

## Implementation Flow

1. **Zorro Script** sets `Stop` global variable (distance in price units)
2. **BrokerBuy2** calculates stop price from current market + Stop distance
3. **TcpBridge** sends stop price to NinjaTrader
4. **ZorroBridge.cs** creates `OrderType.StopMarket` or `OrderType.StopLimit`
5. **NinjaTrader** monitors market and triggers order when price reached

## Usage Examples

### Example 1: Breakout Entry with Buy Stop

```c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    asset("MES 0326");
    Lots = 1;
    
    // Enter long if price breaks above resistance
    var resistance = 6050.00;
    var currentPrice = priceClose();
    
    if(NumOpenLong == 0 && currentPrice < resistance) {
        // Place buy stop above resistance
        Stop = resistance - currentPrice + PIP;
        enterLong();
        
        printf("\nBuy stop placed at: %.2f", resistance + PIP);
    }
}
```

### Example 2: Stop-Loss Protection

```c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    asset("MES 0326");
    Lots = 1;
    
    // Enter at market with stop-loss
    if(NumOpenLong == 0) {
        // Enter long at market
        enterLong();
    }
    
    // Set stop-loss 20 ticks below entry
    if(NumOpenLong > 0) {
        var stopPrice = TradePriceOpen - 20*PIP;
        
        // Exit with stop-loss
        Stop = TradePriceOpen - stopPrice;
        exitLong();
    }
}
```

### Example 3: Trailing Stop

```c
var g_HighestPrice = 0;

function run()
{
    BarPeriod = 1;
    LookBack = 0;
    asset("MES 0326");
    
    if(NumOpenLong > 0) {
        // Update trailing stop
        var currentPrice = priceClose();
        
        if(currentPrice > g_HighestPrice) {
            g_HighestPrice = currentPrice;
        }
        
        // Stop 15 ticks below highest price
        var trailStop = g_HighestPrice - 15*PIP;
        
        if(currentPrice <= trailStop) {
            printf("\nTrailing stop hit at: %.2f", currentPrice);
            exitLong();
        }
    } else {
        g_HighestPrice = 0;  // Reset on new entry
    }
}
```

### Example 4: Support/Resistance Breakout

```c
function run()
{
    BarPeriod = 60;  // 1-hour bars
    LookBack = 0;
    asset("MES 0326");
    Lots = 1;
    
    var support = 6000.00;
    var resistance = 6100.00;
    var currentPrice = priceClose();
    
    if(NumOpenLong == 0 && NumOpenShort == 0) {
        // Inside range - place both stops
        if(currentPrice > support && currentPrice < resistance) {
            // Buy stop above resistance (long breakout)
            Stop = resistance - currentPrice + 2*PIP;
            enterLong();
            
            // Sell stop below support (short breakdown)
            Stop = currentPrice - support + 2*PIP;
            enterShort();
            
            printf("\nBreakout orders placed:");
            printf("\n  Buy stop: %.2f", resistance + 2*PIP);
            printf("\n  Sell stop: %.2f", support - 2*PIP);
        }
    }
}
```

## Stop Distance Calculation

The `Stop` variable specifies distance in **price units** from current market:

```c
// Current price: 6050.00
// PIP: 0.25

Stop = 10 * PIP;     // Distance = 2.50 points
enterLong();         // Buy stop at: 6052.50

Stop = 20 * PIP;     // Distance = 5.00 points  
enterShort();        // Sell stop at: 6045.00
```

**Direction automatically determined by order side:**
- `enterLong()` with `Stop > 0` ? Buy stop **above** market
- `enterShort()` with `Stop > 0` ? Sell stop **below** market

## Stop vs Limit Comparison

| Feature | Stop Order | Limit Order |
|---------|------------|-------------|
| **Placement** | Above (buy) or below (sell) market | Below (buy) or above (sell) market |
| **Purpose** | Enter breakouts, stop-losses | Enter pullbacks, take profits |
| **Trigger** | When market reaches stop price | Immediately at limit or better |
| **Fill Price** | Market price after trigger | At limit or better |

## Common Patterns

### Pattern 1: Breakout with Stop-Loss

```c
// Enter on breakout, protect with stop
if(NumOpenLong == 0) {
    // Buy stop entry
    Stop = 10 * PIP;  // Above market
    enterLong();
}

if(NumOpenLong > 0) {
    // Stop-loss exit
    Stop = 20 * PIP;  // Below entry (calculated by plugin)
    exitLong();
}
```

### Pattern 2: Range Trading with Stops

```c
var highRange = 6100.00;
var lowRange = 6000.00;

if(NumOpenLong == 0 && NumOpenShort == 0) {
    if(priceClose() > lowRange && priceClose() < highRange) {
        // Buy if breaks above range
        Stop = highRange - priceClose() + PIP;
        enterLong();
        
        // Sell if breaks below range
        Stop = priceClose() - lowRange + PIP;
        enterShort();
    }
}
```

### Pattern 3: Scalping with Tight Stops

```c
if(NumOpenLong > 0) {
    // Very tight stop for scalping
    Stop = 5 * PIP;  // Only 5 ticks of room
    exitLong();
}
```

## Important Notes

### Stop Direction Rules

**For Entry Orders:**
- Buy stop: Always placed **above** current market
- Sell stop: Always placed **below** current market

**For Exit Orders:**
- Long exit stop: Below entry/current price (stop-loss)
- Short exit stop: Above entry/current price (stop-loss)

### Stop Price Calculation

The plugin automatically calculates stop price:

```cpp
// In BrokerBuy2:
if (Amount > 0) {
    // Buy stop: above market
    stopPrice = currentPrice + StopDist;
} else {
    // Sell stop: below market
    stopPrice = currentPrice - StopDist;
}
```

### Stop-Limit Orders

When both `Stop` and `OrderLimit` are set:

```c
Stop = 10 * PIP;      // Trigger price
OrderLimit = 6050.00; // Limit after trigger
enterLong();          // Stop-limit order
```

**Behavior:**
1. Order triggers when market reaches stop price
2. Becomes limit order at `OrderLimit`
3. Fills only at limit or better

## Testing Stop Orders

### Manual Testing (TradeTest.c)

1. Start NinjaTrader and Zorro
2. Load TradeTest script
3. Set Stop slider (0-50)
4. Click "Buy Long" or "Buy Short"
5. Watch Orders window for stop orders

### Automated Testing (StopOrderTest.c)

```bash
# Copy test script
copy scripts\StopOrderTest.c C:\Zorro_2.66\Strategy\

# Run in Zorro
1. Select NT8-Sim account
2. Load StopOrderTest
3. Click Trade
```

**Expected Results:**
- Buy stop triggers when price rises
- Sell stop triggers when price falls
- Fills at or near stop price

## Troubleshooting

### Stop Order Not Triggering

**Possible causes:**
1. Stop price not reached yet (normal)
2. Stop in wrong direction (check buy vs sell)
3. Market moved through stop too fast

**Solutions:**
- Verify stop direction matches order side
- Check NinjaTrader Orders window
- Ensure market is active

### Stop Filled Far from Stop Price

**Cause:** Slippage in fast markets

**Solutions:**
- Use stop-limit orders to control fill price
- Widen stop distance in volatile markets
- Consider time of day (avoid news events)

### Stop Order Rejected

**Possible causes:**
1. Stop price invalid (too close to market)
2. Insufficient margin
3. Market closed

**Solutions:**
- Increase stop distance from current price
- Check account margin
- Verify market hours

## Best Practices

### 1. Reasonable Stop Distance

```c
// Good: At least 2-3 ticks from market
Stop = 3 * PIP;

// Bad: Too close, may trigger immediately
Stop = 0.5 * PIP;
```

### 2. Always Clear Stop Variable

```c
Stop = 10 * PIP;
enterLong();
Stop = 0;  // IMPORTANT: Clear for next order
```

### 3. Use Stop-Limits in Volatile Markets

```c
Stop = 10 * PIP;       // Trigger price
OrderLimit = 6055.00;  // Maximum acceptable price
enterLong();
```

### 4. Test Before Live Trading

```c
// Test with simulation first!
// - Verify stop triggers correctly
// - Check fill prices  
// - Monitor slippage
```

### 5. Monitor Stop Fills

```c
if(TradeIsOpen && TradePriceOpen > 0) {
    printf("\nFilled at: %.2f", TradePriceOpen);
    printf("\nExpected stop: %.2f", g_StopPrice);
    printf("\nSlippage: %.2f", TradePriceOpen - g_StopPrice);
}
```

## Implementation Status

? **Fully Implemented:**
- Stop-market orders (BUY STOP, SELL STOP)
- Stop-limit orders (combined stop + limit)
- Entry stop orders (breakouts)
- Exit stop orders (stop-loss)
- Automatic stop price calculation

?? **Limitations:**
- Trailing stops require script logic (not automatic)
- One-Cancels-Other (OCO) not yet supported
- Stop modification requires cancel & replace

## See Also

- **[StopOrderTest.c](../scripts/StopOrderTest.c)** - Automated test suite
- **[TradeTest.c](../scripts/TradeTest.c)** - Manual stop order testing
- **[LIMIT_ORDER_GUIDE.md](LIMIT_ORDER_GUIDE.md)** - Limit order documentation
- **[API Reference](API_REFERENCE.md)** - Complete API documentation

## Summary

? Stop orders are **fully functional** in the NT8 plugin  
? Use `Stop` variable to set distance from market  
? Buy stops placed above market, sell stops below  
? Combine with `OrderLimit` for stop-limit orders  
? Test with **StopOrderTest.c** or **TradeTest.c**  

**Ready for breakout strategies and stop-loss protection!** ??
