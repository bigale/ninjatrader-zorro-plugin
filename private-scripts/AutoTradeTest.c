// AutoTradeTest.c - Automated Trade Test
// Tests long and short trades without manual intervention
//
// NOTE: This script may not work in Zorro Free edition
// Zorro Free requires manual button clicks for each trade
// Use TradeTest.c for manual testing instead

////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////
#define BARPERIOD 1
#define VERBOSE 2
#define LOG_TRADES
#define ASSET "BTCUSD"

// Try to enable automated trading
#pragma warning disable 
#define RULES

////////////////////////////////////////////////////////////
// Test State Machine
////////////////////////////////////////////////////////////
int g_TestPhase = 0;
int g_TradeID = 0;
var g_EntryPrice = 0;
var g_ExitPrice = 0;
int g_WaitCounter = 0;
int g_TestsFailed = 0;

////////////////////////////////////////////////////////////
// Trade Execution Functions (using call() like TradeTest)
////////////////////////////////////////////////////////////
void doLongEntry()
{
	printf("\n[doLongEntry] Executing enterLong()");
	printf("\n  Lots before: %d", Lots);
	Lots = 1;
	printf("\n  Lots after: %d", Lots);
	printf("\n  Price: %.2f", priceClose());
	printf("\n  is(LOOKBACK): %d", is(LOOKBACK));
	
	g_TradeID = enterLong();
	
	printf("\n[doLongEntry] Returned ID: %d", g_TradeID);
	//printf("\n  NumPending: %d", NumPending);
	printf("\n  NumOpenLong: %d", NumOpenLong);
	
	if(g_TradeID > 0) {
		printf("\n[PASS] LONG trade placed, ID: %d", g_TradeID);
		g_TestPhase = 2; // Move to wait for fill
	} else {
		printf("\n[FAIL] enterLong returned 0 - this may be a Zorro Free limitation");
		printf("\n  Zorro Free may require manual button clicks for trades");
		printf("\n  Try using TradeTest.c instead for manual testing");
		g_TestsFailed++;
		quit("Long entry failed - use TradeTest.c for manual testing");
	}
}

void doShortEntry()
{
	printf("\n[doShortEntry] Executing enterShort()");
	Lots = 1;
	g_TradeID = enterShort();
	printf("\n[doShortEntry] Returned ID: %d", g_TradeID);
	
	if(g_TradeID > 0) {
		printf("\n[PASS] SHORT trade placed, ID: %d", g_TradeID);
		g_TestPhase = 7; // Move to wait for fill
	} else {
		printf("\n[FAIL] enterShort returned 0 - this may be a Zorro Free limitation");
		printf("\n  Zorro Free may require manual button clicks for trades");
		printf("\n  Try using TradeTest.c instead for manual testing");
		g_TestsFailed++;
		quit("Short entry failed - use TradeTest.c for manual testing");
	}
}

void doLongExit()
{
	printf("\n[doLongExit] Executing exitLong()");
	exitLong();
	g_TestPhase = 4; // Move to wait for close
}

void doShortExit()
{
	printf("\n[doShortExit] Executing exitShort()");
	exitShort();
	g_TestPhase = 9; // Move to wait for close
}

////////////////////////////////////////////////////////////
// Main Function
////////////////////////////////////////////////////////////
function run()
{
	BarPeriod = BARPERIOD;
	LookBack = 0; // Live only
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);
		asset(ASSET);
		
		// Match TradeTest.c settings
		TradesPerBar = 1;
		
		g_TestPhase = 0;
		g_WaitCounter = 0;
		g_TestsFailed = 0;
		
		printf("\n========================================");
		printf("\n=== Automated Trade Test ===");
		printf("\n========================================");
		printf("\nAsset: %s", Asset);
		printf("\nPrice: %.2f", priceClose());
		printf("\nBalance: $%.2f", Balance);
		printf("\nConnected: %d", Live);
		printf("\n");
		printf("\n** Using call() + wait() pattern **");
		printf("\n** Click [Trade] to start **");
		printf("\n");
	}
	
	// State machine for automated testing
	switch(g_TestPhase) {
		
		// Phase 0: Initial wait
		case 0:
			if(priceClose() > 0) {
				printf("\n[PHASE 0] Market data received");
				printf("\n[PHASE 1] Placing LONG order...");
				call(1, doLongEntry, 0, 0);
				g_TestPhase = 1; // Wait for call() to execute
			} else {
				printf("\n[PHASE 0] Waiting for market data...");
			}
			break;
		
		// Phase 1: Long entry called, waiting for execution
		case 1:
			// doLongEntry will move us to Phase 2
			wait(100); // Keep event loop alive
			break;
			
		// Phase 2: Wait for Long Fill
		case 2:
			g_WaitCounter++;
			printf("\n[PHASE 2] Waiting for LONG fill... (%d)", g_WaitCounter);
			
			if(NumOpenLong > 0) {
				printf("\n[PASS] LONG position opened");
				printf("\n        Entry: %.2f", TradePriceOpen);
				g_EntryPrice = TradePriceOpen;
				g_TestPhase = 3;
				g_WaitCounter = 0;
				wait(3000); // Wait 3 seconds before closing
			} else if(g_WaitCounter > 30) {
				printf("\n[FAIL] LONG entry timeout");
				g_TestsFailed++;
				ExitCode = 1;
				quit("Long entry timeout");
			} else {
				wait(500); // Wait 0.5 sec between checks
			}
			break;
			
		// Phase 3: Close Long
		case 3:
			printf("\n[PHASE 3] Closing LONG position...");
			call(1, doLongExit, 0, 0);
			// doLongExit will move us to Phase 4
			break;
			
		// Phase 4: Wait for Long Close
		case 4:
			g_WaitCounter++;
			printf("\n[PHASE 4] Waiting for LONG close... (%d)", g_WaitCounter);
			
			if(NumOpenLong == 0) {
				printf("\n[PASS] LONG position closed");
				printf("\n        Exit: %.2f", TradePriceClose);
				printf("\n        P&L: $%.2f", TradeProfit);
				g_ExitPrice = TradePriceClose;
				g_TestPhase = 5;
				g_WaitCounter = 0;
				wait(2000); // Wait 2 seconds before SHORT
			} else if(g_WaitCounter > 30) {
				printf("\n[FAIL] LONG exit timeout");
				g_TestsFailed++;
				ExitCode = 2;
				quit("Long exit timeout");
			} else {
				wait(500);
			}
			break;
			
		// Phase 5: Start Short
		case 5:
			printf("\n[PHASE 5] Placing SHORT order...");
			call(1, doShortEntry, 0, 0);
			g_TestPhase = 6; // Wait for call() to execute
			break;
		
		// Phase 6: Short entry called, waiting for execution
		case 6:
			// doShortEntry will move us to Phase 7
			wait(100);
			break;
			
		// Phase 7: Wait for Short Fill
		case 7:
			g_WaitCounter++;
			printf("\n[PHASE 7] Waiting for SHORT fill... (%d)", g_WaitCounter);
			
			if(NumOpenShort > 0) {
				printf("\n[PASS] SHORT position opened");
				printf("\n        Entry: %.2f", TradePriceOpen);
				g_EntryPrice = TradePriceOpen;
				g_TestPhase = 8;
				g_WaitCounter = 0;
				wait(3000); // Wait 3 seconds before closing
			} else if(g_WaitCounter > 30) {
				printf("\n[FAIL] SHORT entry timeout");
				g_TestsFailed++;
				ExitCode = 3;
				quit("Short entry timeout");
			} else {
				wait(500);
			}
			break;
			
		// Phase 8: Close Short
		case 8:
			printf("\n[PHASE 8] Closing SHORT position...");
			call(1, doShortExit, 0, 0);
			// doShortExit will move us to Phase 9
			break;
			
		// Phase 9: Wait for Short Close
		case 9:
			g_WaitCounter++;
			printf("\n[PHASE 9] Waiting for SHORT close... (%d)", g_WaitCounter);
			
			if(NumOpenShort == 0) {
				printf("\n[PASS] SHORT position closed");
				printf("\n        Exit: %.2f", TradePriceClose);
				printf("\n        P&L: $%.2f", TradeProfit);
				g_ExitPrice = TradePriceClose;
				g_TestPhase = 10;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 30) {
				printf("\n[FAIL] SHORT exit timeout");
				g_TestsFailed++;
				ExitCode = 4;
				quit("Short exit timeout");
			} else {
				wait(500);
			}
			break;
			
		// Phase 10: Complete
		case 10:
			printf("\n");
			printf("\n========================================");
			printf("\n=== Test Complete ===");
			printf("\n========================================");
			printf("\n[PASS] LONG trade (open/close)");
			printf("\n[PASS] SHORT trade (open/close)");
			printf("\n");
			printf("\nAll automated trades successful!");
			printf("\n========================================\n");
			
			if(g_TestsFailed > 0) {
				ExitCode = g_TestsFailed;
				quit("Tests failed!");
			} else {
				ExitCode = 0;
				quit("All tests passed!");
			}
			break;
	}
}
