// TestHelpers.cpp - Implementation of test utilities

#include "TestHelpers.h"

namespace ZorroTest {

// Static member initialization
int TestRunner::s_passCount = 0;
int TestRunner::s_failCount = 0;
std::vector<TestResult> TestRunner::s_results;

void TestRunner::ReportResult(const char* testName, bool passed, const char* message) {
    if (passed) {
        s_passCount++;
        printf("\n? PASS: %s", testName);
    } else {
        s_failCount++;
        printf("\n? FAIL: %s", testName);
        if (message && *message) {
            printf("\n       %s", message);
        }
    }
    
    s_results.push_back({testName, passed, message ? message : ""});
}

void TestRunner::PrintSummary() {
    printf("\n\n========================================");
    printf("\n   Test Summary");
    printf("\n========================================");
    printf("\n  Total Tests: %d", s_passCount + s_failCount);
    printf("\n  Passed: %d", s_passCount);
    printf("\n  Failed: %d", s_failCount);
    
    if (s_failCount > 0) {
        printf("\n\n  Failed Tests:");
        for (const auto& result : s_results) {
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

void TestRunner::Reset() {
    s_passCount = 0;
    s_failCount = 0;
    s_results.clear();
}

} // namespace ZorroTest
