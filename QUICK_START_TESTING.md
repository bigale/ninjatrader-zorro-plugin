# Quick Start: C++ DLL Testing

## What Just Got Implemented

You now have **automated C++ test scripts** that replace the old Lite-C tests! ??

## How to Use

### Option 1: Automated Test Runner (Recommended)

```bash
# From repository root
.\test-runner.bat
```

This will:
1. Build all test DLLs
2. Run them in Zorro automatically
3. Parse log files for pass/fail
4. Print summary

### Option 2: Manual Testing

```bash
# 1. Build the test DLL
cd build
cmake --build . --config Release --target PositionTest

# 2. Run in Zorro
# - Start NinjaTrader
# - Start Zorro
# - Select NT8-Sim account
# - Select **PositionTest.dll** (not .c!)
# - Click Trade
```

### Option 3: Visual Studio Debugging

```bash
# 1. Set breakpoint in zorro-tests/PositionTest.cpp
# 2. Build: cmake --build . --config Release --target PositionTest
# 3. In Visual Studio: Debug ? Attach to Process ? Zorro.exe
# 4. Run PositionTest.dll in Zorro
# 5. Breakpoint hits - step through C++ code!
```

## Available Tests

| Test DLL | Description | Log File |
|----------|-------------|----------|
| `PositionTest.dll` | Position tracking (long/short) | `C:\Zorro_2.66\Log\PositionTest_demo.log` |
| `LimitOrderTest.dll` | Limit order placement & cancel | `C:\Zorro_2.66\Log\LimitOrderTest_demo.log` |

## Key Improvements vs Lite-C

| Feature | Lite-C (.c) | C++ DLL (.dll) |
|---------|-------------|----------------|
| **Syntax** | Limited Lite-C | Full C++17 |
| **Debugging** | `printf` only | Visual Studio breakpoints |
| **Assertions** | Manual `if` checks | `ASSERT_EQ`, `ASSERT_TRUE` |
| **STL** | ? No | ? Yes (vector, string, etc.) |
| **Build** | Zorro compiles | CMake ? DLL |

## Next Steps

1. **Try it now:**
   ```bash
   .\test-runner.bat
   ```

2. **Create new test:**
   - Copy `zorro-tests/PositionTest.cpp`
   - Modify for your test case
   - Add to `zorro-tests/CMakeLists.txt`
   - Build and run!

3. **Port more Lite-C tests:**
   - FiveBarTest ? MultiPositionTest.cpp
   - Any custom tests you have

## Documentation

- **Full guide**: `zorro-tests/README.md`
- **Simple plan**: `docs/CPP_TESTING_SIMPLE.md`
- **Advanced plan**: `docs/CPP_TESTING_PLAN.md` (future)

---

**You now have professional-grade testing with full C++ power!** ??
