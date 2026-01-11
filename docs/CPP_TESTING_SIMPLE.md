# Zorro C++ Test Scripts - Simple Approach

## Overview

Instead of complex mocking frameworks, use **Zorro's built-in C++ DLL support** to write test scripts in C++ instead of Lite-C. This gives you:

? **Real C++ syntax** - No Lite-C limitations
? **Full debugging** - Breakpoints, step-through in Visual Studio
? **Same testing** - Still uses real NT8 connection
? **No mocking needed** - Tests actual broker behavior
? **Minimal setup** - Just compile a DLL instead of a .c script

---

## How Zorro C++ DLLs Work

Zorro can load C++ DLLs as "scripts" using this pattern:

```cpp
// TestScript.cpp - Compiled as DLL
#include <trading.h>

DLLFUNC void run()
{
    // Your test code here in C++
    // Full access to: loops, STL, classes, templates, etc.
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 2);
        asset("MESH26");
    }
    
    // Test logic...
    enterLong(1);
    
    if(NumOpenLong == 1) {
        printf("? Position opened\n");
    } else {
        printf("? Position failed\n");
    }
    
    exitLong();
    quit("Test complete");
}
```

Then in Zorro: Just select the DLL name instead of a .c script!

---

## Directory Structure

```
ninjatrader-zorro-plugin/
??? zorro-tests/                    # NEW - C++ test scripts
?   ??? CMakeLists.txt             # Build configuration
?   ??? common/                    # Shared test utilities
?   ?   ??? TestHelpers.h         # Helper functions
?   ?   ??? TestHelpers.cpp       
?   ??? PositionTest.cpp          # C++ version of PositionTest.c
?   ??? LimitOrderTest.cpp        # C++ version of LimitOrderTest.c
?   ??? MultiPositionTest.cpp     # C++ version of FiveBarTest.c
??? scripts/                        # Original Lite-C (keep for reference)
?   ??? *.c
??? CMakeLists.txt                 # Add zorro-tests subdirectory
```

---

## Implementation

### Step 1: Create Test Helper Library

**zorro-tests/common/TestHelpers.h**
```cpp
#pragma once
#include <trading.h>
#include <string>
#include <vector>

namespace ZorroTest {

// Test result tracking
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

class TestRunner {
public:
    static void ReportResult(const char* testName, bool passed, const char* message = "");
    static void PrintSummary();
    static int GetFailCount() { return failCount; }
    static int GetPassCount() { return passCount; }
    
private:
    static int passCount;
    static int failCount;
    static std::vector<TestResult> results;
};

// Assertion helpers
#define ASSERT_EQ(actual, expected, msg) \
    do { \
        bool pass = ((actual) == (expected)); \
        ZorroTest::TestRunner::ReportResult(msg, pass, \
            pass ? "" : ("Expected " + std::to_string(expected) + \
                        " but got " + std::to_string(actual)).c_str()); \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    ZorroTest::TestRunner::ReportResult(msg, (condition))

// Order helpers
inline int PlaceMarketLong(int quantity = 1) {
    return enterLong(quantity);
}

inline int PlaceMarketShort(int quantity = 1) {
    return enterShort(quantity);
}

inline int PlaceLimitLong(int quantity, var limitPrice) {
    OrderLimit = limitPrice;
    return enterLong(quantity);
}

inline int PlaceLimitShort(int quantity, var limitPrice) {
    OrderLimit = limitPrice;
    return enterShort(quantity);
}

} // namespace ZorroTest
```

**zorro-tests/common/TestHelpers.cpp**
```cpp
#include "TestHelpers.h"
#include <stdio.h>

namespace ZorroTest {

int TestRunner::passCount = 0;
int TestRunner::failCount = 0;
std::vector<TestResult> TestRunner::results;

void TestRunner::ReportResult(const char* testName, bool passed, const char* message) {
    if (passed) {
        passCount++;
        printf("\n? PASS: %s", testName);
    } else {
        failCount++;
        printf("\n? FAIL: %s", testName);
        if (message && *message) {
            printf(" - %s", message);
        }
    }
    
    results.push_back({testName, passed, message ? message : ""});
}

void TestRunner::PrintSummary() {
    printf("\n\n========================================");
    printf("\n   Test Summary");
    printf("\n========================================");
    printf("\n  Total: %d", passCount + failCount);
    printf("\n  Passed: %d", passCount);
    printf("\n  Failed: %d", failCount);
    printf("\n========================================\n");
}

} // namespace ZorroTest
```

---

### Step 2: Create C++ Test Scripts

**zorro-tests/PositionTest.cpp**
```cpp
// PositionTest.cpp - Position tracking test in C++
#include <trading.h>
#include "common/TestHelpers.h"

using namespace ZorroTest;

DLLFUNC void run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 2);
        asset("MESH26");
        printf("\n=== C++ Position Test ===\n");
    }
    
    // Test 1: Long position
    printf("\n--- Test LONG ---");
    PlaceMarketLong(1);
    
    ASSERT_EQ(NumOpenLong, 1, "Long position opened");
    ASSERT_EQ(NumOpenShort, 0, "No short positions");
    
    exitLong();
    ASSERT_EQ(NumOpenLong, 0, "Long position closed");
    
    // Test 2: Short position
    printf("\n--- Test SHORT ---");
    PlaceMarketShort(1);
    
    ASSERT_EQ(NumOpenShort, 1, "Short position opened");
    ASSERT_EQ(NumOpenLong, 0, "No long positions");
    
    exitShort();
    ASSERT_EQ(NumOpenShort, 0, "Short position closed");
    
    // Print summary
    TestRunner::PrintSummary();
    
    if(TestRunner::GetFailCount() == 0) {
        quit("All tests passed!");
    } else {
        quit("Some tests failed");
    }
}
```

**zorro-tests/LimitOrderTest.cpp**
```cpp
// LimitOrderTest.cpp - Limit order testing in C++
#include <trading.h>
#include "common/TestHelpers.h"

using namespace ZorroTest;

DLLFUNC void run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 1);
        asset("MESH26");
        printf("\n=== C++ Limit Order Test ===\n");
    }
    
    var currentPrice = priceClose();
    var limitPrice = currentPrice - 2.0;  // 2 points below market
    
    printf("\nCurrent Price: %.2f", currentPrice);
    printf("\nLimit Price: %.2f\n", limitPrice);
    
    // Place limit order
    int tradeID = PlaceLimitLong(1, limitPrice);
    
    ASSERT_TRUE(tradeID > 0, "Order placed successfully");
    
    // Check if pending
    if(NumPendingLong > 0) {
        printf("\n? Order is pending (expected)");
        
        // Cancel it
        exitLong();
        ASSERT_EQ(NumPendingLong, 0, "Order canceled");
    } else {
        printf("\n? Order filled immediately");
        ASSERT_EQ(NumOpenLong, 1, "Position opened");
        
        // Close it
        exitLong();
    }
    
    TestRunner::PrintSummary();
    quit("Test complete");
}
```

---

### Step 3: CMake Build Configuration

**zorro-tests/CMakeLists.txt**
```cmake
# Build C++ test scripts as DLLs

# Common test helpers library
add_library(ZorroTestHelpers STATIC
    common/TestHelpers.cpp
)

target_include_directories(ZorroTestHelpers PUBLIC
    common
    ${CMAKE_SOURCE_DIR}/include
)

# Function to create a test script DLL
function(add_zorro_test TEST_NAME)
    add_library(${TEST_NAME} SHARED
        ${TEST_NAME}.cpp
    )
    
    target_link_libraries(${TEST_NAME} PRIVATE
        ZorroTestHelpers
    )
    
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        common
    )
    
    # Output to Zorro's Strategy folder
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/zorro/Strategy"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/zorro/Strategy"
    )
    
    # 32-bit (if Zorro is 32-bit)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_definitions(${TEST_NAME} PRIVATE _WIN32)
    endif()
endfunction()

# Create test DLLs
add_zorro_test(PositionTest)
add_zorro_test(LimitOrderTest)
add_zorro_test(MultiPositionTest)
```

**Root CMakeLists.txt** (add this)
```cmake
# Add test scripts
option(BUILD_ZORRO_TESTS "Build Zorro C++ test scripts" ON)

if(BUILD_ZORRO_TESTS)
    add_subdirectory(zorro-tests)
endif()
```

---

### Step 4: Usage

1. **Build the test DLLs:**
   ```bash
   cmake -B build -G Ninja -DBUILD_ZORRO_TESTS=ON
   cmake --build build
   ```

2. **Run in Zorro:**
   - Start Zorro
   - Select NT8-Sim account
   - Select **PositionTest.dll** instead of PositionTest.c
   - Click Trade
   - Watch it run with full C++ power!

3. **Debug in Visual Studio:**
   - Set breakpoint in `PositionTest.cpp`
   - Attach debugger to Zorro.exe
   - Step through your C++ code!

---

## Benefits of This Approach

? **Simple** - No complex mocking infrastructure
? **Real testing** - Tests actual NT8 behavior
? **C++ features** - STL, classes, templates, lambdas
? **Debugging** - Full Visual Studio debugging
? **Incremental** - Port one test at a time
? **Keep Lite-C** - Old tests still work as reference

---

## Comparison

| Approach | Complexity | Isolation | Debugging | Real NT8 |
|----------|-----------|-----------|-----------|----------|
| **Lite-C scripts** | Low | No | Limited | Yes |
| **C++ DLL tests** | Low | No | **Full** | Yes |
| **Mock framework** | High | Yes | Full | No |

**Recommendation:** Use C++ DLL approach for now, consider mock framework later if needed.

---

## Next Steps

1. Create `zorro-tests/` directory
2. Implement `TestHelpers.h/cpp`
3. Port one Lite-C test to C++ (PositionTest)
4. Build and verify it works
5. Port remaining tests incrementally

---

## File Checklist

- [ ] `zorro-tests/CMakeLists.txt`
- [ ] `zorro-tests/common/TestHelpers.h`
- [ ] `zorro-tests/common/TestHelpers.cpp`
- [ ] `zorro-tests/PositionTest.cpp`
- [ ] `zorro-tests/LimitOrderTest.cpp`
- [ ] `zorro-tests/MultiPositionTest.cpp`
- [ ] `CMakeLists.txt` (updated)

---

**Status**: Ready to implement
**Complexity**: Low (just DLL compilation)
**Timeline**: 1-2 days
**Risk**: Low (uses standard Zorro features)
