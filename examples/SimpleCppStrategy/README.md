# SimpleCppStrategy - Example C++ Zorro Strategy

This is a complete example of a C++ trading strategy for Zorro, demonstrating the proper development workflow.

## What This Is

- **NOT a test script** - This is a real trading strategy
- **Compiled as DLL** - Pre-compiled, loaded by Zorro at runtime
- **Links against zorro.lib** - Uses Zorro's API functions
- **Full C++17** - Can use STL, classes, templates, etc.

## Strategy Logic

Simple SMA (Simple Moving Average) crossover:
- **Buy Signal:** Fast SMA (10) crosses above Slow SMA (20)
- **Sell Signal:** Fast SMA crosses below Slow SMA

## Prerequisites

Before building, you need:

1. **Zorro installation** - Default: `C:\Zorro_2.66`
2. **zorro.lib** - Import library for Zorro API
   - Located in `C:\Zorro\lib\zorro.lib`
   - If not present, extract from Zorro.exe or use [szabonorbert's template](https://github.com/szabonorbert/zorro-project-template)
3. **trading.h** - Zorro API header
   - Located in `C:\Zorro\include\trading.h`
4. **Visual Studio** with C++ tools
5. **CMake 3.15+**

## Quick Start

### Option 1: Build from This Directory

```bash
# From examples/SimpleCppStrategy/
mkdir build
cd build

# Configure (32-bit for Zorro)
cmake -G "Visual Studio 17 2022" -A Win32 ..

# Build
cmake --build . --config Release

# DLL will be in C:\Zorro_2.66\Strategy\SimpleCppStrategy.dll
```

### Option 2: Build from Repository Root

```bash
# From repository root
cd build
cmake --build . --config Release --target SimpleCppStrategy
```

## Running in Zorro

1. **Start Zorro**
2. **Select Account** (e.g., Demo account for testing)
3. **Select Script**: Choose **SimpleCppStrategy.dll** from dropdown
4. **Click Test** - Run backtest on historical data
5. **Click Trade** - Run live (paper trading recommended first!)

## Debugging

### Method 1: Visual Studio Debugger

```bash
# 1. Build with debug symbols
cmake --build . --config Debug

# 2. Open SimpleCppStrategy.cpp in Visual Studio
# 3. Set breakpoint in run() function
# 4. Debug ? Attach to Process ? Zorro.exe
# 5. Load SimpleCppStrategy.dll in Zorro
# 6. Breakpoint will hit on each bar!
```

### Method 2: Printf Debugging

Already included in the strategy - check Zorro's console output.

### Method 3: Log File

Modify the strategy to write to a log file:

```cpp
FILE* log = fopen("C:\\Zorro\\Log\\SimpleCppStrategy_debug.log", "a");
if(log) {
    fprintf(log, "Bar %d: SMA values\n", (int)Bar);
    fclose(log);
}
```

## Customization

Edit `SimpleCppStrategy.cpp` to:

1. **Change parameters:**
   ```cpp
   static int g_fastPeriod = 5;   // Faster crossover
   static int g_slowPeriod = 15;
   ```

2. **Use different asset:**
   ```cpp
   asset("GBP/USD");  // Or any Zorro-supported asset
   ```

3. **Add indicators:**
   ```cpp
   var rsi = RSI(closePrices, 14);
   if(rsi < 30) {
       enterLong();  // Oversold
   }
   ```

4. **Add stops/limits:**
   ```cpp
   Stop = 20 * PIP;   // 20 pip stop loss
   TakeProfit = 40 * PIP;  // 40 pip take profit
   ```

## Project Structure

```
SimpleCppStrategy/
??? CMakeLists.txt              # Build configuration
??? SimpleCppStrategy.cpp       # Strategy code
??? README.md                   # This file
??? build/                      # Build output (gitignored)
```

## CMake Configuration

The `CMakeLists.txt` is configured to:
- Build 32-bit DLL (default for Zorro)
- Link against `zorro.lib`
- Output directly to `C:\Zorro\Strategy\`
- Include Zorro headers from `C:\Zorro\include\`

Adjust paths if your Zorro is installed elsewhere:

```bash
cmake -DZORRO_PATH=C:/MyZorroPath ..
```

## Comparison: Lite-C vs C++ DLL

| Feature | Lite-C (.c) | This C++ DLL |
|---------|-------------|--------------|
| **Compilation** | Zorro compiles | Pre-compiled |
| **Debugging** | Printf | VS Debugger |
| **Syntax** | Lite-C | Full C++17 |
| **STL** | ? | ? |
| **Build Required** | ? | ? |
| **Performance** | Good | Slightly better |

## Next Steps

1. **Build and test** this example strategy
2. **Modify parameters** and observe changes
3. **Add features** (stops, indicators, filters)
4. **Create your own** strategy using this as template

## References

- [Zorro Manual - C++ Scripting](https://zorro-project.com/manual/en/cpp.htm)
- [Zorro Manual - DLLs](https://zorro-project.com/manual/en/dlls.htm)
- [szabonorbert/zorro-project-template](https://github.com/szabonorbert/zorro-project-template)
- Full workflow documentation: `../../docs/ZORRO_CPP_WORKFLOW.md`

---

**Note:** This is for educational purposes. Always test strategies thoroughly on demo accounts before live trading.
