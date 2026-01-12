# C++ Testing Framework Plan for NT8 Plugin

## Executive Summary

This document outlines the plan to create a comprehensive C++ testing framework for the NinjaTrader 8 Zorro plugin. The goal is to eliminate dependency on Lite-C test scripts and Zorro executable for testing, enabling fast, automated testing during development.

---

## Table of Contents

1. [Background & Motivation](#background--motivation)
2. [Architecture Overview](#architecture-overview)
3. [Technical Approach](#technical-approach)
4. [Implementation Phases](#implementation-phases)
5. [Directory Structure](#directory-structure)
6. [Test Framework Components](#test-framework-components)
7. [Example Test Cases](#example-test-cases)
8. [Build Integration](#build-integration)
9. [Benefits & Trade-offs](#benefits--trade-offs)
10. [Timeline](#timeline)

---

## Background & Motivation

### Current Testing Approach (Lite-C Scripts)

**Problems:**
- ? Requires Zorro executable to run tests
- ? Requires NinjaTrader to be running
- ? Tests run slowly (bar-based execution)
- ? Market must be open or use Market Replay
- ? Lite-C syntax limitations and compile errors
- ? No automated CI/CD integration possible
- ? Difficult to debug (no step-through debugging)
- ? No unit test isolation

**Current Test Scripts:**
- `PositionTest.c` - Position tracking validation
- `PositionTest2.c` - NumOpen tracking tests
- `FiveBarTest.c` - Multiple position accumulation
- `LimitOrderTest.c` - Limit order placement
- `LimitOrderDebug.c` - Limit order monitoring

### Proposed Approach (C++ Testing)

**Advantages:**
- ? Tests run standalone (no Zorro executable needed)
- ? Fast execution (milliseconds vs minutes)
- ? Automated CI/CD integration
- ? Full debugger support (breakpoints, step-through)
- ? Mock NT8 responses for predictable testing
- ? Unit test isolation
- ? Standard C++ testing frameworks (Catch2, Google Test)
- ? Works on developer machine without broker

---

## Architecture Overview

```
???????????????????????????????????????????????????????
?              C++ Test Executable                    ?
?  ?????????????????????????????????????????????????  ?
?  ?         Test Framework                        ?  ?
?  ?  - Mock Zorro Environment                     ?  ?
?  ?  - Mock Broker Callbacks                      ?  ?
?  ?  - Assertion Helpers                          ?  ?
?  ?????????????????????????????????????????????????  ?
?                      ?                              ?
?  ?????????????????????????????????????????????????  ?
?  ?         NT8Plugin Functions                   ?  ?
?  ?  - BrokerOpen / BrokerLogin                   ?  ?
?  ?  - BrokerBuy2 / BrokerSell2                   ?  ?
?  ?  - BrokerCommand (GET_POSITION, etc.)         ?  ?
?  ?????????????????????????????????????????????????  ?
?                      ?                              ?
?  ?????????????????????????????????????????????????  ?
?  ?         Mock TcpBridge                        ?  ?
?  ?  - Simulates NT8 responses                    ?  ?
?  ?  - Predictable order fills                    ?  ?
?  ?  - Configurable market data                   ?  ?
?  ?????????????????????????????????????????????????  ?
???????????????????????????????????????????????????????
```

**Key Insight:** The plugin already has clean separation between:
1. **Zorro API** (BrokerOpen, BrokerLogin, etc.) ? Test these
2. **TcpBridge** (NT8 communication) ? Mock this
3. **Plugin Logic** (position cache, order tracking) ? Verify this

We can test #1 and #3 by mocking #2!

---

## Technical Approach

### 1. Mock TcpBridge

Instead of connecting to real NT8, create `MockTcpBridge` that:
- Inherits from `TcpBridge` interface
- Returns predictable responses
- Simulates order fills instantly or after delay
- Tracks all commands sent

```cpp
class MockTcpBridge : public TcpBridge {
public:
    // Override to return canned responses
    std::string SendCommand(const std::string& command) override;
    
    // Configure mock behavior
    void SetMarketPrice(const char* instrument, double price);
    void SetOrderFillBehavior(FillBehavior behavior);
    
    // Inspection methods for testing
    int GetOrderCount() const;
    std::string GetLastCommand() const;
};
```

### 2. Test Harness

Create a harness that:
- Initializes plugin with mock bridge
- Provides helper functions for common operations
- Includes assertion macros
- Manages test lifecycle

```cpp
class ZorroTestHarness {
public:
    ZorroTestHarness();
    ~ZorroTestHarness();
    
    // Setup
    void Initialize(const char* account = "TestAccount");
    void SetAsset(const char* symbol, double price = 100.0);
    
    // Broker API wrappers
    int PlaceMarketOrder(bool isLong, int quantity);
    int PlaceLimitOrder(bool isLong, int quantity, double limitPrice);
    void CancelOrder(int tradeID);
    
    // Assertions
    void AssertPosition(const char* symbol, int expected);
    void AssertOrderStatus(int tradeID, const char* expectedStatus);
    void AssertCachedPosition(const char* symbol, int expected);
    
    // Mock control
    MockTcpBridge* GetMock() { return mockBridge; }
    
private:
    std::unique_ptr<MockTcpBridge> mockBridge;
    // Store callback pointers
    int (*BrokerMessage)(const char*);
    int (*BrokerProgress)(const int);
};
```

### 3. Test Framework Choice

**Recommended: Catch2**
- Header-only option (no external dependencies)
- Modern C++ syntax
- Excellent reporting
- Easy integration

**Alternative: Google Test**
- More features (mocking, parameterized tests)
- Requires linking
- Industry standard

**Minimal: Custom Framework**
- Simple assertion macros
- No external dependencies
- Limited features

### 4. CMake Integration

Add test target to build system:

```cmake
# test/CMakeLists.txt
enable_testing()

# Option 1: Catch2 (header-only)
add_executable(nt8_tests
    framework/ZorroTestHarness.cpp
    framework/MockTcpBridge.cpp
    cases/PositionTests.cpp
    cases/LimitOrderTests.cpp
    cases/OrderTrackingTests.cpp
    ../src/NT8Plugin.cpp  # Link against plugin code
)

target_include_directories(nt8_tests PRIVATE
    ../include
    framework
    ${CMAKE_SOURCE_DIR}/external/catch2
)

add_test(NAME nt8_tests COMMAND nt8_tests)
```

---

## Implementation Phases

### Phase 1: Foundation (Week 1)
**Goal:** Basic test infrastructure

**Tasks:**
1. Create `test/` directory structure
2. Implement `MockTcpBridge` basic functionality
3. Implement `ZorroTestHarness` core functions
4. Add Catch2 to project
5. Create first simple test (BrokerOpen)
6. Integrate with CMake

**Deliverables:**
- `test/framework/MockTcpBridge.h/cpp`
- `test/framework/ZorroTestHarness.h/cpp`
- `test/cases/BasicTests.cpp`
- `test/CMakeLists.txt`
- One passing test

### Phase 2: Core Functionality (Week 2)
**Goal:** Test all critical broker functions

**Tasks:**
1. Port `PositionTest2.c` logic to C++
2. Test market order placement and fills
3. Test position cache updates
4. Test GET_POSITION command
5. Test position tracking (long/short)

**Deliverables:**
- `test/cases/PositionTests.cpp`
- `test/cases/MarketOrderTests.cpp`
- All position tests passing

### Phase 3: Advanced Features (Week 3)
**Goal:** Test limit orders and edge cases

**Tasks:**
1. Port `LimitOrderTest.c` logic
2. Implement mock limit order fills
3. Test order cancellation
4. Test multiple simultaneous orders
5. Test order ID mapping (ZORRO_xxx ? NT GUID)

**Deliverables:**
- `test/cases/LimitOrderTests.cpp`
- `test/cases/OrderTrackingTests.cpp`
- Limit order tests passing

### Phase 4: Regression & CI (Week 4)
**Goal:** Comprehensive test coverage and automation

**Tasks:**
1. Add edge case tests
2. Test error conditions
3. Memory leak checks (Valgrind/AddressSanitizer)
4. CI integration (GitHub Actions)
5. Performance benchmarks
6. Documentation

**Deliverables:**
- `test/cases/ErrorConditionTests.cpp`
- `.github/workflows/test.yml`
- `docs/CPP_TESTING_GUIDE.md`
- 90%+ code coverage

---

## Directory Structure

```
ninjatrader-zorro-plugin/
??? CMakeLists.txt                    # Add test subdirectory
??? include/
?   ??? NT8Plugin.h
?   ??? TcpBridge.h
??? src/
?   ??? NT8Plugin.cpp
?   ??? TcpBridge.cpp
??? test/                             # NEW
?   ??? CMakeLists.txt               # Test build config
?   ??? framework/                    # Test infrastructure
?   ?   ??? MockTcpBridge.h
?   ?   ??? MockTcpBridge.cpp
?   ?   ??? ZorroTestHarness.h
?   ?   ??? ZorroTestHarness.cpp
?   ??? cases/                        # Actual tests
?   ?   ??? BasicTests.cpp           # BrokerOpen, BrokerLogin
?   ?   ??? PositionTests.cpp        # Position cache, GET_POSITION
?   ?   ??? MarketOrderTests.cpp     # Market orders, fills
?   ?   ??? LimitOrderTests.cpp      # Limit orders, cancels
?   ?   ??? OrderTrackingTests.cpp   # Order ID mapping
?   ?   ??? ErrorConditionTests.cpp  # Edge cases, errors
?   ??? external/                     # Third-party (optional)
?       ??? catch2/
?           ??? catch.hpp             # Single-header Catch2
??? scripts/                          # Keep Lite-C for integration tests
?   ??? LimitOrderTest.c
??? private-scripts/                  # Development scripts
    ??? PositionTest2.c
```

---

## Test Framework Components

### MockTcpBridge.h

```cpp
#pragma once
#include "TcpBridge.h"
#include <map>
#include <vector>
#include <functional>

enum class FillBehavior {
    IMMEDIATE,      // Orders fill instantly
    DELAYED,        // Orders fill after N calls
    NEVER,          // Orders never fill (stay pending)
    ON_PRICE_MATCH  // Orders fill when price reaches limit
};

class MockTcpBridge : public TcpBridge {
public:
    MockTcpBridge();
    ~MockTcpBridge() override = default;
    
    // Override TcpBridge methods
    std::string SendCommand(const std::string& command) override;
    
    // Configuration
    void SetMarketPrice(const char* instrument, double last, double bid, double ask);
    void SetOrderFillBehavior(FillBehavior behavior, int delayCount = 0);
    void SetAccountBalance(double cashValue, double buyingPower);
    
    // Inspection
    int GetOrderCount() const { return orderCount; }
    std::string GetLastCommand() const { return lastCommand; }
    std::vector<std::string> GetAllCommands() const { return commandHistory; }
    
    // Simulate events
    void SimulateOrderFill(const std::string& orderId, double fillPrice);
    void SimulatePriceUpdate(const char* instrument, double newPrice);
    
private:
    struct MockOrder {
        std::string orderId;
        std::string action;      // BUY/SELL
        std::string orderType;   // MARKET/LIMIT
        int quantity;
        double limitPrice;
        int filled;
        double avgFillPrice;
        std::string state;       // Submitted/Filled/Cancelled
    };
    
    struct MockPrice {
        double last;
        double bid;
        double ask;
        double volume;
    };
    
    struct MockPosition {
        int quantity;            // Signed: + for long, - for short
        double avgPrice;
    };
    
    std::map<std::string, MockOrder> orders;
    std::map<std::string, MockPrice> prices;
    std::map<std::string, MockPosition> positions;
    
    FillBehavior fillBehavior;
    int fillDelayCounter;
    int fillDelayMax;
    
    int orderCount;
    std::string lastCommand;
    std::vector<std::string> commandHistory;
    
    // Command handlers
    std::string HandlePlaceOrder(const std::string& command);
    std::string HandleGetPosition(const std::string& command);
    std::string HandleGetPrice(const std::string& command);
    std::string HandleGetOrderStatus(const std::string& command);
    std::string HandleCancelOrder(const std::string& command);
    
    // Helpers
    std::string GenerateOrderId();
    void UpdatePosition(const std::string& instrument, const std::string& action, int quantity, double price);
};
```

### ZorroTestHarness.h

```cpp
#pragma once
#include "MockTcpBridge.h"
#include "NT8Plugin.h"
#include <memory>
#include <string>

class ZorroTestHarness {
public:
    ZorroTestHarness();
    ~ZorroTestHarness();
    
    // Initialization
    void Initialize(const char* account = "TestAccount");
    void SetAsset(const char* symbol, double price = 100.0, double pip = 0.25);
    void Cleanup();
    
    // Broker API wrappers
    int Open(const char* pluginName = "NT8");
    int Login(const char* account);
    void Logout();
    
    int PlaceMarketOrder(bool isLong, int quantity);
    int PlaceLimitOrder(bool isLong, int quantity, double limitPrice);
    int ClosePosition(int tradeID);
    int CancelOrder(int tradeID);
    
    // Query functions
    int GetPosition(const char* symbol);
    double GetAvgEntry(const char* symbol);
    int GetTradeStatus(int tradeID, double* pOpen, double* pClose);
    
    // Mock control
    MockTcpBridge* GetMock() { return mockBridge.get(); }
    void SimulateFill(int tradeID, double fillPrice);
    void SimulatePriceMove(const char* symbol, double newPrice);
    
    // Assertions
    void AssertPosition(const char* symbol, int expected, const char* message = nullptr);
    void AssertCachedPosition(const char* symbol, int expected);
    void AssertOrderCount(int expected);
    void AssertOrderStatus(int tradeID, const char* expectedStatus);
    
    // Utilities
    void PrintState(const char* label = nullptr);
    void ResetState();
    
private:
    std::unique_ptr<MockTcpBridge> mockBridge;
    std::string currentAccount;
    std::string currentSymbol;
    bool isInitialized;
    
    // Callback stubs
    static int MessageCallback(const char* text);
    static int ProgressCallback(const int progress);
    
    // Store last error for assertions
    std::string lastError;
};

// Assertion macros
#define ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            throw std::runtime_error(std::string(message) + \
                ": Expected " + std::to_string(expected) + \
                " but got " + std::to_string(actual)); \
        } \
    } while(0)

#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error(std::string(message) + \
                ": Condition failed"); \
        } \
    } while(0)
```

---

## Example Test Cases

### BasicTests.cpp

```cpp
#include "catch.hpp"
#include "ZorroTestHarness.h"

TEST_CASE("Plugin opens and returns version", "[basic]") {
    ZorroTestHarness harness;
    
    int version = harness.Open("NT8");
    
    REQUIRE(version == PLUGIN_VERSION);
}

TEST_CASE("Login succeeds with valid account", "[basic]") {
    ZorroTestHarness harness;
    harness.Open("NT8");
    
    int result = harness.Login("Sim101");
    
    REQUIRE(result == 1);  // Success
}

TEST_CASE("Login fails with NULL account", "[basic][error]") {
    ZorroTestHarness harness;
    harness.Open("NT8");
    
    int result = harness.Login(nullptr);
    
    REQUIRE(result == 0);  // Failure
}
```

### PositionTests.cpp

```cpp
#include "catch.hpp"
#include "ZorroTestHarness.h"

TEST_CASE("Position cache updates on long fill", "[position]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    
    // Configure mock to fill immediately
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::IMMEDIATE);
    
    // Place market order
    int tradeID = harness.PlaceMarketOrder(true, 1);  // BUY 1
    
    // Verify position cache
    harness.AssertCachedPosition("MES 03-26", 1);
}

TEST_CASE("Position cache updates on short fill", "[position]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::IMMEDIATE);
    
    int tradeID = harness.PlaceMarketOrder(false, 1);  // SELL 1
    
    harness.AssertCachedPosition("MES 03-26", -1);
}

TEST_CASE("GET_POSITION returns absolute value", "[position]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    
    // Enter short (cache = -1)
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::IMMEDIATE);
    harness.PlaceMarketOrder(false, 1);
    
    // GET_POSITION should return abs(-1) = 1
    int position = harness.GetPosition("MES 03-26");
    
    REQUIRE(position == 1);  // Absolute value!
}

TEST_CASE("Multiple positions accumulate correctly", "[position]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::IMMEDIATE);
    
    // Enter 3 long positions
    harness.PlaceMarketOrder(true, 1);
    harness.PlaceMarketOrder(true, 1);
    harness.PlaceMarketOrder(true, 1);
    
    harness.AssertCachedPosition("MES 03-26", 3);
}
```

### LimitOrderTests.cpp

```cpp
#include "catch.hpp"
#include "ZorroTestHarness.h"

TEST_CASE("Limit buy order stays pending when above market", "[limit]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    
    // Configure to never fill automatically
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::NEVER);
    
    // Place limit BUY at 7002 (above market 7000)
    int tradeID = harness.PlaceLimitOrder(true, 1, 7002.0);
    
    // Should NOT fill
    harness.AssertCachedPosition("MES 03-26", 0);
    harness.AssertOrderStatus(tradeID, "Submitted");
}

TEST_CASE("Limit buy fills when price drops to limit", "[limit]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::ON_PRICE_MATCH);
    
    // Place limit BUY at 6998 (below market)
    int tradeID = harness.PlaceLimitOrder(true, 1, 6998.0);
    
    // Simulate price drop
    harness.SimulatePriceMove("MES 03-26", 6998.0);
    
    // Should fill
    harness.AssertCachedPosition("MES 03-26", 1);
    harness.AssertOrderStatus(tradeID, "Filled");
}

TEST_CASE("Canceling limit order clears pending", "[limit]") {
    ZorroTestHarness harness;
    harness.Initialize();
    harness.SetAsset("MES 03-26", 7000.0);
    
    harness.GetMock()->SetOrderFillBehavior(FillBehavior::NEVER);
    
    int tradeID = harness.PlaceLimitOrder(true, 1, 7002.0);
    
    // Cancel the order
    harness.CancelOrder(tradeID);
    
    // Order should be canceled
    harness.AssertOrderStatus(tradeID, "Cancelled");
    harness.AssertCachedPosition("MES 03-26", 0);
}
```

---

## Build Integration

### Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(NT8Plugin VERSION 1.0.0 LANGUAGES CXX)

# ... existing configuration ...

# Option to enable tests
option(BUILD_TESTS "Build test suite" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
```

### test/CMakeLists.txt

```cmake
# Download Catch2 (single header)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/catch2/catch.hpp)
    file(DOWNLOAD
        https://github.com/catchorg/Catch2/releases/download/v2.13.10/catch.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/external/catch2/catch.hpp
        SHOW_PROGRESS
    )
endif()

# Test executable
add_executable(nt8_tests
    # Framework
    framework/MockTcpBridge.cpp
    framework/ZorroTestHarness.cpp
    
    # Test cases
    cases/BasicTests.cpp
    cases/PositionTests.cpp
    cases/MarketOrderTests.cpp
    cases/LimitOrderTests.cpp
    cases/OrderTrackingTests.cpp
    
    # Plugin source (link against implementation)
    ../src/NT8Plugin.cpp
    # Note: TcpBridge.cpp is NOT included - we use MockTcpBridge instead
)

target_include_directories(nt8_tests PRIVATE
    ../include
    framework
    external/catch2
)

target_compile_features(nt8_tests PRIVATE cxx_std_17)

# Register with CTest
add_test(NAME nt8_tests COMMAND nt8_tests)

# Optional: Run tests after build
add_custom_command(TARGET nt8_tests POST_BUILD
    COMMAND nt8_tests
    COMMENT "Running tests..."
)
```

### Running Tests

```bash
# Build with tests
cmake -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build

# Run tests
cd build
ctest --output-on-failure

# Or run directly
./nt8_tests

# Run specific test
./nt8_tests "[position]"

# Verbose output
./nt8_tests -s
```

---

## Benefits & Trade-offs

### Benefits

? **Speed**: Tests run in milliseconds vs minutes
? **Isolation**: No NT8, no Zorro, no network
? **Debugging**: Full debugger support
? **CI/CD**: Automated testing on every commit
? **Coverage**: Test edge cases easily
? **Reliability**: No market data dependencies
? **Refactoring**: Catch regressions instantly

### Trade-offs

? **Mocking complexity**: Need to simulate NT8 behavior accurately
? **Integration gaps**: Doesn't test actual NT8 communication
? **Maintenance**: Mock must stay in sync with real TcpBridge
? **Initial effort**: Setup time required

### Mitigation Strategy

1. **Keep Lite-C integration tests**: Use for end-to-end validation
2. **Mock validation**: Periodically verify mock matches real NT8 behavior
3. **Layered testing**: Unit tests (C++) + Integration tests (Lite-C)

---

## Timeline

### Week 1: Foundation
- [ ] Create directory structure
- [ ] Implement MockTcpBridge skeleton
- [ ] Implement ZorroTestHarness skeleton
- [ ] Add Catch2
- [ ] First passing test (BrokerOpen)
- [ ] CMake integration

### Week 2: Core Tests
- [ ] Position cache tests
- [ ] Market order tests
- [ ] GET_POSITION tests
- [ ] Order fill simulation
- [ ] Port PositionTest2.c logic

### Week 3: Advanced Tests
- [ ] Limit order tests
- [ ] Order cancellation
- [ ] Multiple orders
- [ ] Order ID mapping
- [ ] Port LimitOrderTest.c logic

### Week 4: Polish
- [ ] Error condition tests
- [ ] Edge cases
- [ ] CI integration
- [ ] Documentation
- [ ] Code coverage report

---

## Success Criteria

1. ? All tests run without NT8 or Zorro
2. ? Tests execute in < 1 second total
3. ? 90%+ code coverage of plugin logic
4. ? CI runs tests automatically
5. ? All existing Lite-C test scenarios covered
6. ? Developer can debug tests with breakpoints
7. ? New contributors can add tests easily

---

## Future Enhancements

### Phase 2 (Optional)
- Performance benchmarking
- Thread safety tests
- Stress testing (1000s of orders)
- Memory leak detection (Valgrind/ASan)
- Fuzzing (random inputs)

### Phase 3 (Advanced)
- Property-based testing
- Mutation testing
- Test coverage dashboard
- Automated test generation

---

## References

- [Catch2 Documentation](https://github.com/catchorg/Catch2)
- [CMake Testing](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [C++ Mock Patterns](https://martinfowler.com/articles/mocksArentStubs.html)
- Zorro Manual: Broker API specification

---

## Appendix: File Checklist

### Framework Files
- [ ] `test/framework/MockTcpBridge.h`
- [ ] `test/framework/MockTcpBridge.cpp`
- [ ] `test/framework/ZorroTestHarness.h`
- [ ] `test/framework/ZorroTestHarness.cpp`

### Test Case Files
- [ ] `test/cases/BasicTests.cpp`
- [ ] `test/cases/PositionTests.cpp`
- [ ] `test/cases/MarketOrderTests.cpp`
- [ ] `test/cases/LimitOrderTests.cpp`
- [ ] `test/cases/OrderTrackingTests.cpp`
- [ ] `test/cases/ErrorConditionTests.cpp`

### Build Files
- [ ] `test/CMakeLists.txt`
- [ ] `CMakeLists.txt` (updated)
- [ ] `.github/workflows/test.yml` (CI)

### Documentation
- [ ] `docs/CPP_TESTING_GUIDE.md`
- [ ] `docs/CPP_TESTING_PLAN.md` (this file)

---

**Status**: Planning Complete
**Next Step**: Begin Phase 1 implementation
**Owner**: Development Team
**Review Date**: After each phase completion
