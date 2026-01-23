# ZorroBridge Logging - Quick Reference

## Log Levels

The ZorroBridge AddOn uses 5 log levels to control output verbosity:

| Level | When to Use | What You See |
|-------|-------------|--------------|
| **TRACE** | Deep debugging | Every TCP command, every response, every detail |
| **DEBUG** | Normal debugging | Important operations (orders, subscriptions, positions) |
| **INFO** | Production | Heartbeat (every 10s), login/logout, significant events |
| **WARN** | Production | Warnings and potential issues |
| **ERROR** | Production | Actual errors only |

## Default Level

**INFO** - Clean output suitable for live trading

## Changing Log Level

### From Zorro Script
```c
// Not yet implemented in plugin
// Future: brokerCommand(SET_LOGLEVEL, LOG_DEBUG);
```

### Via TCP Command (for testing)
```
SETLOGLEVEL:DEBUG
SETLOGLEVEL:INFO
SETLOGLEVEL:TRACE
```

### In AddOn Code
```csharp
private LogLevel currentLogLevel = LogLevel.INFO;  // Change here
```

## What Each Level Shows

### TRACE (Level 0)
```
[TRACE] << COMMAND: PLACEORDER:BUY:MES 03-26:1:MARKET:0:0
[TRACE] SUBSCRIBE RESPONSE: OK:Subscribed:MES 03-26:0.25:1.25
[TRACE] GetPrice: MES 03-26
[TRACE] Volume: 123456
[TRACE] >> PRICE:6850.00:6849.75:6850.25:123456
```
**When:** Diagnosing communication issues, protocol problems

### DEBUG (Level 1)
```
[DEBUG] Client connected
[DEBUG] Subscribe request: MES 03-26
[DEBUG] Contract specs: TickSize=0.25 PointValue=1.25
[DEBUG] Order: BUY 1 MES 03-26 @ MARKET
[DEBUG] ==== PlaceOrder START ====
[DEBUG] PLACEORDER command: BUY 1 MES 03-26
[DEBUG] ==== PlaceOrder SUCCESS ====
[DEBUG] Price MES 03-26: L:6850.00 B:6849.75 A:6850.25
```
**When:** Testing strategies, debugging order issues

### INFO (Level 2) ? **DEFAULT**
```
[INFO] Zorro Bridge starting...
[INFO] Listening on port 8888
[INFO] Logged in to account: Sim101
[INFO] Subscribed to MES 03-26
[INFO] Status OK | Prices:234 Orders:5 Instruments:1
[INFO] ORDER PLACED: BUY 1 MES 03-26 @ MARKET (ID:abc123...)
[INFO] POSITION QUERY: MES 03-26 = 1 contracts @ 6850.00
```
**When:** Production trading, clean monitoring

### WARN (Level 3)
```
[WARN] MarketData is null for MES 03-26 - connect to data feed
[WARN] Unknown command: BADCMD
[WARN] Error calculating unrealized P&L for MES 03-26: ...
```
**When:** Potential issues that don't stop execution

### ERROR (Level 4)
```
[ERROR] Failed to start TCP server: Address already in use
[ERROR] Not logged in
[ERROR] Account 'BadAccount' not found
[ERROR] Instrument 'BADSYMBOL' not found
[ERROR] PlaceOrder failed: Insufficient margin
```
**When:** Actual failures that prevent operation

## Heartbeat

At INFO level and above, status is logged every 10 seconds:

```
[INFO] Status OK | Prices:234 Orders:5 Instruments:1
```

Shows:
- **Prices**: Price requests since last heartbeat
- **Orders**: Total orders placed (cumulative)
- **Instruments**: Currently subscribed instruments

## Common Scenarios

### Scenario 1: "My orders aren't working"
**Use:** DEBUG level
```csharp
private LogLevel currentLogLevel = LogLevel.DEBUG;
```

**Look for:**
```
[DEBUG] PLACEORDER command: BUY 1 MES 03-26
[DEBUG] ==== PlaceOrder START ====
[DEBUG] Order: BUY 1 MES 03-26 @ MARKET
[INFO] ORDER PLACED: ... (ID:abc123)
[DEBUG] ==== PlaceOrder SUCCESS ====
```

**If missing:** Order never reached AddOn (check plugin)

### Scenario 2: "Connection drops randomly"
**Use:** TRACE level
```csharp
private LogLevel currentLogLevel = LogLevel.TRACE;
```

**Look for:**
```
[TRACE] << COMMAND: PING
[TRACE] >> PONG
[TRACE] << COMMAND: GETPRICE:MES 03-26
[TRACE] >> PRICE:6850.00:...
```

**If stops:** TCP connection lost

### Scenario 3: "Production trading"
**Use:** INFO level (default)
```csharp
private LogLevel currentLogLevel = LogLevel.INFO;
```

**You'll see:**
- Startup/shutdown messages
- Account login
- Subscriptions
- Order placements
- Position queries (important ones)
- Heartbeat every 10s

**You WON'T see:**
- Every price request
- Every position query
- TCP communication details
- Internal state details

### Scenario 4: "Prices seem wrong"
**Use:** DEBUG level

**Look for:**
```
[DEBUG] Price MES 03-26: L:6850.00 B:6849.75 A:6850.25
```

**Check:**
- Last (L), Bid (B), Ask (A) are reasonable
- Spread (A - B) makes sense
- Volume is non-zero

## Performance Impact

| Level | Overhead | Notes |
|-------|----------|-------|
| ERROR | Minimal | Almost no logging in normal operation |
| WARN | Minimal | Rare events only |
| INFO | Low | 10-second heartbeat + events |
| DEBUG | Medium | Every operation logged |
| TRACE | High | Every TCP message logged |

**For live trading:** Use INFO (default)  
**For debugging:** Use DEBUG  
**For protocol debugging:** Use TRACE (temporarily)

## Recent Changes

### v1.1.0 (2026-01-23)
- **Fixed:** PLACEORDER no longer logs at ERROR level
  - **Old:** `[ERROR] !! PLACEORDER RECEIVED: ...` (misleading!)
  - **New:** `[DEBUG] PLACEORDER command: BUY 1 MES 03-26` (correct)
- **Reason:** PLACEORDER is normal operation, not an error
- **Impact:** Cleaner INFO-level logs

### Previous Issues
The old code logged PLACEORDER at ERROR level for debugging:
```csharp
// OLD (v1.0.x)
case "PLACEORDER":
    Log(LogLevel.ERROR, $"!! PLACEORDER RECEIVED: {command}");  // Wrong!
    return HandlePlaceOrder(parts);
```

This made it appear like errors were occurring when orders were placed successfully. Now fixed:
```csharp
// NEW (v1.1.0+)
case "PLACEORDER":
    Log(LogLevel.DEBUG, $"PLACEORDER command: {parts[1]} {parts[3]} {parts[2]}");
    return HandlePlaceOrder(parts);
```

## See Also

- HandlePlaceOrder has detailed DEBUG logging at each step
- HandleGetPosition logs at DEBUG/TRACE for position queries
- All price requests log at TRACE only (unless error)
- Heartbeat ensures INFO level shows system is alive

---

**Recommendation:** Keep at INFO level for production, switch to DEBUG only when troubleshooting specific issues.
