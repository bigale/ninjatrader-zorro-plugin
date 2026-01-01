# Troubleshooting Guide

Solutions to common issues with the NinjaTrader 8 Zorro Plugin.

## Quick Diagnostics

Run this test first to identify the issue:

```c
// Diagnostic.c
function run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 1);
        printf("\n=== NT8 Plugin Diagnostics ===\n");
        
        // Test 1: Connection
        if(Live)
            printf("\n[OK] Connected to broker");
        else {
            printf("\n[FAIL] Not connected");
            quit("Connection test failed");
        }
        
        // Test 2: Account
        printf("\n[TEST] Account balance: $%.2f", Balance);
        
        // Test 3: Asset
        asset("MESH26");
        printf("\n[TEST] Asset: %s", Asset);
        
        // Test 4: Price
        printf("\n[TEST] Price: %.2f", priceClose());
        
        quit("Diagnostics complete");
    }
}
```

---

## Connection Issues

### "Failed to connect to NinjaTrader AddOn on localhost:8888"

**Symptoms:**
- Zorro shows "Login 0 NT8.. failed"
- Error about port 8888
- Connection timeout

**Solutions:**

#### 1. Verify AddOn is Running

**Check NinjaTrader Output window:**
```
Should see: [Zorro ATI] Zorro ATI Bridge listening on port 8888
```

**If missing:**
1. Press **F5** in NinjaTrader to recompile
2. Check for compilation errors in Output
3. Restart NinjaTrader

#### 2. Check AddOn File Location

File must be at:
```
C:\Users\<YourName>\Documents\NinjaTrader 8\bin\Custom\AddOns\ZorroBridge.cs
```

**Verify:**
```powershell
Get-ChildItem "C:\Users\$env:USERNAME\Documents\NinjaTrader 8\bin\Custom\AddOns" -Filter "ZorroBridge.cs"
```

#### 3. Verify Port is Free

**Check if port 8888 is in use:**
```powershell
netstat -ano | findstr :8888
```

**If port is taken:**
- Close the conflicting application
- Or modify port in both plugin and AddOn

#### 4. Firewall/Antivirus

**Windows Firewall:**
- Usually allows localhost connections
- If blocked, add exception for Zorro.exe

**Antivirus:**
- May block TCP connections
- Add exception for both Zorro and NinjaTrader

---

### "Login failed: ERROR:Account not found"

**Cause:** Account name mismatch

**Solution:**

1. **Check account name in NinjaTrader:**
   - Control Center ? Accounts tab
   - Note exact name (e.g., "Sim101")

2. **Update accounts.csv:**
   ```csv
   NT8-Sim,Sim101,,Demo
            ^^^^^^
            Must match exactly!
   ```

3. **Common account names:**
   - Simulation: `Sim101`
   - Playback: `Playback101`
   - Live: Your broker account name

---

## Market Data Issues

### "No bars generated" / "Error 047"

**Symptoms:**
- Script starts but immediately stops
- "Error 047: No bars generated"
- Market closed message

**Solutions:**

#### 1. Set LookBack = 0

```c
// WRONG:
LookBack = 100;  // Tries to get historical data

// CORRECT:
LookBack = 0;  // Live-only
```

**Why:** Plugin doesn't provide historical data

#### 2. Check Market Hours

**Futures markets:**
- Sunday 6 PM ET - Friday 5 PM ET
- Daily maintenance: 5:00-6:00 PM ET

**During closed hours:**
- Use Playback mode in NinjaTrader
- Or use Simulation with market replay

#### 3. Connect to Data Feed

**In NinjaTrader:**
1. Connections ? Configure
2. Select your data provider
3. Connect
4. Verify "Connected" status

---

### All Prices Show Zero

**Symptoms:**
- `priceClose()` returns 0
- No bid/ask data
- Spread is 0

**Solutions:**

#### 1. Check Symbol Exists

**In NinjaTrader:**
- Open chart for symbol (e.g., MES 03-26)
- Verify data appears

**If symbol not found:**
- Update symbol name in script
- Check contract month/year
- Verify symbol format

#### 2. Wait for Subscription

```c
asset("MESH26");
Sleep(500);  // Brief delay for data
```

#### 3. Check MarketData in AddOn

**NinjaTrader Output should show:**
```
[Zorro ATI] Subscribed to MES 03-26
```

**If "WARNING: Last is null":**
- Market closed
- No data subscription
- Symbol doesn't exist

---

### "MarketData properties are null"

**Cause:** No live data available

**Check:**

1. **NinjaTrader connection status**
   - Bottom left: Should show "Connected"
   - Not "Disconnected" or "Connecting"

2. **Data subscription**
   - Open chart for the symbol
   - Verify prices updating

3. **Market hours**
   - Check if market is open
   - Or use replay/simulation data

---

## Order Issues

### Orders Not Executing

**Symptoms:**
- `enterLong()` returns 0
- No orders appear in NinjaTrader
- "Order placement failed" in logs

**Solutions:**

#### 1. Check Account Balance

```c
if(Balance < MarginCost) {
    printf("\nInsufficient margin!");
}
```

#### 2. Check Symbol

```c
// Verify symbol is correct
printf("\nOrdering: %s", Asset);
```

#### 3. Check NinjaTrader Orders Tab

**Should see:**
- Order submitted
- Fill status
- Any rejection messages

#### 4. Check Logs

**NinjaTrader Output:**
```
[Zorro ATI] Order placed: BUY 1 MES 03-26
```

**If missing:**
- AddOn not receiving command
- Check connection

---

### Orders Fill But Zorro Doesn't Know

**Symptoms:**
- Order shows filled in NinjaTrader
- But Zorro thinks still pending
- Position mismatch

**Solution:**

**Wait for fill confirmation:**
```c
// Market orders wait up to 1 second
// If still pending after, check:
int orderId = enterLong(1);
Sleep(1000);  // Give time for fill

// Then check:
int pos = brokerCommand(GET_POSITION, (long)Asset);
```

---

## Symbol Format Issues

### "Failed to subscribe to MESH26"

**Cause:** Symbol conversion failed

**Check conversion in NinjaTrader Output:**
```
[Zorro ATI] Converted symbol: MESH26 -> MES 03-26
```

**If not converting:**
1. Restart NinjaTrader (reload AddOn)
2. Verify AddOn has latest code
3. Check symbol format

**Valid formats:**
```c
asset("MESH26");      // Month code
asset("MES 03-26");   // NT8 direct
asset("MES 0326");    // MMYY format
```

---

## Plugin Load Issues

### "NT8.dll not found"

**Solution:**

1. **Verify file exists:**
   ```powershell
   Get-Item "C:\Zorro\Plugin\NT8.dll"
   ```

2. **Check if 32-bit:**
   ```powershell
   dumpbin /headers "C:\Zorro\Plugin\NT8.dll" | findstr machine
   # Should show: 14C machine (x86)
   ```

3. **Rebuild as 32-bit:**
   ```bash
   cmake -G "Visual Studio 17 2022" -A Win32 ..
   ```

---

### "Failed to load DLL" / Missing Dependencies

**Cause:** Missing Visual C++ Runtime

**Solution:**

Install Visual C++ Redistributable:
- [VC++ 2015-2022 x86](https://aka.ms/vs/17/release/vc_redist.x86.exe)

**Verify:**
```powershell
Get-ItemProperty HKLM:\Software\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\X86
```

---

## Performance Issues

### Slow Order Execution

**Check:**

1. **Network latency** (should be <1ms for localhost)
2. **NinjaTrader performance** (CPU usage)
3. **Polling rate** (default 50ms is optimal)

**Don't:**
- Reduce polling rate below 50ms
- Place orders too frequently (<1 per second)

---

### High CPU Usage

**Causes:**
- Too frequent polling
- Too many log messages
- Large position monitoring

**Solution:**
```c
// Disable diagnostics after debugging
brokerCommand(SET_DIAGNOSTICS, 0);

// Reduce unnecessary logging
// Don't printf every tick!
```

---

## Debugging Steps

### Step 1: Enable Diagnostics

```c
if(is(INITRUN)) {
    brokerCommand(SET_DIAGNOSTICS, 1);
}
```

### Step 2: Check Logs

**Zorro Log:**
```
C:\Zorro\Log\<ScriptName>_demo.log
```

**NinjaTrader Output:**
- Check for [Zorro ATI] messages
- Look for errors or warnings

### Step 3: Manual TCP Test

**PowerShell test:**
```powershell
$client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", 8888)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)

# Test PING
$writer.WriteLine("PING")
$writer.Flush()
$response = $reader.ReadLine()
Write-Host "Response: $response"  # Should be "PONG"

$client.Close()
```

---

## Getting Help

### Before Posting Issues

1. ? Run diagnostic script (top of this doc)
2. ? Check NinjaTrader Output window
3. ? Review Zorro logs
4. ? Verify all prerequisites installed
5. ? Test with provided example scripts

### Include in Issue Report

```
1. Zorro version:
2. NinjaTrader version:
3. Plugin version:
4. Account type (Sim/Live):
5. Symbol being traded:
6. Error message (exact text):
7. Relevant log excerpts:
8. Steps to reproduce:
```

### Common Quick Fixes

| Issue | Quick Fix |
|-------|-----------|
| Connection failed | Restart NinjaTrader, recompile AddOn (F5) |
| No bars | Set `LookBack = 0` |
| Symbol not found | Use correct format: MESH26 or MES 03-26 |
| Order failed | Check balance, symbol, market hours |
| No price data | Connect to data feed, check market hours |

---

## See Also

- [Installation Guide](INSTALLATION.md)
- [API Reference](API_REFERENCE.md)
- [Getting Started](GETTING_STARTED.md)
