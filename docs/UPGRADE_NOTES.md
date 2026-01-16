# Plugin Upgrade Notes

## Version 1.0.0 - Foundation Improvements

### Overview

This upgrade implements UX and logging improvements inspired by analysis of the official Bittrex Zorro plugin and the third-party Alpaca plugin. All changes are **backward compatible** and follow Zorro team's proven patterns.

---

## What Changed

### 1. Version Tracking ?

**Added**: Version macros and display

```cpp
#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 0
#define PLUGIN_VERSION_PATCH 0
#define PLUGIN_VERSION_STRING "1.0.0"
```

**Log Output**:
```
# NT8 plugin v1.0.0 (TCP Bridge for NT8 8.1+)
```

**Benefit**: Clear version identification for support and debugging

---

### 2. Diagnostic Levels ?

**Added**: Configurable logging verbosity

```c
// In Zorro script:
brokerCommand(SET_DIAGNOSTICS, 0);  // Errors only (default)
brokerCommand(SET_DIAGNOSTICS, 1);  // Info + Errors
brokerCommand(SET_DIAGNOSTICS, 2);  // Debug + Info + Errors
```

**Levels**:
- **Level 0** (Default): Errors only - clean production logs
- **Level 1**: Important events (orders placed, fills) + errors
- **Level 2**: Detailed debug info (parameters, flow) + all above

**Benefit**: Reduces log spam in production, detailed logs when debugging

---

### 3. Responsive Operations ?

**Added**: User can cancel long waits

**What it does**:
- During market order fill polling, Zorro UI stays responsive
- User can press ESC or close Zorro to cancel
- Previously: UI frozen during waits

**Affected Functions**:
- `BrokerBuy2` - market order fills
- `BrokerSell2` - closing position fills

**Benefit**: Better UX, especially in slow markets or connection issues

---

### 4. Rate Limiting Support ?

**Added**: `GET_MAXREQUESTS` command

```c
double maxReq = brokerCommand(GET_MAXREQUESTS, 0);
// Returns: 20.0 (20 requests/second)
```

**Benefit**: Zorro can throttle requests if needed (localhost TCP is very fast)

---

## Phase 2: Architecture Refactoring ?

**Added in**: v1.0.0 (upgrades branch)

### 5. PluginState Struct ?

**Added**: Consolidated global state

```cpp
struct PluginState {
    // Configuration
    int diagLevel = 0;
    int orderType = ORDER_GTC;
    
    // Connection state
    bool connected = false;
    
    // Account state
    std::string account;
    std::string currentSymbol;
    
    // Order tracking
    std::map<int, OrderInfo> orders;
    std::map<std::string, int> orderIdMap;
    int nextOrderNum = 1000;
    
    void reset();  // Clean state on logout
};

static PluginState g_state;
```

**Benefits**:
- All state in one place (organized)
- Easy to reset entire state
- Better encapsulation
- Type-safe (C++ containers)
- Clear initialization with defaults

**All references updated**:
- `g_account` ? `g_state.account`
- `g_connected` ? `g_state.connected`
- `g_diagLevel` ? `g_state.diagLevel`
- `g_orders` ? `g_state.orders`
- etc.

---

### 6. std::unique_ptr for g_bridge ?

**Changed**: Raw pointer to smart pointer

```cpp
// Before:
TcpBridge* g_bridge = nullptr;

// After:
std::unique_ptr<TcpBridge> g_bridge;
```

**Benefits**:
- Automatic cleanup (no manual delete)
- Cannot be copied (prevents errors)
- Modern C++ best practice
- Memory safety guaranteed

**Initialization**:
```cpp
// In BrokerOpen:
g_bridge = std::make_unique<TcpBridge>();

// In DllMain cleanup:
g_bridge.reset();  // Automatic cleanup
```

**No API changes**: All usage (g_bridge->method()) stays the same

---

## Migration Guide

### For Existing Scripts

**? No changes required!**

All existing scripts continue to work unchanged. The improvements are:
- Opt-in (diagnostic levels default to 0)
- Backward compatible
- Non-breaking

### To Use New Features

#### Enable Verbose Logging

```c
function run()
{
    if (is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 1);  // Show important events
    }
    // ... rest of script ...
}
```

#### Debug Order Issues

```c
function run()
{
    if (is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug output
    }
    // ... rest of script ...
}
```

---

## Before vs After

### Log Output Comparison

**Before (always verbose)**:
```
# [BrokerBuy2] Called with Asset=MES 03-26, Amount=1, StopDist=0.00, Limit=0.00
# [BrokerBuy2] Pre-check passed
# [BrokerBuy2] Order params: BUY 1 MES 03-26 @ MARKET (limit=0.00, stop=0.00)
# [BrokerBuy2] Generated order ID: ZORRO_1000
# [BrokerBuy2] Time in force: GTC
# [BrokerBuy2] Calling Command(PLACE)...
# [BrokerBuy2] Command returned: 0
# [BrokerBuy2] Order placed successfully!
# Order 1000 (ZORRO_1000): BUY 1 MES 03-26 @ MARKET
# [BrokerBuy2] Waiting for market order fill...
# Order 1000 filled: 1 @ 6926.50
# [BrokerBuy2] Returning order ID: 1000
```

**After (Level 0 - default)**:
```
(clean - only errors appear)
```

**After (Level 1 - production)**:
```
# NT8 plugin v1.0.0 (TCP Bridge for NT8 8.1+)
# Order 1000 (ZORRO_1000): BUY 1 MES 03-26 @ MARKET
# Order 1000 filled: 1 @ 6926.50
```

**After (Level 2 - debugging)**:
```
# NT8 plugin v1.0.0 (TCP Bridge for NT8 8.1+)
# [BrokerBuy2] Called with Asset=MES 03-26, Amount=1, StopDist=0.00, Limit=0.00
# [BrokerBuy2] Order params: BUY 1 MES 03-26 @ MARKET (limit=0.00, stop=0.00)
# [BrokerBuy2] Generated order ID: ZORRO_1000
# [BrokerBuy2] Time in force: GTC
# [BrokerBuy2] Calling Command(PLACE)...
# [BrokerBuy2] Command returned: 0
# Order 1000 (ZORRO_1000): BUY 1 MES 03-26 @ MARKET
# [BrokerBuy2] Waiting for market order fill...
# Order 1000 filled: 1 @ 6926.50
# [BrokerBuy2] Returning order ID: 1000
```

---

## Architecture Changes

### Internal Improvements

1. **Added Functions**:
   - `LogInfo()` - conditional info logging
   - `LogDebug()` - conditional debug logging
   - `responsiveSleep()` - responsive waiting with cancellation

2. **New Global Variables**:
   - `g_diagLevel` - diagnostic level (0-2)

3. **New BrokerCommand Cases**:
   - `SET_DIAGNOSTICS` - set logging level
   - `GET_DIAGNOSTICS` - query logging level
   - `GET_MAXREQUESTS` - query rate limit

### No Breaking Changes

- All existing functions unchanged in behavior
- All existing APIs maintained
- Default behavior identical to previous version
- New features are opt-in only

---

## Testing Recommendations

### For Script Developers

1. **Test with Level 0** (default)
   - Verify logs are clean in production
   - Confirm errors still appear

2. **Test with Level 1** (monitoring)
   - Verify key events logged
   - Check for reasonable verbosity

3. **Test with Level 2** (debugging)
   - Verify detailed flow visible
   - Useful for troubleshooting

4. **Test Cancellation**
   - Start a script with market orders
   - During order wait, press ESC in Zorro
   - Verify graceful cancellation

### Expected Behavior

**Default (Level 0)**:
- Clean logs
- Only errors visible
- Same as before (backward compatible)

**With SET_DIAGNOSTICS**:
- Configurable verbosity
- No performance impact
- Easy debugging when needed

**Responsive Sleep**:
- UI stays responsive during waits
- User can cancel operations
- Better experience in slow markets

---

## Support

### If You Encounter Issues

1. **Enable Debug Logging**:
   ```c
   brokerCommand(SET_DIAGNOSTICS, 2);
   ```

2. **Check Version**:
   - Look for version in Zorro log: `NT8 plugin v1.0.0`

3. **Report Issues**:
   - Include diagnostic level 2 logs
   - Specify version number
   - Describe expected vs actual behavior

### Rolling Back

If needed, previous version is on `main` branch:

```bash
git checkout main
.\rebuild-debug.bat
```

---

## Conclusion

Version 1.0.0 brings **professional quality improvements**:
- ? Version tracking (like official plugins)
- ? Configurable logging (reduces spam)
- ? Responsive operations (better UX)
- ? Complete BrokerCommand support
- ? **Zero breaking changes**

All improvements follow **Zorro team's patterns** from the official Bittrex plugin.

**Recommendation**: Upgrade and use default settings (level 0) for production, enable level 1 or 2 when debugging.

Enjoy cleaner logs and better user experience! ??
