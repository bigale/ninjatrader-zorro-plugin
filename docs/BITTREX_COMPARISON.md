# NT8 vs Bittrex (Official Zorro Plugin) Comparison

## Overview

This document analyzes the **official Bittrex plugin** that ships with Zorro to learn patterns and conventions used by the Zorro team.

**Source**: Zorro's included Bittrex plugin (`zorro/Source/BittrexPlugin/Bittrex.cpp`)  
**Author**: oP group Germany (Zorro developers)  
**Significance**: This represents the **official reference implementation** for Zorro broker plugins

## 1. Project Structure & Setup

### Bittrex Plugin Structure
```cpp
// Header
#include "stdafx.h"
#include <string>
#include <math.h>
#include <ATLComTime.h>
#include <zorro.h>

#define PLUGIN_TYPE	2
#define PLUGIN_NAME	"Bittrex V3"
#define PLUGIN_VERSION "3.02"
#define DLLFUNC extern "C" __declspec(dllexport)
```

### NT8 Plugin Structure
```cpp
// Header
#include "NT8Plugin.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <vector>
#include <sstream>

#define PLUGIN_NAME "NT8"
#define DLLFUNC extern "C" __declspec(dllexport)
```

### Comparison

| Aspect | Bittrex (Official) | NT8 |
|--------|-------------------|-----|
| **Header** | Uses `<zorro.h>` | Uses `"trading.h"` |
| **Version Tracking** | `PLUGIN_VERSION` macro | `PLUGIN_VERSION` constant |
| **Plugin Type** | `PLUGIN_TYPE 2` (crypto) | Returns version number |
| **Dependencies** | ATL, WinHttp | Winsock2 |

**Key Insight**: Official plugins define `PLUGIN_TYPE` and `PLUGIN_VERSION` macros.

### Recommendation
? **Add version tracking**:

```cpp
// NT8Plugin.h
#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 0
#define PLUGIN_VERSION_PATCH 0
#define PLUGIN_VERSION ((PLUGIN_VERSION_MAJOR << 16) | (PLUGIN_VERSION_MINOR << 8) | PLUGIN_VERSION_PATCH)
```

---

## 2. Global State Management

### Bittrex: Parameter Singleton Pattern

```cpp
struct GLOBAL {
	int Diag;
	int HttpId;
	int PriceType, VolType, OrderType;
	double Unit;
	char Url[256];
	char Key[256], Secret[256];
	char Symbol[64], Uuid[256];
	char AccountId[16];
} G;  // Single global instance
```

**Pattern**: All state in one struct, single global instance `G`.

**Benefits**:
- All state in one place
- Easy to reset (`memset(&G, 0, sizeof(G))`)
- Clear initialization
- No memory allocation needed

**Drawbacks**:
- Fixed-size buffers (C-style)
- No RAII cleanup
- Not type-safe

### NT8: Scattered Globals

```cpp
TcpBridge* g_bridge = nullptr;
int (__cdecl *BrokerMessage)(const char* text) = nullptr;
int (__cdecl *BrokerProgress)(const int progress) = nullptr;

static std::string g_account;
static std::map<int, OrderInfo> g_orders;
static std::map<std::string, int> g_orderIdMap;
static int g_nextOrderNum = 1000;
static bool g_connected = false;
```

**Pattern**: Individual global variables, C++ containers.

**Benefits**:
- Type-safe (std::string, std::map)
- RAII cleanup
- Dynamic allocation

**Drawbacks**:
- Scattered state
- Manual initialization

### Recommendation
?? **Consider hybrid approach**:

```cpp
struct PluginState {
    // Configuration
    int diagLevel = 0;
    int orderType = ORDER_GTC;
    bool connected = false;
    
    // Account state
    std::string account;
    std::string currentSymbol;
    
    // Order tracking
    std::map<int, OrderInfo> orders;
    std::map<std::string, int> orderIdMap;
    int nextOrderNum = 1000;
    
    void reset() {
        connected = false;
        account.clear();
        currentSymbol.clear();
        orders.clear();
        orderIdMap.clear();
        nextOrderNum = 1000;
    }
};

static PluginState g_state;
static std::unique_ptr<TcpBridge> g_bridge;
```

---

## 3. Utility Functions

### Bittrex: Rich Utility Library

```cpp
// Display messages
void showMsg(const char* Text, const char* Detail);

// Number formatting
char* i64toa(__int64 n);
char* ftoa(double f);  // Smart precision based on value

// Keep Zorro responsive
int sleep(int ms);

// Connection check
inline BOOL isConnected();

// Symbol conversion
char* fixSymbol(const char* Symbol);  // ETH/BTC -> ETH-BTC

// Amount rounding
double fixAmount(double Val);  // Round to power of 10
```

**Key Features**:
- **Smart formatting** (`ftoa` adjusts precision based on value)
- **Responsive waiting** (`sleep` calls `BrokerProgress`)
- **Symbol normalization** (converts formats)
- **Value rounding helpers**

### NT8: Basic Utilities

```cpp
void LogMessage(const char* format, ...);
void LogError(const char* format, ...);
DATE ConvertUnixToDATE(__time64_t unixTime);
__time64_t ConvertDATEToUnix(DATE date);

// Helpers
static const char* GetTimeInForce(int orderType);
static double CalculateStopPrice(int amount, double currentPrice, double stopDist);
```

**Key Features**:
- Logging functions
- Date conversion
- Order-specific helpers

### Comparison

| Function Type | Bittrex | NT8 |
|--------------|---------|-----|
| **Logging** | `showMsg()` | `LogMessage()`, `LogError()` |
| **Number Format** | `ftoa()`, `i64toa()` | sprintf |
| **Sleep** | Custom with progress | Win32 `Sleep()` |
| **Symbol Conversion** | `fixSymbol()` | Done in TcpBridge |
| **Connection Check** | `isConnected()` inline | `g_connected` check |

### Recommendation
? **Add responsive sleep**:

```cpp
// Keep Zorro responsive during waits
static int responsiveSleep(int ms) {
    Sleep(ms);
    if (BrokerProgress) {
        return BrokerProgress(0);
    }
    return 1;
}

// Use in BrokerBuy2:
for (int i = 0; i < 10; i++) {
    if (!responsiveSleep(100)) break;  // User cancelled
    // Check fill status...
}
```

---

## 4. HTTP Communication Pattern

### Bittrex: Comprehensive HTTP Handler

```cpp
char* send(const char* Path, int Mode = 0, const char* Method = NULL, const char* Body = NULL)
{
    static char URL[1024], Header[2048], Signature[1024],
        Buffer1[1024*1024], Buffer2[2048];
    
    // Build URL
    sprintf_s(URL, "%s%s", RHEADER, Path);
    
    // Authentication header (if Mode & 1)
    if (Mode & 1) {
        strcpy_s(Header, "Content-Type:application/json");
        // ... build HMAC signature ...
        strcat_s(Header, "\nApi-Signature: ");
        strcat_s(Header, hmac(Signature, 0, G.Secret, 512));
    }
    
    // Send request
    int Id = http_request(URL, Body, Header, Method);
    if (!Id) goto send_error;
    
    // Wait for response (30 seconds max)
    int Size = 0, Wait = 3000;
    while (!(Size = http_status(Id)) && --Wait > 0)
        if (!sleep(10)) goto send_error;  // Responsive wait!
    
    if (!Size) goto send_error;
    if (!http_result(Id, Response, MaxSize))
        goto send_error;
    
    Response[MaxSize-1] = 0;  // Prevent buffer overrun
    
    // Keep connection or free
    if (Mode & 1) http_free(Id);
    else G.HttpId = Id;  // Keep stream open
    
    return Response;

send_error:
    if (Id) http_free(Id);
    G.HttpId = 0;
    if (G.Diag >= 1)
        showMsg("Failed:", Path);
    return NULL;
}
```

**Key Patterns**:
1. **Static buffers** for responses (reusable)
2. **Responsive waiting** (`sleep(10)` in loop)
3. **Connection reuse** (keeps HTTP stream open)
4. **Error handling** with `goto` cleanup
5. **Buffer overflow protection** (`Response[MaxSize-1] = 0`)
6. **Conditional diagnostics** (`if (G.Diag >= 1)`)

### NT8: TCP Socket Communication

```cpp
std::string TcpBridge::SendCommand(const std::string& command)
{
    if (!m_connected || m_socket == INVALID_SOCKET) {
        return "ERROR:Not connected";
    }
    
    // Send command
    std::string fullCommand = command + "\n";
    int sent = send(m_socket, fullCommand.c_str(), (int)fullCommand.length(), 0);
    if (sent == SOCKET_ERROR) {
        m_connected = false;
        return "ERROR:Send failed";
    }
    
    // Receive response
    char buffer[4096];
    int received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    if (received == SOCKET_ERROR || received == 0) {
        m_connected = false;
        return "ERROR:Receive failed";
    }
    
    buffer[received] = '\0';
    m_lastResponse = std::string(buffer);
    
    // Remove trailing newline
    if (!m_lastResponse.empty() && m_lastResponse.back() == '\n') {
        m_lastResponse.pop_back();
    }
    
    return m_lastResponse;
}
```

**Key Patterns**:
1. **std::string** for dynamic responses
2. **Immediate send/receive** (no async)
3. **Simple error handling** (returns error string)
4. **Connection state tracking**

### Comparison

| Aspect | Bittrex (HTTP) | NT8 (TCP) |
|--------|----------------|-----------|
| **Waiting** | Responsive loop with progress | Blocking recv |
| **Buffers** | Static, reusable | Dynamic std::string |
| **Connection** | Pooling (keeps open) | Single persistent |
| **Error Handling** | goto cleanup | Return error string |
| **Diagnostics** | Level-based logging | Always log errors |

### Recommendation
? **Add responsive waiting to TCP receive**:

```cpp
std::string TcpBridge::SendCommand(const std::string& command) {
    // ... send code ...
    
    // Receive with timeout and responsiveness
    char buffer[4096];
    int received = 0;
    int attempts = 100;  // 10 seconds max
    
    while (attempts-- > 0) {
        // Try non-blocking receive
        received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) break;
        
        // Check for error vs would-block
        if (received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                m_connected = false;
                return "ERROR:Receive failed";
            }
        }
        
        // Wait 100ms and check if user wants to abort
        Sleep(100);
        if (BrokerProgress && !BrokerProgress(0)) {
            return "ERROR:Cancelled by user";
        }
    }
    
    // ... rest of function ...
}
```

---

## 5. BrokerBuy2 Implementation

### Bittrex: JSON Body Construction

```cpp
DLLFUNC int BrokerBuy2(char* Symbol, int Volume, double StopDist, double Limit, 
    double* pPrice, int* pFill)
{
    if (!isConnected() || !Volume) return 0;
    
    // Compose JSON body
    char Body[256] = "{\n";
    strcat_s(Body, "\"marketsymbol\": \"");
    strcat_s(Body, fixSymbol(Symbol));
    strcat_s(Body, "\",\n\"direction\": \"");
    strcat_s(Body, Volume > 0 ? "BUY" : "SELL");
    strcat_s(Body, "\",\n\"type\": \"");
    strcat_s(Body, Limit > 0. ? "LIMIT" : "MARKET");
    strcat_s(Body, "\",\n\"quantity\": \"");
    
    double Size = labs(Volume);
    if (G.Unit < 1.) Size *= G.Unit;
    strcat_s(Body, ftoa(Size));
    
    if (Limit > 0.) {
        strcat_s(Body, "\",\n\"limit\": \"");
        strcat_s(Body, ftoa(Limit));
    }
    
    strcat_s(Body, "\",\n\"timeInForce\": \"");
    if ((G.OrderType & 2) && Limit > 0.)
        strcat_s(Body, "GOOD_TIL_CANCELLED");
    else if (G.OrderType & 1)
        strcat_s(Body, "FILL_OR_KILL");
    else
        strcat_s(Body, "IMMEDIATE_OR_CANCEL");
    
    strcat_s(Body, "\"\n}");
    
    // Send order
    char* Response = send("orders", 1, 0, Body);
    if (!Response) return 0;
    
    // Extract UUID and fill info
    char* Uuid = strtext(Response, "id", "");
    if (!*Uuid) {
        showMsg("Failed:", Response);
        return 0;
    }
    strcpy(G.Uuid, Uuid);
    
    double Filled = strvar(Response, "fillQuantity", 0);
    if (Filled == 0. && !(G.OrderType & 2)) {
        showMsg("FOK/IOC order", "unfilled");
        return 0;
    }
    
    double Price = strvar(Response, "proceeds", 0);
    if (Filled > 0. && Price > 0. && pPrice)
        *pPrice = Price / Filled;
    if (*pFill) *pFill = Filled / min(1., G.Unit);
    
    return -1;  // Special: get ID with GET_UUID
}
```

**Key Patterns**:
1. **Manual JSON construction** (no JSON library)
2. **Order type from G.OrderType flags** (bitmask)
3. **Time-in-force handling** (GTC/FOK/IOC)
4. **Returns -1** to signal "get UUID via BrokerCommand"
5. **Handles unfilled FOK/IOC** orders gracefully
6. **Volume scaling** with `G.Unit`

### NT8: TCP Command Construction

```cpp
DLLFUNC int BrokerBuy2(char* Asset, int Amount, double StopDist, double Limit,
    double* pPrice, int* pFill)
{
    // ... validation ...
    
    // Determine order type
    const char* orderType = "MARKET";
    double limitPrice = 0.0;
    double stopPrice = 0.0;
    
    if (StopDist > 0) {
        double currentPrice = g_bridge->GetLast(Asset);
        if (currentPrice > 0) {
            stopPrice = CalculateStopPrice(Amount, currentPrice, StopDist);
            orderType = (Limit > 0) ? "STOPLIMIT" : "STOP";
            if (Limit > 0) limitPrice = Limit;
        }
    } else if (Limit > 0) {
        orderType = "LIMIT";
        limitPrice = Limit;
    }
    
    // Place order via TcpBridge
    int result = g_bridge->Command(
        "PLACE", g_account.c_str(), Asset, action, quantity,
        orderType, limitPrice, stopPrice, tif, "", orderId.c_str(), "", "");
    
    if (result != 0) {
        LogError("Order placement failed");
        return 0;
    }
    
    // Wait for fill (market orders only)
    if (strcmp(orderType, "MARKET") == 0) {
        for (int i = 0; i < 10; i++) {
            Sleep(100);
            int filled = g_bridge->Filled(orderId.c_str());
            if (filled > 0) {
                // ... update fill info ...
                break;
            }
        }
    }
    
    return numericId;  // Returns numeric ID directly
}
```

**Key Patterns**:
1. **Order type determination** (prioritized logic)
2. **Helper function** for stop price calculation
3. **Returns numeric ID** directly
4. **Polls for fills** (market orders only)
5. **Stop order support** (Bittrex doesn't have this)

### Comparison

| Aspect | Bittrex | NT8 |
|--------|---------|-----|
| **Order Construction** | Manual JSON string | Parameters to Command() |
| **TIF Handling** | Bitmask flags | String constant |
| **ID Return** | -1 + GET_UUID | Numeric ID |
| **Fill Waiting** | Checks immediately | Polls 10x for market |
| **Stop Orders** | ? Not supported | ? Supported |
| **Volume Scaling** | Uses G.Unit | Uses Lots directly |

### Recommendation
? **Current NT8 approach is cleaner**. Bittrex's manual JSON is error-prone.

?? **Consider** adding Order Type flags like Bittrex:

```cpp
// In BrokerCommand SET_ORDERTYPE:
// Bit 0: FOK/IOC flag
// Bit 1: GTC flag
g_orderType = (int)dwParameter;

// In BrokerBuy2:
const char* tif = "DAY";
if (g_orderType & 2)
    tif = "GTC";
else if (g_orderType & 1)
    tif = "FOK";
```

---

## 6. Order Tracking Pattern

### Bittrex: UUID in Global State

```cpp
struct GLOBAL {
    char Uuid[256];  // Last order UUID
    // ...
} G;

// BrokerBuy2 stores UUID:
strcpy(G.Uuid, Uuid);
return -1;  // Signal: get UUID via GET_UUID

// BrokerTrade uses stored UUID:
DLLFUNC int BrokerTrade(int nTradeID, /*...*/) {
    if (!isConnected() || !*G.Uuid) return 0;
    
    sprintf_s(G.Url, "orders/%s", G.Uuid);
    char* Response = send(G.Url, 1);
    // ...
}

// BrokerCommand provides UUID access:
case GET_UUID: 
    strcpy_s((char*)Parameter, 256, G.Uuid);
    return 1;
case SET_UUID: 
    strcpy_s(G.Uuid, (char*)Parameter);
    return 1;
```

**Pattern**: 
- Stores **one** order UUID globally
- Returns -1 from BrokerBuy2
- Zorro calls GET_UUID to retrieve it
- Subsequent calls use SET_UUID to switch orders

**Limitation**: Only tracks one order at a time!

### NT8: Map-Based Multi-Order Tracking

```cpp
static std::map<int, OrderInfo> g_orders;
static std::map<std::string, int> g_orderIdMap;
static int g_nextOrderNum = 1000;

// BrokerBuy2 returns numeric ID:
int numericId = RegisterOrder(orderId.c_str(), info);
return numericId;

// BrokerTrade looks up by numeric ID:
DLLFUNC int BrokerTrade(int nTradeID, /*...*/) {
    OrderInfo* order = GetOrder(nTradeID);
    if (!order) return NAY;
    
    const char* status = g_bridge->OrderStatus(order->orderId.c_str());
    // ...
}
```

**Pattern**:
- Tracks **multiple** orders simultaneously
- Maps numeric ID ? NT order ID
- Stores full order info (price, quantity, status, etc.)

**Advantage**: Supports multiple open orders!

### Comparison

| Aspect | Bittrex | NT8 |
|--------|---------|-----|
| **Tracking** | Single UUID | Multiple orders |
| **ID Type** | String (UUID) | Numeric mapping |
| **Storage** | Global char array | std::map |
| **Return Pattern** | -1 + GET_UUID | Direct numeric ID |
| **Order Info** | Query each time | Cached locally |

### Recommendation
? **NT8's multi-order tracking is superior** for strategies with multiple positions.

**Bittrex's pattern is simpler** but limited. Good for single-position strategies only.

---

## 7. BrokerCommand Extended Commands

### Bittrex: Comprehensive Command Set

```cpp
DLLFUNC double BrokerCommand(int Mode, intptr_t Parameter)
{
    switch (Mode) {
        case GET_COMPLIANCE: return 2;  // Hedging allowed
        case GET_MAXREQUESTS: return 2;  // 2 requests/second
        
        case SET_DIAGNOSTICS: 
            G.Diag = Parameter;
            return 1;
        
        case SET_AMOUNT: 
            G.Unit = *(double*)Parameter;
            if (G.Unit <= 0.) G.Unit = 0.00001;
            return 1;
        
        case GET_UUID: 
            strcpy_s((char*)Parameter, 256, G.Uuid);
            return 1;
        
        case SET_UUID: 
            strcpy_s(G.Uuid, (char*)Parameter);
            return 1;
        
        case SET_VOLTYPE: 
            G.VolType = Parameter;
            return 1;
        
        case SET_PRICETYPE: 
            G.PriceType = Parameter;
            return 1;
        
        case SET_ORDERTYPE: 
            G.OrderType = Parameter;
            return Parameter & 3;
        
        case GET_POSITION: {
            double Value = 0;
            char* Coin = fixSymbol((char*)Parameter);
            char* End = strchr(Coin, '-');
            if (End) {
                *End = 0;
                BrokerAccount(Coin, &Value, NULL, NULL);
            }
            return Value / min(1., G.Unit);
        }
        
        case DO_CANCEL: {
            if (Parameter == 0) {
                // Cancel all open orders
                char* Response = send("orders/open", 1, "DELETE");
                if (Response) return 1;
            } else {
                // Cancel specific order
                sprintf_s(G.Url, "orders/%s", G.Uuid);
                char* Response = send(G.Url, 1, "DELETE");
                if (Response) return 1;
            }
            return 0;
        }
    }
    return 0.;
}
```

**Supported Commands**:
- ? GET_COMPLIANCE (hedging rules)
- ? GET_MAXREQUESTS (rate limiting)
- ? SET_DIAGNOSTICS (logging level)
- ? SET_AMOUNT (contract size)
- ? GET/SET_UUID (order ID management)
- ? SET_VOLTYPE (volume type)
- ? SET_PRICETYPE (price type)
- ? SET_ORDERTYPE (TIF flags)
- ? GET_POSITION (query position)
- ? DO_CANCEL (cancel orders)

### NT8: Basic Command Set

```cpp
DLLFUNC double BrokerCommand(int Command, DWORD dwParameter)
{
    switch (Command) {
        case GET_COMPLIANCE:
            return NFA_COMPLIANT;  // Position netting
        
        case GET_BROKERZONE:
            return -5;  // EST timezone
        
        case GET_MAXTICKS:
            return 0;  // No historical data
        
        case GET_POSITION: {
            const char* symbol = (const char*)dwParameter;
            return (double)g_bridge->MarketPosition(symbol, g_account.c_str());
        }
        
        case GET_AVGENTRY: {
            const char* symbol = (const char*)dwParameter;
            return g_bridge->AvgEntryPrice(symbol, g_account.c_str());
        }
        
        case SET_ORDERTYPE:
            g_orderType = (int)dwParameter;
            return 1;
        
        case SET_SYMBOL:
            if (dwParameter) {
                g_currentSymbol = (const char*)dwParameter;
            }
            return 1;
        
        case DO_CANCEL: {
            int orderId = (int)dwParameter;
            OrderInfo* order = GetOrder(orderId);
            if (order) {
                int result = g_bridge->CancelOrder(order->orderId.c_str());
                return (result == 0) ? 1 : 0;
            }
            return 0;
        }
        
        case GET_WAIT:
            return 50;  // 50ms polling
    }
    return 0;
}
```

**Supported Commands**:
- ? GET_COMPLIANCE
- ? GET_BROKERZONE
- ? GET_MAXTICKS
- ? GET_POSITION
- ? GET_AVGENTRY
- ? SET_ORDERTYPE
- ? SET_SYMBOL
- ? DO_CANCEL
- ? GET_WAIT

**Missing** (vs Bittrex):
- ? GET_MAXREQUESTS (rate limiting)
- ? SET_DIAGNOSTICS (logging level)
- ? SET_AMOUNT (contract size)
- ? GET/SET_UUID (not needed - returns ID directly)
- ? SET_VOLTYPE / SET_PRICETYPE

### Recommendation
?? **Consider adding**:

```cpp
case GET_MAXREQUESTS:
    return 20.0;  // 20 requests/second (localhost TCP is fast)

case SET_DIAGNOSTICS:
    g_diagLevel = (int)dwParameter;
    LogMessage("# Diagnostic level set to %d", g_diagLevel);
    return 1;
```

---

## 8. Error Handling Philosophy

### Bittrex: goto-Based Cleanup

```cpp
char* send(const char* Path, int Mode, const char* Method, const char* Body)
{
    // ... setup ...
    
    int Id = http_request(URL, Body, Header, Method);
    if (!Id) goto send_error;
    
    int Size = 0, Wait = 3000;
    while (!(Size = http_status(Id)) && --Wait > 0)
        if (!sleep(10)) goto send_error;
    
    if (!Size) goto send_error;
    if (!http_result(Id, Response, MaxSize))
        goto send_error;
    
    // ... success path ...
    return Response;

send_error:
    if (Id) http_free(Id);
    G.HttpId = 0;
    if (G.Diag >= 1)
        showMsg("Failed:", Path);
    return NULL;
}
```

**Pattern**: `goto` for cleanup (common in C)

**Benefits**:
- Single cleanup point
- No resource leaks
- Clear error path

**Drawbacks**:
- Considered "bad style" by some
- Can be confusing if overused

### NT8: Early Returns

```cpp
std::string TcpBridge::SendCommand(const std::string& command)
{
    if (!m_connected || m_socket == INVALID_SOCKET) {
        return "ERROR:Not connected";
    }
    
    int sent = send(m_socket, /*...*/);
    if (sent == SOCKET_ERROR) {
        m_connected = false;
        return "ERROR:Send failed";
    }
    
    int received = recv(m_socket, /*...*/);
    if (received == SOCKET_ERROR || received == 0) {
        m_connected = false;
        return "ERROR:Receive failed";
    }
    
    // ... success path ...
    return m_lastResponse;
}
```

**Pattern**: Early returns, RAII for cleanup

**Benefits**:
- Clear linear flow
- RAII handles cleanup automatically
- Modern C++ style

**Drawbacks**:
- Multiple return points
- Could miss cleanup if not careful

### Comparison

| Aspect | Bittrex | NT8 |
|--------|---------|-----|
| **Error Pattern** | goto cleanup | Early return |
| **Resource Management** | Manual free | RAII |
| **Style** | C-style | C++-style |
| **Clarity** | Single exit | Multiple exits |

### Recommendation
? **Both patterns are valid**. NT8's RAII approach is more modern and safer.

---

## 9. Key Insights from Official Plugin

### ? Things Bittrex Does Well

1. **Parameter Singleton** (`struct GLOBAL`)
   - All state in one place
   - Easy to reset
   - Clear organization

2. **Responsive Waiting**
   ```cpp
   while (!(Size = http_status(Id)) && --Wait > 0)
       if (!sleep(10)) goto send_error;
   ```
   - Calls `BrokerProgress(0)` during waits
   - Allows user to cancel
   - Keeps Zorro UI responsive

3. **Smart Number Formatting**
   ```cpp
   char* ftoa(double f) {
       if (f < 1.) sprintf(buffer, "%.8f", f);
       else if (f < 30.) sprintf(buffer, "%.4f", f);
       // ...
   }
   ```
   - Precision based on value
   - Cleaner output

4. **Diagnostic Levels**
   ```cpp
   if (G.Diag >= 2) showMsg("Send:", Path);
   if (G.Diag >= 1) showMsg("Failed:", Path);
   ```
   - Configurable verbosity
   - Less spam in production

5. **Connection Reuse**
   ```cpp
   if (Mode & 1) http_free(Id);
   else G.HttpId = Id;  // Keep stream open
   ```
   - Performance optimization
   - Reduces overhead

### ? Things NT8 Does Better

1. **Multi-Order Tracking**
   - Bittrex: Single UUID only
   - NT8: Full order map

2. **Type Safety**
   - Bittrex: C-style char arrays
   - NT8: std::string, std::map

3. **Stop Order Support**
   - Bittrex: None
   - NT8: STOP and STOPLIMIT

4. **Return Pattern**
   - Bittrex: -1 + GET_UUID
   - NT8: Direct numeric ID (cleaner)

5. **Modern C++**
   - RAII cleanup
   - STL containers
   - Better error handling

---

## 10. Actionable Recommendations

### High Priority ?

1. **Add Responsive Sleep**
   ```cpp
   static int responsiveSleep(int ms) {
       Sleep(ms);
       return BrokerProgress ? BrokerProgress(0) : 1;
   }
   ```
   - Use in order fill polling
   - Allow user cancellation
   - Keep Zorro responsive

2. **Add Diagnostic Levels**
   ```cpp
   case SET_DIAGNOSTICS:
       g_diagLevel = (int)dwParameter;
       return 1;
   
   // Use in logging:
   if (g_diagLevel >= 2)
       LogMessage("[DEBUG] Order details: ...");
   ```

3. **Add Version Macros**
   ```cpp
   #define PLUGIN_VERSION_MAJOR 1
   #define PLUGIN_VERSION_MINOR 0
   #define PLUGIN_VERSION_PATCH 0
   ```

### Medium Priority ??

4. **Consider Parameter Struct**
   ```cpp
   struct PluginState {
       int diagLevel;
       int orderType;
       bool connected;
       std::string account;
       // ...
   };
   static PluginState g_state;
   ```

5. **Add GET_MAXREQUESTS**
   - Return reasonable rate limit
   - Help Zorro throttle requests

### Low Priority ??

6. **Smart Number Formatting** (if needed)
   - Only if log output becomes cluttered
   - Current sprintf is fine

---

## 11. Summary

### What We Learned

**From Bittrex Official Plugin:**
- ? Parameter singleton pattern
- ? Responsive waiting during network calls
- ? Diagnostic levels for logging
- ? Connection reuse for performance
- ? goto-based cleanup (valid C pattern)
- ? Smart number formatting
- ? Comprehensive BrokerCommand support

**NT8 Advantages:**
- ? Modern C++ with RAII
- ? Multi-order tracking
- ? Stop order support
- ? Type-safe containers
- ? Cleaner ID return pattern

**NT8 Gaps:**
- ?? No responsive sleep
- ?? No diagnostic levels
- ?? No version tracking
- ?? Missing some BrokerCommand cases

### Conclusion

The **Bittrex official plugin** demonstrates proven patterns from the Zorro team:
- Simplicity over complexity
- C-style but effective
- Performance-conscious
- User experience focused (responsive waiting)

**NT8 plugin** uses modern C++ patterns but should adopt:
1. **Responsive sleep** (user can cancel long operations)
2. **Diagnostic levels** (configurable verbosity)
3. **Version tracking** (for support/debugging)

**Status**: NT8 plugin is **architecturally sound** with **modern advantages**. Recommended improvements are **UX enhancements** inspired by official Zorro patterns.

---

## 12. Quick Wins to Implement

```cpp
// 1. Responsive sleep
static int responsiveSleep(int ms) {
    Sleep(ms);
    return BrokerProgress ? BrokerProgress(0) : 1;
}

// 2. Diagnostic level
static int g_diagLevel = 0;

case SET_DIAGNOSTICS:
    g_diagLevel = (int)dwParameter;
    return 1;

// 3. Conditional logging
#define LOG_DEBUG(msg) if (g_diagLevel >= 2) LogMessage(msg)
#define LOG_INFO(msg) if (g_diagLevel >= 1) LogMessage(msg)

// 4. Version info
#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 0
#define PLUGIN_VERSION_PATCH 0

// In BrokerOpen:
LogMessage("# NT8 Plugin v%d.%d.%d", 
    PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH);
```

**Result**: Better UX matching official Zorro plugin standards! ?
