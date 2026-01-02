# Limit Order Guide

## Overview

The NT8 Zorro plugin fully supports limit orders for both LONG and SHORT positions. Limit orders allow you to specify the maximum price you're willing to pay (for buys) or minimum price you're willing to accept (for sells).

## How It Works

### In Zorro Scripts

Simply set `OrderLimit` before calling `enterLong()` or `enterShort()`:

```c
// Buy LONG with limit order
OrderLimit = 6900.00;  // Won't pay more than 6900
enterLong(1);

// Sell SHORT with limit order  
OrderLimit = 6910.00;  // Won't sell for less than 6910
enterShort(1);
```

### Plugin Flow

1. **Zorro** sets `OrderLimit` global variable
2. **BrokerBuy2** checks `if (Limit > 0)` and sets `orderType = "LIMIT"`
3. **TcpBridge** sends: `PLACEORDER:BUY:MES 03-26:1:LIMIT:6900.00`
4. **ZorroBridge.cs** parses limit price and creates `OrderType.Limit`  
5. **NinjaTrader** places limit order on exchange
6. **Order fills** when market reaches limit price (or better)

## Implementation Status

? **Fully Implemented:**
- LONG limit orders (buy limit)
- SHORT limit orders (sell limit)
- Limit order placement
- Fill monitoring
- Price validation

?? **Partial Support:**
- **Stop orders** - Structure ready, needs testing
- **Stop-limit orders** - Not yet implemented

? **Not Supported:**
- FOK (Fill-Or-Kill) limit orders
- IOC (Immediate-Or-Cancel) limit orders  
- Order modification (must cancel & replace)

## Testing

### Automated Test

```bash
# Copy test script
copy scripts\LimitOrderTest.c C:\Zorro_2.66\Strategy\

# Run in Zorro
1. Start NinjaTrader
2. Start Zorro 2.66
3. Select NT8-Sim account
4. Load LimitOrderTest script
5. Click Trade
```

### Manual Test (TradeTest.c)

```bash
# Run TradeTest
Zorro -run TradeTest

# In Zorro UI:
1. Click "MKT Order" ? becomes "LMT Order"
2. Adjust "Limit" slider (0-1000)
   - 500 = 0.5 spreads offset from market
3. Click "Buy Long" or "Buy Short"
4. Check NT Orders window for limit price
5. Observe fill when price reaches limit
```

## Limit Order Behavior

### LONG (Buy) Limit Orders

**Rule:** Limit = Maximum price you'll pay

```c
// Current market: Bid 6900 / Ask 6900.25
OrderLimit = 6899.00;  // Below market
enterLong(1);

// Result: Order pending until price drops to 6899 or lower
// Fills at 6899.00 or better (lower price = better for buyer)
```

### SHORT (Sell) Limit Orders

**Rule:** Limit = Minimum price you'll accept

```c
// Current market: Bid 6900 / Ask 6900.25
OrderLimit = 6901.00;  // Above market
enterShort(1);

// Result: Order pending until price rises to 6901 or higher
// Fills at 6901.00 or better (higher price = better for seller)
```

## Common Patterns

### Limit Entry with Market Exit

```c
// Enter with limit
OrderLimit = priceBid() - PIP;  // 1 tick below market
enterLong(1);

// Exit at market (no OrderLimit)
OrderLimit = 0;
exitLong();
```

### Adaptive Limit Orders

```c
// Try limit first
OrderLimit = priceBid();
int id = enterLong(1);

// Check if filled after few bars
if(!TradeIsOpen) {
    // Order missed, try with better limit
    OrderLimit = priceAsk();  // Cross the spread
    enterLong(1);
}
```

### Limit with Stop-Loss

```c
// Limit entry
OrderLimit = 6900.00;
enterLong(1);

// Market stop-loss (after fill)
Stop = 10 * PIP;  // 10 ticks
```

## Price Validation

The plugin validates limit orders:

### LONG Orders (Buy Limit)
- ? **Valid:** Limit ? Market Ask
- ? **Invalid:** Limit > Market Ask (would execute immediately as market order)

### SHORT Orders (Sell Limit)  
- ? **Valid:** Limit ? Market Bid
- ? **Invalid:** Limit < Market Bid (would execute immediately as market order)

**Note:** NinjaTrader may auto-adjust invalid limits or execute as market order.

## Monitoring Fills

### Check Order Status

```c
// After placing limit order
int id = enterLong(1);

// Check if filled
if(TradeIsOpen) {
    printf("Filled at: %.2f", TradePriceOpen);
} else {
    printf("Still pending");
}
```

### Using TMF (Trade Management Function)

```c
int myTMF()
{
    if(TradeIsMissed) {
        // Order not filled, adapt limit
        OrderLimit += PIP;  // Improve limit by 1 tick
        return 2 + 16;  // Resubmit order
    }
    return 0;
}

// Use TMF
OrderLimit = priceBid();
enterLong(myTMF);
```

## Troubleshooting

### Order Places but Never Fills

**Possible causes:**
1. Limit price too far from market
2. Market moving away from limit
3. Low liquidity at limit price

**Solutions:**
- Reduce offset from market
- Use adaptive limit algorithm
- Consider market order instead

### Order Fills at Wrong Price

**Check:**
1. Was limit price set correctly?
2. Did you get "price improvement" (better than limit)?
3. Check NinjaTrader execution report

**Debug:**
```c
printf("Limit set: %.2f", OrderLimit);
printf("Fill price: %.2f", TradePriceOpen);
printf("Difference: %.2f", TradePriceOpen - OrderLimit);
```

### Order Executes Immediately as Market

**Cause:** Limit price crossed the spread

```c
// BAD - crosses spread, executes as market
OrderLimit = priceAsk() + PIP;  // Above ask!
enterLong(1);

// GOOD - below market, waits for price
OrderLimit = priceBid() - PIP;
enterLong(1);
```

## Best Practices

### 1. Set Reasonable Limits

```c
// Good: Within 1-2 ticks of market
OrderLimit = priceBid() - PIP;

// Bad: Too far from market (may never fill)
OrderLimit = priceBid() - 100*PIP;
```

### 2. Clear OrderLimit After Each Trade

```c
OrderLimit = priceBid();
enterLong(1);

// IMPORTANT: Clear for next trade
OrderLimit = 0;
```

### 3. Use PIP for Offsets

```c
// Good: Tick-based offset
OrderLimit = priceBid() - 2*PIP;

// Bad: Fixed dollar offset (not portable)
OrderLimit = priceBid() - 0.50;
```

### 4. Validate Before Placing

```c
var limit = priceBid() - PIP;

// Sanity check
if(limit > 0 && limit < priceAsk()) {
    OrderLimit = limit;
    enterLong(1);
} else {
    printf("Invalid limit price: %.2f", limit);
}
```

## Example Scripts

### 1. Simple Limit Order Entry

```c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    asset("MES 0326");
    
    if(NumOpenLong == 0) {
        // Buy 1 tick below bid
        OrderLimit = priceBid() - PIP;
        enterLong(1);
    }
}
```

### 2. Limit Entry, Market Exit

```c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    asset("MES 0326");
    Lots = 1;
    
    if(NumOpenLong == 0) {
        // Enter with limit
        OrderLimit = priceBid() - PIP;
        enterLong();
    } else if(priceClose() > TradePriceOpen + 5*PIP) {
        // Exit at market when 5 ticks profit
        OrderLimit = 0;
        exitLong();
    }
}
```

### 3. Adaptive Limit (like TradeTest.c)

```c
int adaptLimit()
{
    if(TradeIsMissed) {
        // Improve limit by 0.5 PIP
        OrderLimit += 0.5 * PIP;
        printf("Adapting limit to: %.2f", OrderLimit);
        return 2 + 16;  // Retry
    }
    return 0;
}

function run()
{
    BarPeriod = 1;
    LookBack = 0;
    asset("MES 0326");
    
    if(NumOpenLong == 0) {
        OrderLimit = priceBid();
        enterLong(adaptLimit);
    }
}
```

## See Also

- **[TradeTest.c](../scripts/TradeTest.c)** - Manual limit order testing
- **[LimitOrderTest.c](../scripts/LimitOrderTest.c)** - Automated test suite
- **[API Reference](API_REFERENCE.md)** - Full broker API documentation
- **[LIMIT_ORDER_TESTING.md](LIMIT_ORDER_TESTING.md)** - Technical implementation details

## Summary

? Limit orders are **fully functional** in the NT8 plugin  
? Use `OrderLimit` to set limit price before entering trades  
? Works for both LONG and SHORT positions  
? Fills at limit price or better  
? Test with **LimitOrderTest.c** or **TradeTest.c**  

**Ready to use in production strategies!** ??
