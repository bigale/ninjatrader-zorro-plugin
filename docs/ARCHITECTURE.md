# Architecture Overview

## Current Implementation: TCP Bridge (v1.0)

**Status:** ? Production Ready

This plugin uses a **modern TCP bridge architecture** that works with NinjaTrader 8.1+.

---

## Architecture Diagram

```
???????????????????         ???????????????????         ????????????????????
?     Zorro       ?         ?   NT8.dll       ?         ?  NinjaTrader 8   ?
?  (Trading Bot)  ? ??????? ?  (C++ Plugin)   ? ??????? ?  (Trading        ?
?                 ?  Calls  ?                 ?   TCP   ?   Platform)      ?
?  - Strategies   ?         ?  - TcpBridge    ?  :8888  ?                  ?
?  - Scripts      ?         ?  - Order Mgmt   ?         ?  - ZorroBridge      ?
?  - Backtests    ?         ?  - Data Parsing ?         ?    AddOn (C#)    ?
???????????????????         ???????????????????         ?  - Market Data   ?
                                                          ?  - Orders        ?
                                                          ?  - Positions     ?
                                                          ????????????????????
```

---

## Components

### 1. Zorro Side
**Location:** User's trading scripts

**Files:**
- `C:\Zorro\Strategy\*.c` - Trading scripts
- `C:\Zorro\Plugin\NT8.dll` - Broker plugin

**Responsibilities:**
- Execute trading strategies
- Call broker API functions
- Manage positions and orders
- Calculate P&L

---

### 2. C++ Plugin (NT8.dll)
**Location:** `src/NT8Plugin.cpp`, `src/TcpBridge.cpp`

**Files:**
```
src/
??? NT8Plugin.cpp       # Main broker API implementation
??? TcpBridge.cpp       # TCP communication layer
??? NT8Plugin.def       # DLL exports

include/
??? NT8Plugin.h         # Plugin interface
??? TcpBridge.h         # TCP bridge interface
??? trading.h           # Zorro types
```

**Responsibilities:**
- Implement Zorro Broker API
- Manage TCP connection to AddOn
- Parse responses from NinjaTrader
- Track orders and positions
- Format commands for NT8

**Key Functions:**
- `BrokerOpen()` - Initialize
- `BrokerLogin()` - Connect to NT8
- `BrokerAsset()` - Subscribe to data
- `BrokerBuy2()` - Place orders
- `BrokerSell2()` - Close positions
- `BrokerAccount()` - Get account info

---

### 3. NinjaScript AddOn (ZorroBridge.cs)
**Location:** `ninjatrader-addon/ZorroBridge.cs`

**Installation:**
```
Copy to: Documents\NinjaTrader 8\bin\Custom\AddOns\
Compile: F5 in NinjaTrader
```

**Responsibilities:**
- Listen on TCP port 8888
- Handle commands from C++ plugin
- Execute NinjaTrader API calls
- Return market data
- Place/track orders
- Query positions

**Key Handlers:**
- `LOGIN` - Account validation
- `SUBSCRIBE` - Market data subscription
- `GETPRICE` - Real-time prices
- `PLACEORDER` - Order placement
- `GETPOSITION` - Position queries
- `GETACCOUNT` - Account info

---

## Communication Protocol

### TCP Commands (localhost:8888)

```
Plugin ? AddOn                    AddOn ? Plugin
??????????????                    ??????????????
PING                         ?    PONG
LOGIN:Sim101                 ?    OK:Logged in
SUBSCRIBE:MES 03-26          ?    OK:Subscribed
GETPRICE:MES 03-26           ?    PRICE:6047.50:6047.25:6047.75:1234
GETACCOUNT:Sim101            ?    ACCOUNT:100000:0:100000
GETPOSITION:MES 03-26:Sim101 ?    POSITION:2:6040.00
PLACEORDER:...               ?    ORDER:orderId123
LOGOUT                       ?    OK:Logged out
```

---

## Data Flow Examples

### Example 1: Get Price
```
1. Zorro calls BrokerAsset("MESH26", &price, ...)
2. NT8Plugin.cpp converts MESH26 ? MES 03-26
3. TcpBridge sends: "GETPRICE:MES 03-26"
4. ZorroBridge.cs queries NT8 MarketData
5. Returns: "PRICE:6047.50:6047.25:6047.75:1234"
6. TcpBridge parses response
7. NT8Plugin returns price to Zorro
```

### Example 2: Place Order
```
1. Zorro calls enterLong(1)
2. Calls BrokerBuy2("MESH26", 1, 0, 0, ...)
3. NT8Plugin creates order ID
4. TcpBridge sends: "PLACEORDER:Sim101:MES 03-26:BUY:1:MARKET:0:0:GTC:..."
5. ZorroBridge.cs creates NT8 order
6. Returns: "ORDER:abc123"
7. NT8Plugin tracks order
8. Waits for fill confirmation
9. Returns order ID to Zorro
```

---

## Why NOT ATI (NtDirect.dll)?

### Old Approach (Removed)
```
Zorro ? NT8.dll ? NtDirect.dll ? NinjaTrader
                   ? Broken on NT8 8.1+
```

**Problems:**
- NtDirect.dll is 32-bit only
- Not compatible with NT8 8.1+
- Deprecated by NinjaTrader
- Limited functionality

### New Approach (Current)
```
Zorro ? NT8.dll ? TCP:8888 ? ZorroBridge AddOn ? NinjaTrader
                   ? Works perfectly!
```

**Benefits:**
- Works with NT8 8.1+
- Modern architecture
- Clean separation
- Easy to debug
- Full API access

---

## Build Process

```bash
# 1. Build C++ plugin (32-bit required)
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A Win32 ..
cmake --build . --config Release

# 2. Install plugin
copy Release\NT8.dll C:\Zorro\Plugin\

# 3. Install AddOn
copy ninjatrader-addon\ZorroBridge.cs Documents\NinjaTrader 8\bin\Custom\AddOns\
# Then press F5 in NinjaTrader to compile

# 4. Configure Zorro
# Add to C:\Zorro\History\accounts.csv:
NT8-Sim,Sim101,,Demo
```

---

## Key Differences from ATI

| Feature | Old ATI | New TCP Bridge |
|---------|---------|----------------|
| NT8 Compatibility | 8.0 only | 8.1+ ? |
| Architecture | DLL ? DLL | DLL ? TCP ? AddOn |
| Dependencies | NtDirect.dll | None |
| Bit Version | 32-bit only | 32-bit plugin, any AddOn |
| Debugging | Difficult | Easy (see TCP traffic) |
| Flexibility | Limited | Full NT8 API access |
| Maintenance | Deprecated | Active |

---

## File Naming Confusion

**"ZorroBridge" AddOn name is historical:**
- Named for compatibility/recognition
- **NOT** using old ATI (NtDirect.dll)
- Actually a TCP bridge implementation

**Better mental model:**
- Think "Zorro Bridge for NinjaTrader"
- ATI in the name = "Automated Trading Interface" (generic term)
- Not = NinjaTrader's old ATI DLL

---

## Version History

### v1.0 (Current) - TCP Bridge
- ? TCP-based architecture
- ? NT8 8.1+ compatible
- ? Modern C# AddOn
- ? Full feature set
- ? Production ready

### v0.x (Deprecated) - ATI Approach
- ? Used NtDirect.dll
- ? NT8 8.0 only
- ? Limited functionality
- ? Removed from codebase

---

## Summary

**What you have:**
- Modern TCP bridge architecture ?
- Works with current NinjaTrader ?
- No legacy ATI dependencies ?
- Clean, maintainable code ?
- Production ready ?

**What was removed:**
- Old NtDirect.cpp/h files ?
- Legacy ATI approach ?
- Outdated architecture ?

**Confusion source:**
- "ZorroBridge" name (historical)
- But uses TCP bridge (modern)
- No actual ATI DLL dependency

---

**Your project is correctly using the modern TCP bridge approach!** ??
