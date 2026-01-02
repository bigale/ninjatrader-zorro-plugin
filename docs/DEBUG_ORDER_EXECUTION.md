# Debugging Order Execution Issues

## What We Added

### 1. Plugin-Side Debug (TcpBridge.cpp)
Prints to Zorro console showing:
- Exact command being sent to NT
- All order parameters
- Response from NT AddOn

### 2. AddOn-Side Debug (ZorroBridge.cs)
Prints to NT Output window showing:
- Raw command received
- Parsed order parameters
- Symbol conversion (Zorro ? NT8)
- Instrument lookup result
- Order creation steps
- Order submission result

## How to Test

### Step 1: Rebuild and Install

```bash
# Build plugin with new debug code
cd build
cmake --build . --config Release

# Install
copy Release\NT8.dll C:\Zorro\Plugin\NT8.dll
```

### Step 2: Update AddOn in NinjaTrader

1. Copy updated `ninjatrader-addon/ZorroBridge.cs` to:
   ```
   Documents\NinjaTrader 8\bin\Custom\AddOns\
   ```

2. In NinjaTrader, press **F5** to recompile

3. **Verify** in Output window:
   ```
   [Zorro Bridge] Zorro Bridge listening on port 8888
   ```

### Step 3: Run TradeTest

1. **Start NinjaTrader**
   - Make sure Output window is visible (Ctrl+O)
   - Clear output (right-click ? Clear)

2. **Start Zorro**
   - Select NT8-Sim account
   - Load TradeTest script
   - Click **Trade**

3. **Try Market Order First**
   - Should be in "MKT Order" mode by default
   - Click "Buy Long"

4. **Watch BOTH windows**:

   **Zorro Console:**
   ```
   [TcpBridge] Sending order command: PLACEORDER:BUY:MES 03-26:1:MARKET:0.00
   [TcpBridge] Details:
     Action: BUY
     Instrument: MES 03-26
     Quantity: 1
     OrderType: MARKET
     LimitPrice: 0.00
   [TcpBridge] Response: ORDER:12345
   ```

   **NinjaTrader Output:**
   ```
   [Zorro Bridge] ==== HandlePlaceOrder START ====
   [Zorro Bridge] Received: PLACEORDER:BUY:MES 03-26:1:MARKET:0
   [Zorro Bridge] Account: Sim101
   [Zorro Bridge] Parsed order:
     Action: BUY
     Symbol (Zorro): MES 03-26
     Quantity: 1
     OrderType: MARKET
     LimitPrice: 0
     Symbol (NT8): MES 03-26
   [Zorro Bridge] Instrument found: MES 03-26
   [Zorro Bridge] Creating order...
   [Zorro Bridge] Order created: NT123456789
   [Zorro Bridge] Order state: Submitted
   [Zorro Bridge] Submitting to account...
   [Zorro Bridge] Order submitted
   [Zorro Bridge] Order state after submit: Working
   [Zorro Bridge] ==== HandlePlaceOrder SUCCESS ====
   ```

## Common Issues to Look For

### Issue 1: Symbol Not Found
**NT Output shows:**
```
[Zorro Bridge] ERROR: Instrument 'MES 03-26' not found
```

**Cause:** Symbol doesn't exist in NT or misspelled

**Fix:**
- Open NT chart for symbol to verify it exists
- Check spelling exactly
- Make sure contract month/year is correct

### Issue 2: Not Subscribed
**NT Output shows:**
```
[Zorro Bridge] ERROR: Not subscribed to MES 03-26
```

**Cause:** Asset subscription failed

**Fix:**
- Check that `asset("MES 03-26")` or `asset("MESH26")` was called
- Look for earlier SUBSCRIBE message in output

### Issue 3: Account Not Logged In
**NT Output shows:**
```
[Zorro Bridge] ERROR: Not logged in (currentAccount is null)
```

**Cause:** Login failed

**Fix:**
- Check account name matches exactly (case-sensitive)
- Verify account exists in NT Control Center

### Issue 4: Order Created But Not Submitted
**NT Output shows:**
```
[Zorro Bridge] Order created: NT123456789
[Zorro Bridge] Order state: Submitted
[Zorro Bridge] Submitting to account...
[Zorro Bridge] ==== HandlePlaceOrder EXCEPTION ====
Exception: <error message>
```

**This is the key** - tells us exactly what NT rejected

**Common exceptions:**
- Insufficient funds
- Invalid contract
- Market closed
- Connection issue

### Issue 5: No Debug Output at All
**Possible causes:**
1. Old DLL still loaded (restart Zorro)
2. Old AddOn still compiled (recompile with F5)
3. Wrong account selected
4. Connection failed before order

## What to Send Me

If it still doesn't work, capture:

1. **Full Zorro Console Output** (from start to after clicking Buy)
2. **Full NT Output Window** (clear first, then run test)
3. **NT Orders Tab** screenshot (does order appear there?)
4. **Assets.csv** - the MESH26 line
5. **Accounts.csv** - the NT8-Sim line

## Next Steps Based on Output

### If Order Reaches NT But Fails
? NT is rejecting the order (we'll see why in exception message)

### If Order Never Reaches NT
? Communication problem (check TCP connection logs)

### If Order Submits But Doesn't Fill
? Market data/pricing issue (need to check prices)

## Quick Sanity Checks

Before running test:

1. ? NT Output shows "listening on port 8888"
2. ? Zorro shows "Connected to account: Sim101"
3. ? Asset line: Name=MESH26, Symbol=MES 03-26
4. ? Account line: Plugin=NT8.dll, Assets=Assets
5. ? Can open NT chart for MES 03-26 and see prices

Ready to test! Run the test and send me the debug output from both windows.
