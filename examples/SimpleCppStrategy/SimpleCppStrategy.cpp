// SimpleCppStrategy.cpp - Example C++ Zorro Trading Strategy
// Demonstrates proper C++ strategy development workflow
//
// This is a REAL trading strategy (not a test script) compiled as a DLL
// that links against zorro.lib and uses the full Zorro API.
//
// Build: cmake --build . --config Release
// Run: Load SimpleCppStrategy.dll in Zorro

#include <trading.h>  // Zorro API - from C:\Zorro\include\trading.h

//=============================================================================
// Strategy Parameters
//=============================================================================

static int g_fastPeriod = 10;   // Fast SMA period
static int g_slowPeriod = 20;   // Slow SMA period
static int g_positionSize = 1;  // Position size in lots

//=============================================================================
// Required Export - Zorro calls this every bar/tick
//=============================================================================

DLLFUNC void run()
{
    //=========================================================================
    // Initialization (called once at start)
    //=========================================================================
    if(is(INITRUN)) {
        BarPeriod = 60;           // 1-hour bars
        LookBack = 100;           // Need 100 bars of history
        
        // Select asset to trade
        asset("EUR/USD");
        
        // Optional: Set broker commands
        brokerCommand(SET_DIAGNOSTICS, 1);  // Info level logging
        
        printf("\n========================================");
        printf("\n  Simple C++ Strategy Example");
        printf("\n========================================");
        printf("\nAsset: %s", Asset);
        printf("\nBar Period: %d minutes", (int)BarPeriod);
        printf("\nFast SMA: %d", g_fastPeriod);
        printf("\nSlow SMA: %d", g_slowPeriod);
        printf("\n========================================\n");
    }
    
    //=========================================================================
    // Cleanup (called once at end)
    //=========================================================================
    if(is(EXITRUN)) {
        printf("\nStrategy stopped. Final results:\n");
        printf("  Bars processed: %d\n", (int)Bar);
        printf("  Open positions: %d\n", NumOpenTotal);
    }
    
    //=========================================================================
    // Skip bars until we have enough history
    //=========================================================================
    if(Bar < g_slowPeriod) {
        return;  // Not enough data yet
    }
    
    //=========================================================================
    // Calculate indicators
    //=========================================================================
    vars closePrices = series(priceClose());  // Price series
    
    var fastSMA = SMA(closePrices, g_fastPeriod);
    var slowSMA = SMA(closePrices, g_slowPeriod);
    
    //=========================================================================
    // Strategy Logic - Simple SMA Crossover
    //=========================================================================
    
    // Entry Signal: Fast SMA crosses above Slow SMA
    if(crossOver(fastSMA, slowSMA)) {
        // Close any short positions first
        if(NumOpenShort > 0) {
            exitShort();
        }
        
        // Enter long
        if(NumOpenLong == 0) {
            printf("\n[Bar %d] BUY SIGNAL - Fast SMA crossed above Slow SMA", (int)Bar);
            printf("\n  Fast SMA: %.5f", fastSMA);
            printf("\n  Slow SMA: %.5f", slowSMA);
            printf("\n  Price: %.5f\n", priceClose());
            
            enterLong(g_positionSize);
        }
    }
    
    // Exit Signal: Fast SMA crosses below Slow SMA
    if(crossUnder(fastSMA, slowSMA)) {
        // Close any long positions first
        if(NumOpenLong > 0) {
            exitLong();
        }
        
        // Enter short
        if(NumOpenShort == 0) {
            printf("\n[Bar %d] SELL SIGNAL - Fast SMA crossed below Slow SMA", (int)Bar);
            printf("\n  Fast SMA: %.5f", fastSMA);
            printf("\n  Slow SMA: %.5f", slowSMA);
            printf("\n  Price: %.5f\n", priceClose());
            
            enterShort(g_positionSize);
        }
    }
    
    //=========================================================================
    // Optional: Status output every 100 bars
    //=========================================================================
    if((int)Bar % 100 == 0) {
        printf("[Bar %d] Status - Fast SMA: %.5f | Slow SMA: %.5f | Positions: %d\n",
            (int)Bar, fastSMA, slowSMA, NumOpenTotal);
    }
}

//=============================================================================
// Optional: Additional exported functions
//=============================================================================

// Called by Zorro to get strategy information
DLLFUNC const char* about()
{
    return "Simple C++ Strategy - SMA Crossover Example";
}
