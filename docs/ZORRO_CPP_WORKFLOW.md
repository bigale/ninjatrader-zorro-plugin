# Zorro C++ Development Workflow

## Overview

This document explains how to develop Zorro trading strategies in C++ based on the [zorro-project-template](https://github.com/szabonorbert/zorro-project-template).

**Key Insight:** Each C++ trading strategy is a **separate DLL project** that links against `zorro.lib` and gets loaded by Zorro at runtime.

---

## Architecture

```
???????????????????????????????????????
?         Zorro.exe                   ?
?  (Trading Platform)                 ?
???????????????????????????????????????
            ? Loads DLL
            ?
???????????????????????????????????????
?    YourStrategy.dll                 ?
?  (C++ Compiled Strategy)            ?
?                                     ?
?  Exports: run()                     ?
?  Links: zorro.lib                   ?
?  Uses: Zorro API functions          ?
???????????????????????????????????????
            ? Calls
            ?
???????????????????????????????????????
?         zorro.lib                   ?
?  (Import library for Zorro API)     ?
?                                     ?
?  Provides:                          ?
?  - enterLong(), enterShort()        ?
?  - brokerCommand()                  ?
?  - Global vars (NumOpenLong, etc.)  ?
???????????????????????????????????????
```

---

## From szabonorbert/zorro-project-template

### Project Structure

```
zorro-project-template/
??? CMakeLists.txt              # Root CMake configuration
??? include/
?   ??? trading.h              # Zorro API definitions
??? lib/
?   ??? zorro.lib              # 32-bit import library
?   ??? zorro_x64.lib          # 64-bit import library
??? src/
    ??? Strategy.cpp           # Your strategy code
```

### Key Files Analyzed

#### 1. CMakeLists.txt Pattern

```cmake
cmake_minimum_required(VERSION 3.10)
project(ZorroStrategy)

# Set output directory to Zorro's Strategy folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:/Zorro/Strategy")

# Create DLL from strategy source
add_library(Strategy SHARED src/Strategy.cpp)

# Link against zorro.lib (provides Zorro API)
target_link_libraries(Strategy 
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/zorro.lib  # For 32-bit
)

# Include Zorro headers
target_include_directories(Strategy 
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)

# Ensure DLL exports run() function
set_target_properties(Strategy PROPERTIES
    PREFIX ""           # No "lib" prefix
    SUFFIX ".dll"       # .dll extension
)
```

#### 2. Strategy.cpp Pattern

```cpp
#include <trading.h>  // Zorro API

// Required export - Zorro calls this function
DLLFUNC void run()
{
    // Initialize on first call
    if(is(INITRUN)) {
        BarPeriod = 1;
        asset("EUR/USD");
    }
    
    // Strategy logic using Zorro API
    if(crossOver(SMA(20), SMA(50))) {
        enterLong();
    }
    
    if(crossUnder(SMA(20), SMA(50))) {
        enterShort();
    }
}
```

#### 3. trading.h (from Zorro)

Must be copied from `C:\Zorro\include\trading.h` - contains:
- Function declarations (enterLong, brokerCommand, etc.)
- Global variable externs (NumOpenLong, Asset, etc.)
- Type definitions (var, DATE, T6, etc.)
- Constants (INITRUN, EXITRUN, etc.)

---

## Critical Requirements

### 1. Must Link Against zorro.lib

**Why:** Zorro API functions (enterLong, brokerCommand, etc.) are **implemented in Zorro.exe**, not your DLL.

The `.lib` file is an **import library** that tells the linker:
- "These functions exist in Zorro.exe"
- "Link them at runtime when Zorro loads the DLL"

```cmake
# REQUIRED: Link against zorro.lib
target_link_libraries(YourStrategy PRIVATE zorro.lib)
```

### 2. Must Export run() Function

```cpp
DLLFUNC void run()  // DLLFUNC = __declspec(dllexport)
{
    // Your strategy code
}
```

Zorro calls `run()` every bar/tick.

### 3. Must Use Zorro's trading.h

```cpp
#include <trading.h>  // From C:\Zorro\include\trading.h
```

This header provides:
- Function declarations
- Global variable externs
- Type definitions

### 4. 32-bit vs 64-bit

- **Zorro 32-bit:** Link `zorro.lib` (default)
- **Zorro 64-bit:** Link `zorro_x64.lib`

Most installations use 32-bit Zorro.

---

## Build Workflow

### Step 1: Get zorro.lib

**Option A:** Extract from Zorro installation
```powershell
# zorro.lib is in C:\Zorro\lib\zorro.lib
# OR generate from Zorro.exe using dumpbin/lib tools
```

**Option B:** Use szabonorbert's template
```bash
git clone https://github.com/szabonorbert/zorro-project-template
# Copy lib/zorro.lib to your project
```

### Step 2: Create Strategy Project

```
MyStrategy/
??? CMakeLists.txt
??? include/
?   ??? trading.h        # From C:\Zorro\include\
??? lib/
?   ??? zorro.lib        # Import library
??? src/
    ??? MyStrategy.cpp   # Your code
```

### Step 3: Configure CMake

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyStrategy)

# Output to Zorro's Strategy folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "C:/Zorro/Strategy")

# Create DLL
add_library(MyStrategy SHARED src/MyStrategy.cpp)

# Link Zorro API
target_link_libraries(MyStrategy 
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/zorro.lib
)

target_include_directories(MyStrategy 
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(MyStrategy PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
)
```

### Step 4: Build

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A Win32 ..
cmake --build . --config Release
```

### Step 5: Run in Zorro

1. Start Zorro
2. Select **MyStrategy.dll** from dropdown
3. Click Trade
4. Strategy runs!

---

## Debugging C++ Strategies

### Method 1: Visual Studio Debugger

```bash
# 1. Build with debug symbols
cmake --build . --config Debug

# 2. Set breakpoint in MyStrategy.cpp
# 3. Visual Studio: Debug ? Attach to Process ? Zorro.exe
# 4. Run strategy in Zorro
# 5. Breakpoint hits!
```

### Method 2: OutputDebugString

```cpp
#include <windows.h>

DLLFUNC void run()
{
    char msg[256];
    sprintf(msg, "NumOpenLong: %d\n", NumOpenLong);
    OutputDebugStringA(msg);  // View in DebugView or VS Output
}
```

### Method 3: File Logging

```cpp
DLLFUNC void run()
{
    FILE* log = fopen("C:\\Zorro\\Log\\MyStrategy_debug.log", "a");
    if(log) {
        fprintf(log, "Bar %d: Price %.2f\n", (int)Bar, priceClose());
        fclose(log);
    }
}
```

---

## Available Zorro API Functions

When linked against `zorro.lib`, you can use:

### Trading Functions
```cpp
int enterLong(int lots = 0);
int enterShort(int lots = 0);
void exitLong(const char* name = 0);
void exitShort(const char* name = 0);
```

### Market Data
```cpp
var priceClose(int offset = 0);
var priceOpen(int offset = 0);
var priceHigh(int offset = 0);
var priceLow(int offset = 0);
```

### Indicators
```cpp
var SMA(vars data, int period);
var EMA(vars data, int period);
```

### Broker Commands
```cpp
double brokerCommand(int command, DWORD parameter);
```

### Global Variables
```cpp
extern int NumOpenLong;
extern int NumOpenShort;
extern int NumPendingLong;
extern int NumPendingShort;
extern var* Asset;
extern int Bar;
```

Full API: [Zorro Manual - Function Reference](https://zorro-project.com/manual/en/manual.htm)

---

## Comparison: Lite-C vs C++ DLL

| Feature | Lite-C Script | C++ DLL |
|---------|---------------|---------|
| **Compilation** | Zorro compiles at load | Pre-compiled DLL |
| **Debugging** | printf only | Visual Studio debugger |
| **Syntax** | Lite-C (C subset) | Full C++17 |
| **STL** | ? No | ? Yes |
| **Build Time** | None (instant load) | Manual build required |
| **Performance** | Slightly slower | Slightly faster |
| **Complexity** | Simple | Requires build system |

---

## Common Issues

### 1. "Unresolved external symbol" errors

**Cause:** Not linking against `zorro.lib`

**Fix:**
```cmake
target_link_libraries(MyStrategy PRIVATE zorro.lib)
```

### 2. "Cannot load DLL" in Zorro

**Cause:** 64-bit DLL for 32-bit Zorro (or vice versa)

**Fix:**
```bash
cmake -A Win32 ..  # For 32-bit Zorro
```

### 3. "run() not found"

**Cause:** Missing `DLLFUNC` export

**Fix:**
```cpp
DLLFUNC void run()  // Must have DLLFUNC
```

### 4. Global variables undefined

**Cause:** Not including `trading.h` or not linking `zorro.lib`

**Fix:**
```cpp
#include <trading.h>  // Declares externs
// AND link zorro.lib in CMakeLists.txt
```

---

## Next Steps

1. **Try szabonorbert's template:**
   ```bash
   git clone https://github.com/szabonorbert/zorro-project-template
   cd zorro-project-template
   # Follow their README
   ```

2. **Create your own strategy:**
   - Use template as starting point
   - Modify `src/Strategy.cpp`
   - Build and test

3. **For this project:**
   - We could create an `examples/` folder
   - Add a simple C++ strategy example
   - Use it to test broker plugin features

---

## References

- [szabonorbert/zorro-project-template](https://github.com/szabonorbert/zorro-project-template)
- [Zorro Manual - DLLs](https://zorro-project.com/manual/en/dlls.htm)
- [Zorro Manual - C++ Scripting](https://zorro-project.com/manual/en/cpp.htm)

---

**Key Takeaway:** C++ strategies ARE possible with Zorro, but each strategy is a **separate project** that links against `zorro.lib`. This is very different from writing test scripts - it's for actual trading strategies, not plugin testing.
