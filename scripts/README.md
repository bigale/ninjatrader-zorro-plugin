# Test Scripts

Zorro test scripts for the NinjaTrader 8 plugin.

## Available Scripts

### NT8Test.c
**Purpose:** Comprehensive plugin test suite

**Tests:**
- ✅ Connection to NinjaTrader
- ✅ Account information
- ✅ Market data subscription
- ✅ Position tracking
- ✅ System information

**Usage:**
1. Copy to `C:\Zorro\Strategy\`
2. Select **NT8-Sim** account in Zorro
3. Select **NT8Test** script
4. Click **Trade**

**Expected Output:**
```
[PASS] Plugin loaded
[PASS] Account Info
[PASS] Market Data
[PASS] Position Query
[PASS] System Info
Plugin Status: OPERATIONAL
```

---

### SimpleNT8Test.c
**Purpose:** Quick connection and price test

**Tests:**
- ✅ Basic connectivity
- ✅ Live price retrieval
- ✅ Bar generation

**Usage:**
1. Copy to `C:\Zorro\Strategy\`
2. Select **NT8-Sim** account in Zorro
3. Select **SimpleNT8Test** script
4. Click **Trade**

**Expected Output:**
```
=== NT8 Live Test ===
No historical data - live only
Asset: MESH26
Price: 6047.50
Spread: 0.25
=== Success! ===
```

---

## Installation

Copy scripts to your Zorro Strategy folder:

```bash
copy scripts\*.c C:\Zorro\Strategy\
```

Or manually copy individual files.

---

## Configuration

Scripts use these settings:
- **Symbol:** MESH26 (MES March 2026)
- **BarPeriod:** 1 minute
- **LookBack:** 0 (live-only, no historical data)
- **Account:** NT8-Sim (configure in Zorro)

To test different symbols, edit the `asset()` line:
```c
asset("MESH26");  // Change to your symbol
```

---

## Troubleshooting

### "Market closed"
- Market is closed (expected on holidays/weekends)
- Use during market hours or with replay data

### "No bars generated"
- Verify `LookBack = 0` is set
- Check NinjaTrader has market data
- Ensure data feed is connected

### "Failed to connect"
- Verify NinjaTrader is running
- Check AddOn shows "listening on port 8888"
- Restart NinjaTrader if needed

---

## See Also

- [Getting Started Guide](../docs/GETTING_STARTED.md)
- [Troubleshooting Guide](../docs/TROUBLESHOOTING.md)
- [API Reference](../docs/API_REFERENCE.md)
