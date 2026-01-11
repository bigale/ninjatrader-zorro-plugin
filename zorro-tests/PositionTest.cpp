// PositionTest.cpp - C++ version of PositionTest2.c
// Tests position tracking using Zorro's built-in variables
// Build as DLL and load in Zorro just like a .c script

#include <trading.h>
#include "TestHelpers.h"

using namespace ZorroTest;

//=============================================================================
// DLL Entry Point - Zorro calls this function
//=============================================================================

DLLFUNC void run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug logging
        asset("MESH26");
        
        PrintTestHeader("C++ Position Tracking Test");
        printf("Testing Zorro's built-in position tracking\n");
        printf("Using C++ with full debugging support!\n");
    }
    
    // Test 1: LONG position tracking
    printf("\n--- Test LONG Position ---\n");
    printf("Calling enterLong(1)...\n");
    
    PlaceMarketLong(1);
    
    printf("Zorro NumOpenLong: %d\n", NumOpenLong);
    printf("Zorro NumOpenShort: %d\n", NumOpenShort);
    printf("Zorro NumOpenTotal: %d\n", NumOpenTotal);
    
    ASSERT_EQ(NumOpenLong, 1, "Long position opened");
    ASSERT_EQ(NumOpenShort, 0, "No short positions");
    ASSERT_EQ(NumOpenTotal, 1, "Total positions = 1");
    
    // Close the long
    printf("\nClosing long position...\n");
    exitLong();
    
    printf("After exitLong:\n");
    printf("  NumOpenLong: %d\n", NumOpenLong);
    printf("  NumOpenTotal: %d\n", NumOpenTotal);
    
    ASSERT_EQ(NumOpenLong, 0, "Long position closed");
    ASSERT_EQ(NumOpenTotal, 0, "No open positions");
    
    // Test 2: SHORT position tracking
    printf("\n--- Test SHORT Position ---\n");
    printf("Calling enterShort(1)...\n");
    
    PlaceMarketShort(1);
    
    printf("Zorro NumOpenLong: %d\n", NumOpenLong);
    printf("Zorro NumOpenShort: %d\n", NumOpenShort);
    printf("Zorro NumOpenTotal: %d\n", NumOpenTotal);
    
    ASSERT_EQ(NumOpenShort, 1, "Short position opened");
    ASSERT_EQ(NumOpenLong, 0, "No long positions");
    ASSERT_EQ(NumOpenTotal, 1, "Total positions = 1");
    
    // Close the short
    printf("\nClosing short position...\n");
    exitShort();
    
    printf("After exitShort:\n");
    printf("  NumOpenShort: %d\n", NumOpenShort);
    printf("  NumOpenTotal: %d\n", NumOpenTotal);
    
    ASSERT_EQ(NumOpenShort, 0, "Short position closed");
    ASSERT_EQ(NumOpenTotal, 0, "No open positions");
    
    // Print test summary
    TestRunner::PrintSummary();
    
    // Quit with appropriate message
    if(TestRunner::GetFailCount() == 0) {
        quit("? All position tests PASSED!");
    } else {
        quit("? Some position tests FAILED");
    }
}
