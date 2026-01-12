// TestHelpers.h - Common utilities for Zorro C++ test scripts
// Provides assertion macros and helper functions for testing
// HEADER-ONLY implementation for simplicity

#pragma once

#ifndef TESTHELPERS_H
#define TESTHELPERS_H

#include <trading_zorro.h>  // Changed from trading.h
#include <string>
#include <vector>
#include <cstdio>

namespace ZorroTest {

//=============================================================================
// Test Result Tracking - Implementation inline
//=============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

class TestRunner {
public:
    static void ReportResult(const char* testName, bool passed, const char* message = "") {
        if (passed) {
            GetPassCount()++;
            printf("\n? PASS: %s", testName);
        } else {
            GetFailCount()++;
            printf("\n? FAIL: %s", testName);
            if (message && *message) {
                printf("\n       %s", message);
            }
        }
        
        GetResults().push_back({testName, passed, message ? message : ""});
    }
    
    static void PrintSummary() {
        printf("\n\n========================================");
        printf("\n   Test Summary");
        printf("\n========================================");
        printf("\n  Total Tests: %d", GetPassCount() + GetFailCount());
        printf("\n  Passed: %d", GetPassCount());
        printf("\n  Failed: %d", GetFailCount());
        
        if (GetFailCount() > 0) {
            printf("\n\n  Failed Tests:");
            for (const auto& result : GetResults()) {
                if (!result.passed) {
                    printf("\n    - %s", result.name.c_str());
                    if (!result.message.empty()) {
                        printf("\n      %s", result.message.c_str());
                    }
                }
            }
        }
        
        printf("\n========================================\n");
    }
    
    static int& GetFailCount() { static int count = 0; return count; }
    static int& GetPassCount() { static int count = 0; return count; }
    static std::vector<TestResult>& GetResults() { static std::vector<TestResult> results; return results; }
    
    static void Reset() {
        GetPassCount() = 0;
        GetFailCount() = 0;
        GetResults().clear();
    }
};

//=============================================================================
// Assertion Macros
//=============================================================================

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        auto _actual = (actual); \
        auto _expected = (expected); \
        bool _pass = (_actual == _expected); \
        char _buf[256]; \
        if (!_pass) { \
            snprintf(_buf, sizeof(_buf), "%s: Expected %d but got %d", msg, (int)_expected, (int)_actual); \
            ZorroTest::TestRunner::ReportResult(msg, false, _buf); \
        } else { \
            ZorroTest::TestRunner::ReportResult(msg, true); \
        } \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        bool _pass = (condition); \
        ZorroTest::TestRunner::ReportResult(msg, _pass, \
            _pass ? "" : #condition " failed"); \
    } while(0)

#define ASSERT_FALSE(condition, msg) \
    do { \
        bool _pass = !(condition); \
        ZorroTest::TestRunner::ReportResult(msg, _pass, \
            _pass ? "" : #condition " should be false"); \
    } while(0)

#define ASSERT_GT(actual, value, msg) \
    do { \
        auto _actual = (actual); \
        auto _value = (value); \
        bool _pass = (_actual > _value); \
        char _buf[256]; \
        if (!_pass) { \
            snprintf(_buf, sizeof(_buf), "%s: Expected > %d but got %d", msg, (int)_value, (int)_actual); \
            ZorroTest::TestRunner::ReportResult(msg, false, _buf); \
        } else { \
            ZorroTest::TestRunner::ReportResult(msg, true); \
        } \
    } while(0)

//=============================================================================
// Order Helper Functions - Inline implementations
//=============================================================================

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

inline void CloseAllLong() {
    while (NumOpenLong > 0) {
        exitLong();
    }
}

inline void CloseAllShort() {
    while (NumOpenShort > 0) {
        exitShort();
    }
}

inline void CloseAll() {
    CloseAllLong();
    CloseAllShort();
}

//=============================================================================
// Utility Functions
//=============================================================================

inline void PrintTestHeader(const char* testName) {
    printf("\n========================================\n");
    printf("   %s\n", testName);
    printf("========================================\n");
}

inline void WaitSeconds(int seconds) {
    wait(seconds * 1000);
}

} // namespace ZorroTest

#endif // TESTHELPERS_H
