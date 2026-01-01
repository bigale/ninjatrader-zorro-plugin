# API Reference

Complete API documentation for the NinjaTrader 8 Zorro Plugin.

## Overview

The plugin implements the Zorro Broker API for live trading via NinjaTrader 8.1+.

**Key Characteristics:**
- Live trading only (no historical data)
- Real-time market data
- Market and limit orders
- Position tracking
- TCP-based architecture (localhost:8888)

---

## Core Functions

### BrokerOpen
```c
int BrokerOpen(char* Name, FARPROC fpMessage, FARPROC fpProgress)
```

Initializes the plugin.

**Parameters:**
- `Name` - Buffer to receive plugin name ("NT8")
- `fpMessage` - Callback for messages
- `fpProgress` - Callback for progress updates

**Returns:** Plugin version number

**Called:** Once at Zorro startup

---

### BrokerLogin
```c
int BrokerLogin(char* User, char* Pwd, char* Type, char* Accounts)
```

Connects to NinjaTrader via TCP bridge.

**Parameters:**
- `User` - NinjaTrader account name (e.g., "Sim101")
- `Pwd` - Password (unused, pass NULL)
- `Type` - Account type (unused)
- `Accounts` - Buffer to receive account list

**Returns:**
- `1` - Login successful
- `0` - Login failed

**Example:**
```c
// In accounts.csv:
NT8-Sim,Sim101,,Demo

// Zorro calls:
BrokerLogin("Sim101", NULL, NULL, accounts_buffer);
```

**Connection Process:**
1. Connects to localhost:8888
2. Sends `LOGIN:Sim101`
3. Waits for confirmation
4. Returns success/failure

---

### BrokerTime
```c
int BrokerTime(DATE* pTimeUTC)
```

Gets current time and keeps connection alive.

**Parameters:**
- `pTimeUTC` - Pointer to receive current UTC time

**Returns:**
- `2` - Connected, market likely open
- `0` - Disconnected

**Called:** Every 50ms (polling interval)

**Note:** Returns local PC time as UTC (NT8 doesn't expose server time)

---

### BrokerAsset
```c
int BrokerAsset(char* Asset, double* pPrice, double* pSpread,
    double* pVolume, double* pPip, double* pPipCost, double* pLotAmount,
    double* pMargin, double* pRollLong, double* pRollShort, double* pCommission)
```

Subscribes to market data and retrieves prices.

**Two modes:**

#### Mode 1: Subscribe (pPrice == NULL)
```c
BrokerAsset("MESH26", NULL, ...);  // Subscribe only
```

**Returns:**
- `1` - Subscription successful
- `0` - Subscription failed

#### Mode 2: Get Prices (pPrice != NULL)
```c
double price, spread, volume;
BrokerAsset("MESH26", &price, &spread, &volume, ...);
```

**Parameters:**
- `Asset` - Symbol name (auto-converted: MESH26 ? MES 03-26)
- `pPrice` - Last/Ask price
- `pSpread` - Bid-Ask spread
- `pVolume` - Daily volume
- `pPip` - Tick size (0 = use asset file)
- `pPipCost` - Tick value (0 = use asset file)
- `pLotAmount` - Contract multiplier (returns 1)
- `pMargin` - Margin cost (0 = use asset file)
- `pRollLong/Short` - Rollover (unused for futures)
- `pCommission` - Commission (0 = use asset file)

**Returns:**
- `1` - Valid price data
- `0` - No data available

**Symbol Conversion:**
```
MESH26    ? MES 03-26
MES 0326  ? MES 03-26  
MES 03-26 ? MES 03-26 (no change)
```

---

### BrokerAccount
```c
int BrokerAccount(char* Account, double* pBalance, 
    double* pTradeVal, double* pMarginVal)
```

Retrieves account information.

**Parameters:**
- `Account` - Account name (NULL = use current)
- `pBalance` - Cash balance
- `pTradeVal` - Realized P&L
- `pMarginVal` - Available margin/buying power

**Returns:**
- `1` - Success
- `0` - Failed

**Example:**
```c
double balance, pnl, margin;
BrokerAccount(NULL, &balance, &pnl, &margin);

printf("Balance: $%.2f", balance);
printf("P&L: $%.2f", pnl);
printf("Margin: $%.2f", margin);
```

---

### BrokerBuy2
```c
int BrokerBuy2(char* Asset, int Amount, double StopDist, double Limit,
    double* pPrice, int* pFill)
```

Places orders.

**Parameters:**
- `Asset` - Symbol name
- `Amount` - Contracts (positive = buy, negative = sell)
- `StopDist` - Stop distance (currently unused)
- `Limit` - Limit price (0 = market order)
- `pPrice` - Filled price (output)
- `pFill` - Filled quantity (output)

**Returns:**
- `>0` - Order ID number
- `0` - Order failed

**Order Types:**

```c
// Market order
enterLong(1);  // Amount=1, Limit=0

// Limit order
OrderLimit = 6000.00;
enterLong(1);  // Amount=1, Limit=6000.00
```

**Fill Handling:**
- Market orders: waits up to 1 second for fill
- Limit orders: returns immediately, check status later

---

### BrokerSell2
```c
int BrokerSell2(int nTradeID, int nAmount, double Limit,
    double* pClose, double* pCost, double* pProfit, int* pFill)
```

Closes positions.

**Parameters:**
- `nTradeID` - Order ID from BrokerBuy2
- `nAmount` - Contracts to close (0 = close all)
- `Limit` - Limit price (0 = market)
- `pClose` - Fill price (output)
- `pCost` - Cost basis (output)
- `pProfit` - Realized profit (output)
- `pFill` - Filled quantity (output)

**Returns:**
- `>0` - Trade ID
- `0` - Close failed

**Example:**
```c
// Market close
exitLong();  // Limit=0

// Limit close
OrderLimit = 6100.00;
exitLong();  // Limit=6100.00
```

---

### BrokerTrade
```c
int BrokerTrade(int nTradeID, double* pOpen, double* pClose,
    double* pCost, double* pProfit)
```

Queries order/trade status.

**Parameters:**
- `nTradeID` - Order ID
- `pOpen` - Entry price (output)
- `pClose` - Current/exit price (output)
- `pCost` - Cost (output)
- `pProfit` - Unrealized/realized profit (output)

**Returns:**
- `>0` - Filled quantity
- `NAY` - Order cancelled/rejected/not found

---

## Broker Commands

### GET_POSITION
```c
int pos = brokerCommand(GET_POSITION, (long)Asset);
```

Returns current net position for symbol.

**Returns:**
- `> 0` - Long position (contracts)
- `< 0` - Short position (contracts)
- `0` - Flat (no position)

**Example:**
```c
int pos = brokerCommand(GET_POSITION, (long)"MESH26");
if(pos > 0)
    printf("Long %d contracts", pos);
else if(pos < 0)
    printf("Short %d contracts", -pos);
else
    printf("Flat");
```

---

### GET_AVGENTRY
```c
double entry = brokerCommand(GET_AVGENTRY, (long)Asset);
```

Returns average entry price for current position.

**Returns:** Average entry price, or 0 if no position

**Example:**
```c
int pos = brokerCommand(GET_POSITION, (long)Asset);
if(pos != 0) {
    double entry = brokerCommand(GET_AVGENTRY, (long)Asset);
    double current = priceClose();
    double pnl = (current - entry) * pos * 5;  // MES = $5/point
    printf("Unrealized P&L: $%.2f", pnl);
}
```

---

### GET_COMPLIANCE
```c
int nfa = brokerCommand(GET_COMPLIANCE, 0);
```

Returns compliance mode.

**Returns:** `NFA_COMPLIANT` (1) - Position netting enabled

---

### GET_BROKERZONE
```c
int tz = brokerCommand(GET_BROKERZONE, 0);
```

Returns broker timezone offset.

**Returns:** `-5` (EST timezone)

---

### GET_MAXTICKS
```c
int hist = brokerCommand(GET_MAXTICKS, 0);
```

Returns maximum historical ticks available.

**Returns:** `0` (no historical data via this plugin)

---

### GET_WAIT
```c
int poll = brokerCommand(GET_WAIT, 0);
```

Returns polling interval in milliseconds.

**Returns:** `50` (50ms polling rate)

---

### SET_ORDERTYPE
```c
brokerCommand(SET_ORDERTYPE, ORDER_GTC);
```

Sets time-in-force for orders.

**Values:**
- `ORDER_GTC` - Good-Till-Cancelled
- `ORDER_IOC` - Immediate-Or-Cancel
- `ORDER_FOK` - Fill-Or-Kill
- `ORDER_DAY` - Day order

**Default:** GTC

---

### SET_DIAGNOSTICS
```c
brokerCommand(SET_DIAGNOSTICS, 1);
```

Enables diagnostic logging.

**Parameter:**
- `1` - Enable diagnostics
- `0` - Disable diagnostics

**Use:** For debugging connection and order issues

---

### DO_CANCEL
```c
int result = brokerCommand(DO_CANCEL, orderID);
```

Cancels a pending order.

**Parameter:** Order ID from BrokerBuy2

**Returns:**
- `1` - Cancel successful
- `0` - Cancel failed

---

## Data Structures

### OrderInfo (Internal)
```cpp
struct OrderInfo {
    std::string orderId;      // NT order ID
    std::string instrument;   // Symbol
    std::string action;       // "BUY" or "SELL"
    int quantity;             // Order size
    int filled;               // Filled quantity
    double limitPrice;        // Limit price
    double stopPrice;         // Stop price
    double avgFillPrice;      // Average fill price
    std::string status;       // Order status
};
```

---

## Symbol Format Conversion

The plugin automatically converts between formats:

| Input Format | NT8 Format | Notes |
|--------------|------------|-------|
| MESH26 | MES 03-26 | Month code (H=March) |
| MES 0326 | MES 03-26 | MMYY format |
| MES 03-26 | MES 03-26 | Direct NT8 format |

**Month Codes:**
```
F=Jan, G=Feb, H=Mar, J=Apr, K=May, M=Jun,
N=Jul, Q=Aug, U=Sep, V=Oct, X=Nov, Z=Dec
```

---

## TCP Protocol

Communication between plugin and AddOn uses simple text protocol on localhost:8888.

### Commands

```
PING                           ? PONG
LOGIN:Sim101                   ? OK:Logged in to Sim101
SUBSCRIBE:MES 03-26            ? OK:Subscribed
GETPRICE:MES 03-26             ? PRICE:6047.50:6047.25:6047.75:12345
GETACCOUNT:Sim101              ? ACCOUNT:100000:0:100000
GETPOSITION:MES 03-26:Sim101   ? POSITION:2:6040.00
PLACEORDER:...                 ? ORDER:orderId
CANCELORDER:orderId            ? OK:Cancelled
LOGOUT                         ? OK:Logged out
```

---

## Error Handling

### Connection Errors

```c
// Check connection on startup
if(is(INITRUN) && !Live) {
    printf("\nFailed to connect to NinjaTrader");
    quit("Connection error");
}
```

### Order Errors

```c
// Check order placement
int orderId = enterLong(1);
if(orderId == 0) {
    printf("\nOrder failed - check logs");
}
```

### Data Errors

```c
// Check for valid price
if(priceClose() == 0) {
    printf("\nNo market data available");
}
```

---

## Limitations

| Feature | Status | Workaround |
|---------|--------|------------|
| Historical bars | ? Not available | Use separate data source |
| Contract specs | ?? Limited | Configure in asset file |
| Stop orders | ?? Planned | Use limit orders |
| OCO orders | ?? Planned | Manual management |
| Multiple accounts | ?? One at a time | Reconnect to switch |

---

## Performance

- **Polling rate:** 50ms (20 updates/second)
- **Order latency:** <100ms typical for market orders
- **Connection:** Localhost TCP (minimal latency)
- **Memory:** <5MB typical usage

---

## Thread Safety

The plugin is **not thread-safe**. All calls must be from Zorro's main thread.

---

## See Also

- [Installation Guide](INSTALLATION.md)
- [Getting Started](GETTING_STARTED.md)
- [Troubleshooting](TROUBLESHOOTING.md)
- [Zorro Manual](https://zorro-project.com/manual/)
- [NinjaTrader API](https://ninjatrader.com/support/helpGuides/nt8/)
