# Installation Guide

Complete installation instructions for the NinjaTrader 8 Zorro Plugin.

## Prerequisites

### Required Software

1. **NinjaTrader 8.1 or later**
   - Download: https://ninjatrader.com/GetStarted
   - Free version works perfectly
   - Must be version 8.1+ (not 8.0)

2. **Zorro 2.62 or later**
   - Download: https://zorro-project.com/download.php
   - Free version works for testing
   - Pro version for live trading

3. **Visual Studio 2019+** (for building from source)
   - Community Edition (free) is sufficient
   - Install "Desktop development with C++" workload
   - Windows 10 SDK required

4. **CMake 3.15+**
   - Download: https://cmake.org/download/
   - Add to PATH during installation

---

## Step-by-Step Installation

### Part 1: Build the Plugin

#### Option A: Using the Build Script (Easiest)

```bash
cd ninjatrader-zorro-plugin
build-and-install.bat
```

This will:
- Build NT8.dll as 32-bit
- Copy to `C:\Zorro\Plugin\`
- Display build status

#### Option B: Manual Build with CMake

```bash
# Create build directory
mkdir build
cd build

# Configure (32-bit is critical!)
cmake -G "Visual Studio 17 2022" -A Win32 ..

# Build
cmake --build . --config Release

# Install
copy Release\NT8.dll C:\Zorro\Plugin\
```

#### Option C: Visual Studio IDE

1. Open `CMakeLists.txt` in Visual Studio 2019+
2. Select configuration: **x86-Release**
3. Build ? Build All
4. Copy `build\Release\NT8.dll` to `C:\Zorro\Plugin\`

**?? Important**: The plugin MUST be compiled as 32-bit (Win32) for Zorro compatibility.

---

### Part 2: Install the NinjaScript AddOn

The AddOn provides the TCP bridge between Zorro and NinjaTrader.

#### 2.1 Copy the AddOn File

Copy file from:
```
ninjatrader-zorro-plugin\ninjatrader-addon\ZorroBridge.cs
```

To:
```
C:\Users\<YourUsername>\Documents\NinjaTrader 8\bin\Custom\AddOns\
```

Replace `<YourUsername>` with your Windows username.

#### 2.2 Compile in NinjaTrader

1. **Open NinjaTrader 8**

2. **Open NinjaScript Editor**
   - Press **F5**, or
   - Tools ? NinjaScript Editor

3. **Compile**
   - Click **Compile** button (or Ctrl+F5)
   - Watch the **Output** tab

4. **Verify Success**
   - Look for: `Compiled successfully`
   - Should see: `[Zorro ATI] Zorro ATI Bridge listening on port 8888`

**If compilation fails:**
- Check you're using NinjaTrader 8.1+ (not 8.0)
- Look for error messages in Output tab
- Verify file was copied to correct location

#### 2.3 Verify AddOn is Running

In NinjaTrader's Output window, you should see:
```
[Zorro ATI] Zorro ATI Bridge listening on port 8888
```

This confirms the AddOn is active and ready to accept connections.

---

### Part 3: Configure Zorro

#### 3.1 Create Account Configuration

Edit (or create) `C:\Zorro\History\accounts.csv`:

```csv
NT8-Sim,Sim101,,Demo
```

**Format**: `BrokerName,NTAccountName,Password(unused),Type`

**For live trading**, use:
```csv
NT8-Live,YourLiveAccount,,Real
```

**Common NinjaTrader account names:**
- `Sim101` - Default simulation account
- `Playback101` - Playback account  
- Your broker account name for live trading

#### 3.2 Create Asset Configuration

Edit `C:\Zorro\History\AssetsRithmic.csv` (or create new file):

```csv
Name,Price,Spread,RollLong,RollShort,PIP,PIPCost,MarginCost,Market,Multiplier,Commission,Symbol
MESH26,6000,0.25,0,0,0.25,1.25,500,CME,5,0.52,MES 03-26
ESH26,6000,0.25,0,0,0.25,12.50,12000,CME,50,2.04,ES 03-26
MNQH26,18000,0.25,0,0,0.25,0.50,1600,CME,2,0.52,MNQ 03-26
```

**Key columns:**
- **Name**: Symbol as used in Zorro (MESH26, ESH26, etc.)
- **Symbol**: Actual NinjaTrader format (MES 03-26, ES 03-26, etc.)
- **Multiplier**: Contract multiplier ($5 for MES, $50 for ES)
- **Commission**: Per-contract commission

---

## Verification

### Test the Installation

1. **Start NinjaTrader 8**
   - Connect to data feed (or use Sim)
   - Verify Output shows: `[Zorro ATI] Zorro ATI Bridge listening on port 8888`

2. **Start Zorro**

3. **Create test script** `TestInstall.c`:
   ```c
   function run()
   {
       BarPeriod = 1;
       LookBack = 0;
       
       if(is(INITRUN)) {
           printf("\n=== Testing NT8 Plugin ===");
           printf("\nBalance: $%.2f", Balance);
       }
       
       asset("MESH26");
       printf("\nPrice: %.2f", priceClose());
       
       quit("Installation test complete!");
   }
   ```

4. **Run the test**
   - Select **NT8-Sim** account in Zorro
   - Select **TestInstall** script
   - Click **Trade**

5. **Expected output**:
   ```
   === Testing NT8 Plugin ===
   Balance: $100000.00
   Price: 6047.50
   Installation test complete!
   ```

---

## Troubleshooting

### Plugin doesn't load in Zorro

**Check:**
- NT8.dll exists in `C:\Zorro\Plugin\`
- File is 32-bit (not 64-bit)
- Visual C++ Runtime installed
- Try running Zorro as Administrator

**Verify file:**
```powershell
Get-Item "C:\Zorro\Plugin\NT8.dll" | Select-Object Name, Length
```

### AddOn doesn't compile

**Common issues:**

1. **Wrong NinjaTrader version**
   - Requires NT8 8.1+
   - Check: Help ? About NinjaTrader

2. **File in wrong location**
   - Must be in: `Documents\NinjaTrader 8\bin\Custom\AddOns\`
   - Create AddOns folder if it doesn't exist

3. **Compilation errors**
   - Check Output tab for specific errors
   - Verify file wasn't corrupted during copy

### "Failed to connect to NinjaTrader AddOn"

**Solutions:**

1. **Verify AddOn is running**
   - Check NinjaTrader Output: should show "listening on port 8888"
   - If not, recompile AddOn (F5)

2. **Restart NinjaTrader**
   - Close completely
   - Relaunch
   - Verify Output message appears

3. **Check firewall**
   - Allow localhost:8888 connections
   - Usually not needed for localhost

4. **Port conflict**
   - Check if another program uses port 8888
   - Use: `netstat -ano | findstr :8888`

### No price data

**Check:**
1. NinjaTrader connected to data feed
2. Symbol exists and has data
3. Market is open (or use sim/replay)
4. Subscription successful (check logs)

---

## Next Steps

After successful installation:

1. **Read**: [Getting Started Guide](GETTING_STARTED.md)
2. **Learn**: [API Reference](API_REFERENCE.md)
3. **Test**: Run included test scripts
4. **Trade**: Start with simulation account

---

## Quick Reference

### File Locations

| File | Location |
|------|----------|
| Plugin DLL | `C:\Zorro\Plugin\NT8.dll` |
| AddOn Source | `Documents\NinjaTrader 8\bin\Custom\AddOns\ZorroBridge.cs` |
| Accounts | `C:\Zorro\History\accounts.csv` |
| Assets | `C:\Zorro\History\AssetsRithmic.csv` |
| Test Scripts | `C:\Zorro\Strategy\` |

### Key Commands

```bash
# Build plugin
build-and-install.bat

# Compile AddOn in NinjaTrader
Press F5

# Test connection
# In Zorro: Select NT8-Sim, run TestInstall.c
```

---

**Need help?** Check the [Troubleshooting Guide](TROUBLESHOOTING.md) or open an issue.
