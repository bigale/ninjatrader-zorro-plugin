# TradeTest.c Analysis - Testing Limit Orders

## Overview

The TradeTest.c script is Zorro's manual broker testing tool. It supports multiple order types that can be tested:

## Order Modes (Click "MKT Order" button to cycle)

1. **MKT Order** (`OrderMode = 0`)
   - Market orders (immediate execution at current price)
   - No OrderLimit set

2. **LMT Order** (`OrderMode = 1`)
   - Limit orders with fixed offset
   - `OrderLimit = priceClose() + Factor*Spread`
   - Factor controlled by Limit slider (0-1000)

3. **Adaptive** (`OrderMode = 2`)
   - FOK (Fill-Or-Kill) with adaptive limit
   - Starts at current price
   - Auto-adjusts if order is missed
   - Uses `tmf()` callback to adapt limit by 0.333*Spread increments

4. **GTC Order** (`OrderMode = 3`)
   - Good-Till-Cancelled with limit
   - Like LMT but with `TR_GTC|TR_FILLED` flags
   - Also uses adaptive algorithm via `tmf()`

## Key Features for Testing

### Limit Order Algorithm (LMT Mode)
```c
OrderLimit = priceClose() + Factor*Spread;
```
- **Long Entry**: Limit above market (Factor > 0)
- **Short Entry**: Limit below market (Factor < 0)
- Factor = Limit slider value / 1000
- Example: Slider at 500 = Factor of 0.5 = 0.5 spreads above/below

### Adaptive Algorithm (Adaptive & GTC Modes)
```c
int tmf() // Trade Management Function
{
    if(TradeIsMissed && OrderMode >= 2) {
        var Step = max(0.333*Spread, 0.333*PIP);
        if(OrderMode == 2) // FOK
            OrderDelay = 30; // retry in 30 sec
        else // GTC
            OrderDelay = 0;  // retry immediately
        
        if(!tradeAdapt(Step))
            return 1; // cancel
        else
            return ifelse(TradeIsOpen, 1+16, 2+16); // repeat order
    }
}

int tradeAdapt(var Step)
{
    if(!TradeIsOpen) { // entry limit
        if(TradeIsLong) {
            if(OrderLimit > TradePriceOpen) 
                return 0; // give up
            OrderLimit += Step; // move limit up
        } else { // short
            if(OrderLimit < TradePriceOpen-Spread) 
                return 0;
            OrderLimit -= Step; // move limit down
        }
    }
    // ... exit logic similar ...
}
```

## How Our Plugin Handles These

### Market Orders (OrderMode = 0)
```cpp
// BrokerBuy2 in NT8Plugin.cpp
const char* orderType = "MARKET";
double limitPrice = 0.0;
```

### Limit Orders (OrderMode = 1, 3)
```cpp
if (Limit > 0) {
    orderType = "LIMIT";
    limitPrice = Limit;
}
```

### What's Missing for Full Support

1. **FOK Orders** - Not currently sent to NT
   - Would need: `orderType = "FOK"`
   - Or: Use IOC with special handling

2. **Stop Orders** - Partially implemented
   ```cpp
   double stopPrice = 0.0;
   // StopDist not currently used
   ```

3. **GTC Time-In-Force** - Sent but not distinguished
   ```cpp
   const char* tif = GetTimeInForce(g_orderType);
   // Returns: "GTC", "IOC", "FOK", or "DAY"
   ```

## Testing Workflow with TradeTest.c

### Test 1: Basic Limit Orders (Works on Holidays!)

1. Start NinjaTrader (Simulation mode)
2. Enable ZorroBridge AddOn
3. Run Zorro: `Zorro -run TradeTest`
4. Click "MKT Order" ? becomes "LMT Order"
5. Adjust "Limit" slider (e.g., 500 = 0.5 spreads)
6. Click "Buy Long"

**Expected:**
- Order placed with limit offset from market
- Check NT Orders window for limit price
- Verify fill or pending status

### Test 2: Adaptive Orders

1. Click "LMT Order" ? becomes "Adaptive"
2. Click "Buy Long"
3. Watch console for adaptation messages:
   ```
   Trade ... Limit 6905.25
   (if missed)
   Trade ... Limit 6905.50
   ```

### Test 3: GTC Orders

1. Click "Adaptive" ? becomes "GTC Order"
2. Click "Buy Long"
3. Verify order stays open (doesn't cancel at bar end)

## Diagnostic Output to Monitor

With `VERBOSE 2` and `LOG_TRADES` enabled:

```
Buy Long MESH26
Trade: TradeTest MESH26 ... Limit 6905.25

# On each tick:
123: Lots 1 Target 1 Open 6905.25 Close 0.00000
```

## Current Plugin Status

? **Supported:**
- Market orders
- Limit orders (entry & exit)
- Basic GTC handling

?? **Partially Supported:**
- Stop orders (structure ready, needs testing)
- Order modification (limit adaptation)

? **Not Supported:**
- FOK orders (needs NinjaScript support)
- Order books (GET_BOOK command)
- Tick-level volume (SET_VOLTYPE,4)

## Recommendations

### For Testing Limit Orders Now

Use TradeTest.c with:
```c
#define ASSET "MESH26"
#define VERBOSE 2
#define LOG_TRADES
#define MAXLOTS 10
```

Then manually:
1. Switch to "LMT Order" mode
2. Set Limit slider to 500 (0.5 spreads offset)
3. Click Buy/Sell buttons
4. Observe NT8 Orders window

### For Automated Testing (Future)

Create `LimitOrderTest.c` based on AutoTradeTest.c:
- Set `OrderMode = 1` (limit orders)
- Set `OrderLimit` before each trade
- Test during market hours
- Verify fills at expected prices

## Files Referenced

- `scripts/TradeTest.c` - Manual test UI
- `src/NT8Plugin.cpp` - BrokerBuy2 implementation
- `src/TcpBridge.cpp` - Command() for order placement

## Next Steps

1. **Test limit orders manually** with TradeTest.c (works on holidays!)
2. **Verify in NinjaTrader** Orders window shows correct limit prices
3. **Monitor fills** - do they execute at limit price or better?
4. **Check diagnostics** - any errors in NT Output window?
5. **Document results** - create test report showing:
   - Order type sent
   - Limit price vs fill price
   - Time to fill
   - Any errors

This manual testing will help verify our limit order implementation works correctly before automating it.
