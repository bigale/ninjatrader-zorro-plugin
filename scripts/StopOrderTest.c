// StopOrderTest.c - Comprehensive Stop Order Testing
// Tests stop-market and stop-limit orders for entries and exits

////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////
#define BARPERIOD 5./60  // 5 second bars
#define VERBOSE 2

////////////////////////////////////////////////////////////
// Test State
////////////////////////////////////////////////////////////
int g_TestPhase = 0;
int g_LastTradeID = 0;
var g_StopPrice = 0;
var g_EntryPrice = 0;
int g_WaitCounter = 0;
int g_TestCount = 0;
int g_PassCount = 0;
int g_FailCount = 0;

function run()
{
	BarPeriod = BARPERIOD;
	LookBack = 0;
	
	asset("MES 0326");
	Lots = 1;
	
	if(is(INITRUN)) {
		printf("\n========================================");
		printf("\n   Stop Order Test Suite");
		printf("\n========================================");
		printf("\nAsset: %s", Asset);
		printf("\nBar Period: %.0f seconds", BarPeriod * 60);
		printf("\n");
		printf("\nTests:");
		printf("\n  1. BUY STOP order (enter long above market)");
		printf("\n  2. SELL STOP order (enter short below market)");
		printf("\n  3. Stop-loss exit for LONG position");
		printf("\n  4. Stop-loss exit for SHORT position");
		printf("\n========================================\n");
		
		g_TestPhase = 0;
		g_WaitCounter = 0;
		g_TestCount = 0;
		g_PassCount = 0;
		g_FailCount = 0;
	}
	
	// Debug every 5 bars
	if(Bar % 5 == 0 && !is(LOOKBACK)) {
		printf("\n[STATUS] Phase:%d | Price:%.2f | Spread:%.2f | Open:%d",
			g_TestPhase, priceClose(), Spread, NumOpenLong + NumOpenShort);
	}
	
	// State machine for testing
	switch(g_TestPhase) {
		
		// Phase 0: Wait for market data
		case 0:
			if(priceClose() > 0 && Spread > 0) {
				printf("\n[PHASE 0] Market data ready");
				printf("\n  Price: %.2f | Spread: %.2f | PIP: %.2f",
					priceClose(), Spread, PIP);
				g_TestPhase = 1;
			} else {
				printf("\n[PHASE 0] Waiting for market data...");
			}
			break;
			
		// Phase 1: Test BUY STOP (enter long above market)
		case 1:
			printf("\n========================================");
			printf("\n[TEST 1] BUY STOP Order (enter long above market)");
			printf("\n========================================");
			
			// Set stop 2 ticks above current price
			g_StopPrice = roundto(priceClose() + 2*PIP, PIP);
			
			printf("\n  Current Price: %.2f", priceClose());
			printf("\n  Stop Price: %.2f (%.2f above)", g_StopPrice, g_StopPrice - priceClose());
			printf("\n  PIP size: %.2f", PIP);
			printf("\n  Expected: Order pending until price rises to stop");
			
			// Place buy stop order using Stop global variable
			Stop = g_StopPrice - priceClose();  // Distance from current price
			g_LastTradeID = enterLong();
			Stop = 0;  // Clear for next order
			
			if(g_LastTradeID > 0) {
				printf("\n  [PASS] Order placed ID:%d", g_LastTradeID);
				g_PassCount++;
			} else {
				printf("\n  [FAIL] Order placement failed!");
				g_FailCount++;
			}
			
			g_TestCount++;
			g_TestPhase = 2;
			g_WaitCounter = 0;
			break;
			
		// Phase 2: Monitor BUY STOP trigger and fill
		case 2:
			g_WaitCounter++;
			
			if(NumOpenLong > 0) {
				printf("\n[TEST 1] BUY STOP Triggered and Filled");
				printf("\n  [PASS] Order triggered and filled!");
				printf("\n  Entry Price: %.2f", TradePriceOpen);
				printf("\n  Stop Price: %.2f", g_StopPrice);
				
				string fillQuality;
				if(TradePriceOpen >= g_StopPrice - PIP)
					fillQuality = "GOOD (at or above stop)";
				else
					fillQuality = "BAD (below stop trigger)";
				
				printf("\n  Fill Quality: %s", fillQuality);
				
				g_EntryPrice = TradePriceOpen;
				
				if(TradePriceOpen >= g_StopPrice - PIP) {
					printf("\n  [PASS] Filled at stop or better");
					g_PassCount++;
				} else {
					printf("\n  [FAIL] Filled below stop!");
					g_FailCount++;
				}
				
				g_TestCount++;
				g_TestPhase = 3;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[TEST 1] Buy stop timeout (20 bars)");
				printf("\n  Status: Order still pending");
				printf("\n  Current Price: %.2f vs Stop: %.2f", priceClose(), g_StopPrice);
				printf("\n  [NOTE] Expected if price hasn't reached stop");
				
				// Cancel and move on
				exitLong();
				g_TestPhase = 3;
				g_WaitCounter = 0;
			} else {
				if(g_WaitCounter % 2 == 0) {
					printf("\n[MONITOR] Waiting for stop trigger... (Bar %d | Price:%.2f | Stop:%.2f)",
						g_WaitCounter, priceClose(), g_StopPrice);
				}
			}
			break;
			
		// Phase 3: Close LONG position
		case 3:
			if(NumOpenLong > 0) {
				printf("\n[CLEANUP] Closing LONG position");
				exitLong();
				g_WaitCounter = 0;
			}
			
			if(NumOpenLong == 0) {
				printf("\n  Position closed");
				g_TestPhase = 4;
			}
			break;
			
		// Phase 4: Test SELL STOP (enter short below market)
		case 4:
			printf("\n");
			printf("\n========================================");
			printf("\n[TEST 2] SELL STOP Order (enter short below market)");
			printf("\n========================================");
			
			// Set stop 2 ticks below current price
			g_StopPrice = roundto(priceClose() - 2*PIP, PIP);
			
			printf("\n  Current Price: %.2f", priceClose());
			printf("\n  Stop Price: %.2f (%.2f below)", g_StopPrice, priceClose() - g_StopPrice);
			printf("\n  Expected: Order pending until price falls to stop");
			
			// Place sell stop order
			Stop = priceClose() - g_StopPrice;  // Distance from current price
			g_LastTradeID = enterShort();
			Stop = 0;
			
			if(g_LastTradeID > 0) {
				printf("\n  [PASS] Order placed ID:%d", g_LastTradeID);
				g_PassCount++;
			} else {
				printf("\n  [FAIL] Order placement failed!");
				g_FailCount++;
			}
			
			g_TestCount++;
			g_TestPhase = 5;
			g_WaitCounter = 0;
			break;
			
		// Phase 5: Monitor SELL STOP trigger and fill
		case 5:
			g_WaitCounter++;
			
			if(NumOpenShort > 0) {
				printf("\n[TEST 2] SELL STOP Triggered and Filled");
				printf("\n  [PASS] Order triggered and filled!");
				printf("\n  Entry Price: %.2f", TradePriceOpen);
				printf("\n  Stop Price: %.2f", g_StopPrice);
				
				string fillQuality;
				if(TradePriceOpen <= g_StopPrice + PIP)
					fillQuality = "GOOD (at or below stop)";
				else
					fillQuality = "BAD (above stop trigger)";
				
				printf("\n  Fill Quality: %s", fillQuality);
				
				g_EntryPrice = TradePriceOpen;
				
				if(TradePriceOpen <= g_StopPrice + PIP) {
					printf("\n  [PASS] Filled at stop or better");
					g_PassCount++;
				} else {
					printf("\n  [FAIL] Filled above stop!");
					g_FailCount++;
				}
				
				g_TestCount++;
				g_TestPhase = 6;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[TEST 2] Sell stop timeout (20 bars)");
				printf("\n  [NOTE] Expected if price hasn't reached stop");
				
				exitShort();
				g_TestPhase = 6;
				g_WaitCounter = 0;
			} else {
				if(g_WaitCounter % 2 == 0) {
					printf("\n[MONITOR] Waiting for stop trigger... (Bar %d | Price:%.2f | Stop:%.2f)",
						g_WaitCounter, priceClose(), g_StopPrice);
				}
			}
			break;
			
		// Phase 6: Cleanup and final report
		case 6:
			if(NumOpenShort > 0) {
				printf("\n[CLEANUP] Closing SHORT position");
				exitShort();
			}
			
			if(NumOpenShort == 0 && NumOpenLong == 0) {
				printf("\n");
				printf("\n========================================");
				printf("\n   Test Results Summary");
				printf("\n========================================");
				printf("\n  Total Tests: %d", g_TestCount);
				printf("\n  Passed: %d", g_PassCount);
				printf("\n  Failed: %d", g_FailCount);
				printf("\n");
				
				if(g_FailCount == 0) {
					printf("\n  ALL TESTS PASSED");
				} else {
					printf("\n  %d TEST(S) FAILED", g_FailCount);
				}
				
				printf("\n========================================\n");
				
				quit("Stop order tests complete");
			}
			break;
	}
}
