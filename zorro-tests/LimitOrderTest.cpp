// LimitOrderTest.cpp - C++ limit order testing
// Tests limit order placement, monitoring, and cancellation
// Build as DLL and load in Zorro

#include <trading_zorro.h>  // Changed from trading.h
#include "TestHelpers.h"

using namespace ZorroTest;

//=============================================================================
// DLL Entry Point
//=============================================================================

DLLFUNC void run()
{
    BarPeriod = 1;
    LookBack = 0;
    
    if(is(INITRUN)) {
        brokerCommand(SET_DIAGNOSTICS, 1);  // Info level logging
        asset("MESH26");
        
        PrintTestHeader("C++ Limit Order Test");
        printf("Testing limit order placement and cancellation\n");
    }
    
    var currentPrice = priceClose();
    var limitBuyPrice = currentPrice - 2.0;   // 2 points below market
    var limitSellPrice = currentPrice + 2.0;  // 2 points above market
    
    printf("\nCurrent Market Price: %.2f\n", currentPrice);
    printf("Limit BUY Price: %.2f (%.2f below market)\n", 
        limitBuyPrice, currentPrice - limitBuyPrice);
    printf("Limit SELL Price: %.2f (%.2f above market)\n",
        limitSellPrice, limitSellPrice - currentPrice);
    
    // Test 1: Place limit BUY order
    printf("\n--- Test 1: Limit BUY Order ---\n");
    
    int tradeID = PlaceLimitLong(1, limitBuyPrice);
    
    ASSERT_GT(tradeID, 0, "Limit BUY order placed");
    
    printf("Trade ID: %d\n", tradeID);
    printf("NumOpenLong: %d\n", NumOpenLong);
    printf("NumPendingLong: %d\n", NumPendingLong);
    
    // Check order status
    if(NumPendingLong > 0) {
        printf("? Order is PENDING (expected - below market)\n");
        ASSERT_TRUE(NumPendingLong == 1, "Order pending");
        
        // Cancel the order
        printf("Canceling order...\n");
        exitLong();
        
        ASSERT_EQ(NumPendingLong, 0, "Order canceled");
        printf("? Order canceled successfully\n");
    } else if(NumOpenLong > 0) {
        printf("? Order FILLED immediately (market moved to limit)\n");
        ASSERT_TRUE(NumOpenLong == 1, "Order filled");
        
        // Close the position
        exitLong();
        ASSERT_EQ(NumOpenLong, 0, "Position closed");
    }
    
    // Test 2: Place limit SELL order
    printf("\n--- Test 2: Limit SELL Order ---\n");
    
    tradeID = PlaceLimitShort(1, limitSellPrice);
    
    ASSERT_GT(tradeID, 0, "Limit SELL order placed");
    
    printf("Trade ID: %d\n", tradeID);
    printf("NumOpenShort: %d\n", NumOpenShort);
    printf("NumPendingShort: %d\n", NumPendingShort);
    
    // Check order status
    if(NumPendingShort > 0) {
        printf("? Order is PENDING (expected - above market)\n");
        ASSERT_TRUE(NumPendingShort == 1, "Order pending");
        
        // Cancel the order
        printf("Canceling order...\n");
        exitShort();
        
        ASSERT_EQ(NumPendingShort, 0, "Order canceled");
        printf("? Order canceled successfully\n");
    } else if(NumOpenShort > 0) {
        printf("? Order FILLED immediately (market moved to limit)\n");
        ASSERT_TRUE(NumOpenShort == 1, "Order filled");
        
        // Close the position
        exitShort();
        ASSERT_EQ(NumOpenShort, 0, "Position closed");
    }
    
    // Print test summary
    TestRunner::PrintSummary();
    
    // Quit with status
    if(TestRunner::GetFailCount() == 0) {
        quit("? All limit order tests PASSED!");
    } else {
        quit("? Some limit order tests FAILED");
    }
}
