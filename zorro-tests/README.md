# Zorro C++ Testing Framework

This directory contains C++ test scripts that compile as DLLs for Zorro to load, replacing the old Lite-C test scripts.

## Benefits

? **Real C++ syntax** - No Lite-C limitations
? **Full debugging** - Breakpoints and step-through in Visual Studio
? **Better assertions** - ASSERT_EQ, ASSERT_TRUE with detailed failure messages
? **STL support** - Use vectors, strings, etc.
? **Same testing** - Still uses real NT8 connection

## Quick Start

### 1. Build Test DLLs

```bash
# From repository root
cd build
cmake --build . --config Release --target PositionTest
cmake --build . --config Release --target LimitOrderTest
```

DLLs will be output to `C:\Zorro_2.66\Strategy\`

### 2. Run Tests Manually

1. Start NinjaTrader with ZorroBridge AddOn
2. Start Zorro
3. Select NT8-Sim account
4. Select **PositionTest.dll** (not .c!)
5. Click Trade
6. Watch test execute with full C++ debugging!

### 3. Run Tests Automatically

```bash
# From repository root
.\test-runner.bat
```

This will:
- Build all test DLLs
- Run each test in Zorro
- Parse log files
- Report pass/fail status

## Test Files

| File | Description |
|------|-------------|
| `PositionTest.cpp` | Position tracking tests (replaces PositionTest2.c) |
| `LimitOrderTest.cpp` | Limit order placement and cancellation tests |
| `common/TestHelpers.h` | Assertion macros and utility functions |
| `common/TestHelpers.cpp` | Test helper implementation |

## Creating New Tests

### 1. Create Test File

```cpp
// MyNewTest.cpp
#include <trading.h>
#include "TestHelpers.h"

using namespace ZorroTest;

DLLFUNC void run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 1);
        asset("MESH26");
        PrintTestHeader("My New Test");
    }
    
    // Your test code here
    PlaceMarketLong(1);
    ASSERT_EQ(NumOpenLong, 1, "Position opened");
    
    CloseAll();
    
    TestRunner::PrintSummary();
    quit(TestRunner::GetFailCount() == 0 ? "PASSED" : "FAILED");
}
```

### 2. Add to CMakeLists.txt

```cmake
# In zorro-tests/CMakeLists.txt
add_zorro_test(MyNewTest)
```

### 3. Build and Run

```bash
cd build
cmake --build . --config Release --target MyNewTest

# Then load MyNewTest.dll in Zorro
```

## Available Assertions

```cpp
ASSERT_EQ(actual, expected, "message");     // Equal
ASSERT_TRUE(condition, "message");          // Condition true
ASSERT_FALSE(condition, "message");         // Condition false
ASSERT_GT(actual, value, "message");        // Greater than
```

## Helper Functions

```cpp
// Order placement
int id = PlaceMarketLong(quantity);
int id = PlaceMarketShort(quantity);
int id = PlaceLimitLong(quantity, limitPrice);
int id = PlaceLimitShort(quantity, limitPrice);

// Position management
CloseAllLong();   // Close all long positions
CloseAllShort();  // Close all short positions
CloseAll();       // Close everything

// Utilities
PrintTestHeader("Test Name");  // Print formatted header
WaitSeconds(5);                // Wait N seconds
```

## Debugging

### Visual Studio Debugger

1. Set breakpoint in your test .cpp file
2. In Visual Studio: Debug ? Attach to Process
3. Select Zorro.exe
4. Run your test in Zorro
5. Breakpoint will hit - step through C++ code!

### Log Files

Test logs are written to:
```
C:\Zorro_2.66\Log\<TestName>_demo.log
```

View last 10 lines:
```powershell
Get-Content "C:\Zorro_2.66\Log\PositionTest_demo.log" -Tail 10
```

## Configuration

Edit `zorro-tests/CMakeLists.txt` to change Zorro path:

```cmake
set(ZORRO_PATH "C:/Zorro_2.66" CACHE PATH "Path to Zorro installation")
```

Or override when running cmake:
```bash
cmake -DZORRO_PATH=C:/MyZorroPath ..
```

## Troubleshooting

### DLL not found in Zorro

- Check DLL is in `C:\Zorro_2.66\Strategy\`
- Verify DLL was built (check build output)
- Make sure you select the **.dll** in Zorro, not .c

### Compile errors

- Verify `trading.h` is in `include/` directory
- Check C++17 is enabled
- Make sure TestHelpers.h is accessible

### Tests fail

- Check NinjaTrader is running with ZorroBridge
- Verify NT8 plugin is loaded: `C:\Zorro\Plugin\NT8.dll`
- Check NT Output window for ZorroBridge messages
- Review test log file in `C:\Zorro_2.66\Log\`

## Comparison: Lite-C vs C++ DLL

| Feature | Lite-C (.c) | C++ DLL (.dll) |
|---------|-------------|----------------|
| Syntax | Limited Lite-C | Full C++17 |
| Debugging | Printf only | Visual Studio debugger |
| Assertions | Manual checks | ASSERT_* macros |
| STL | No | Yes |
| Build | Zorro compiles | CMake builds DLL |
| Testing | Same | Same |

## Next Steps

1. Port remaining Lite-C tests to C++
2. Add more assertion types as needed
3. Create test suite for all broker functions
4. Integrate with CI/CD (future)

---

**Note:** These tests still require NinjaTrader running with the ZorroBridge AddOn. They are NOT mocked unit tests - they test the real broker connection.
