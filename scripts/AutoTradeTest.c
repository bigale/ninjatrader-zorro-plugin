// AutoTradeTest.c - Automated Trade Test
// Tests long and short trades without manual intervention

////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////
#define BARPERIOD 1
#define VERBOSE 2
#define LOG_TRADES
#define ASSET "MES 0326"

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
// Main Function
////////////////////////////////////////////////////////////
function run()
{
	// CRITICAL: Skip market closed check BEFORE everything else
	Skip = 8;  // Skip market closed check (for simulated data testing)
	
	BarPeriod = BARPERIOD;
	LookBack = 0; // Live only
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);
		asset(ASSET);
		g_TestPhase = 0;
		g_WaitCounter = 0;
		g_TestsFailed = 0;
		
		printf("\n========================================");
		printf("\n=== Automated Trade Test ===");
		printf("\n========================================");
		printf("\nAsset: %s", Asset);
		printf("\nPrice: %.2f", priceClose());
		printf("\nSkip Market Hours Check: ENABLED");
		printf("\n");
	}
	
	// State machine for automated testing
	switch(g_TestPhase) {
		
		// Phase 0: Initial wait
		case 0:
			printf("\n[PHASE 0] Waiting for market data...");
			if(priceClose() > 0) {
				g_TestPhase = 1;
				g_WaitCounter = 0;
			}
			break;
			
		// Phase 1: Buy Long
		case 1:
			printf("\n[PHASE 1] Opening LONG position...");
			enterLong(1);
			g_TestPhase = 2;
			g_WaitCounter = 0;
			break;
			
		// Phase 2: Wait for Long Fill
		case 2:
			g_WaitCounter++;
			if(NumOpenLong > 0) {
				printf("\n[PASS] LONG position opened");
				g_EntryPrice = TradePriceOpen;
				printf("\n        Entry: %.2f", g_EntryPrice);
				g_TestPhase = 3;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[FAIL] LONG entry timeout");
				g_TestsFailed++;
				ExitCode = 1;
				quit("Long entry failed");
			}
			break;
			
		// Phase 3: Wait before closing Long
		case 3:
			g_WaitCounter++;
			if(g_WaitCounter > 3) {
				printf("\n[PHASE 3] Closing LONG position...");
				g_TestPhase = 4;
				g_WaitCounter = 0;
			}
			break;
			
		// Phase 4: Close Long
		case 4:
			exitLong();
			g_TestPhase = 5;
			g_WaitCounter = 0;
			break;
			
		// Phase 5: Wait for Long Close
		case 5:
			g_WaitCounter++;
			if(NumOpenLong == 0) {
				printf("\n[PASS] LONG position closed");
				g_ExitPrice = TradePriceClose;
				printf("\n        Exit: %.2f", g_ExitPrice);
				printf("\n        P&L: $%.2f", TradeProfit);
				g_TestPhase = 6;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[FAIL] LONG exit timeout");
				g_TestsFailed++;
				ExitCode = 2;
				quit("Long close failed");
			}
			break;
			
		// Phase 6: Wait before Short
		case 6:
			g_WaitCounter++;
			if(g_WaitCounter > 3) {
				printf("\n");
				g_TestPhase = 7;
				g_WaitCounter = 0;
			}
			break;
			
		// Phase 7: Buy Short
		case 7:
			printf("\n[PHASE 7] Opening SHORT position...");
			enterShort(1);
			g_TestPhase = 8;
			g_WaitCounter = 0;
			break;
			
		// Phase 8: Wait for Short Fill
		case 8:
			g_WaitCounter++;
			if(NumOpenShort > 0) {
				printf("\n[PASS] SHORT position opened");
				g_EntryPrice = TradePriceOpen;
				printf("\n        Entry: %.2f", g_EntryPrice);
				g_TestPhase = 9;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[FAIL] SHORT entry timeout");
				g_TestsFailed++;
				ExitCode = 3;
				quit("Short entry failed");
			}
			break;
			
		// Phase 9: Wait before closing Short
		case 9:
			g_WaitCounter++;
			if(g_WaitCounter > 3) {
				printf("\n[PHASE 9] Closing SHORT position...");
				g_TestPhase = 10;
				g_WaitCounter = 0;
			}
			break;
			
		// Phase 10: Close Short
		case 10:
			exitShort();
			g_TestPhase = 11;
			g_WaitCounter = 0;
			break;
			
		// Phase 11: Wait for Short Close
		case 11:
			g_WaitCounter++;
			if(NumOpenShort == 0) {
				printf("\n[PASS] SHORT position closed");
				g_ExitPrice = TradePriceClose;
				printf("\n        Exit: %.2f", g_ExitPrice);
				printf("\n        P&L: $%.2f", TradeProfit);
				g_TestPhase = 12;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[FAIL] SHORT exit timeout");
				g_TestsFailed++;
				ExitCode = 4;
				quit("Short close failed");
			}
			break;
			
		// Phase 12: Complete
		case 12:
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
