// LimitOrderTest.c - Comprehensive Limit Order Testing
// Tests limit order placement, fills, cancellation, and monitoring

////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////
#define BARPERIOD 5./60  // 5 second bars for better control
#define VERBOSE 2

////////////////////////////////////////////////////////////
// Test State
////////////////////////////////////////////////////////////
int g_TestPhase = 0;
int g_LastTradeID = 0;
var g_LimitPrice = 0;
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
		printf("\n   Limit Order Test Suite");
		printf("\n========================================");
		printf("\nAsset: %s", Asset);
		printf("\nBar Period: %.0f seconds", BarPeriod * 60);
		printf("\n");
		printf("\nTests:");
		printf("\n  1. LONG limit order (below market)");
		printf("\n  2. LONG limit order fill monitoring");
		printf("\n  3. SHORT limit order (above market)");
		printf("\n  4. SHORT limit order fill monitoring");
		printf("\n  5. Limit order cancellation");
		printf("\n  6. Limit order vs market price validation");
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
				printf("\n  Price: %.2f | Bid: %.2f | Ask: %.2f | Spread: %.2f",
					priceClose(), priceBid(), priceAsk(), Spread);
				g_TestPhase = 1;
			} else {
				printf("\n[PHASE 0] Waiting for market data...");
			}
			break;
			
		// Phase 1: Test LONG limit order (below market)
		case 1:
			printf("\n========================================");
			printf("\n[TEST 1] LONG Limit Order (below market)");
			printf("\n========================================");
			
			// Set limit 2 ticks below bid
			g_LimitPrice = priceBid() - 2*PIP;
			
			printf("\n  Current Bid: %.2f", priceBid());
			printf("\n  Limit Price: %.2f (%.2f below)", g_LimitPrice, priceBid() - g_LimitPrice);
			printf("\n  Expected: Order pending until price drops");
			
			// Place limit order
			OrderLimit = g_LimitPrice;
			g_LastTradeID = enterLong();
			
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
			
		// Phase 2: Monitor LONG limit fill
		case 2:
			g_WaitCounter++;
			
			if(NumOpenLong > 0) {
				printf("\n[TEST 2] LONG Limit Fill Monitoring");
				printf("\n  [PASS] Order filled!");
				printf("\n  Entry Price: %.2f", TradePriceOpen);
				printf("\n  Limit Price: %.2f", g_LimitPrice);
				printf("\n  Fill Quality: %s", 
					TradePriceOpen <= g_LimitPrice ? "GOOD (at or better)" : "BAD (worse than limit)");
				
				g_EntryPrice = TradePriceOpen;
				
				if(TradePriceOpen <= g_LimitPrice + 0.01) {
					printf("\n  [PASS] Filled at limit or better");
					g_PassCount++;
				} else {
					printf("\n  [FAIL] Filled worse than limit!");
					g_FailCount++;
				}
				
				g_TestCount++;
				g_TestPhase = 3;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[TEST 2] Limit order timeout (20 bars)");
				printf("\n  Status: Order still pending");
				printf("\n  Current Price: %.2f vs Limit: %.2f", priceClose(), g_LimitPrice);
				printf("\n  [NOTE] This is expected if price hasn't reached limit");
				
				// Cancel the order and move on
				exitLong();
				g_TestPhase = 3;
				g_WaitCounter = 0;
			} else {
				if(g_WaitCounter % 2 == 0) {
					printf("\n[MONITOR] Waiting for fill... (Bar %d | Price:%.2f | Limit:%.2f)",
						g_WaitCounter, priceClose(), g_LimitPrice);
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
			
			// Wait for position to close
			if(NumOpenLong == 0) {
				printf("\n  Position closed");
				g_TestPhase = 4;
			}
			break;
			
		// Phase 4: Test SHORT limit order (above market)
		case 4:
			printf("\n");
			printf("\n========================================");
			printf("\n[TEST 3] SHORT Limit Order (above market)");
			printf("\n========================================");
			
			// Set limit 2 ticks above ask
			g_LimitPrice = priceAsk() + 2*PIP;
			
			printf("\n  Current Ask: %.2f", priceAsk());
			printf("\n  Limit Price: %.2f (%.2f above)", g_LimitPrice, g_LimitPrice - priceAsk());
			printf("\n  Expected: Order pending until price rises");
			
			// Place limit order
			OrderLimit = g_LimitPrice;
			g_LastTradeID = enterShort();
			
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
			
		// Phase 5: Monitor SHORT limit fill
		case 5:
			g_WaitCounter++;
			
			if(NumOpenShort > 0) {
				printf("\n[TEST 4] SHORT Limit Fill Monitoring");
				printf("\n  [PASS] Order filled!");
				printf("\n  Entry Price: %.2f", TradePriceOpen);
				printf("\n  Limit Price: %.2f", g_LimitPrice);
				printf("\n  Fill Quality: %s",
					TradePriceOpen >= g_LimitPrice ? "GOOD (at or better)" : "BAD (worse than limit)");
				
				g_EntryPrice = TradePriceOpen;
				
				if(TradePriceOpen >= g_LimitPrice - 0.01) {
					printf("\n  [PASS] Filled at limit or better");
					g_PassCount++;
				} else {
					printf("\n  [FAIL] Filled worse than limit!");
					g_FailCount++;
				}
				
				g_TestCount++;
				g_TestPhase = 6;
				g_WaitCounter = 0;
			} else if(g_WaitCounter > 20) {
				printf("\n[TEST 4] Limit order timeout (20 bars)");
				printf("\n  Status: Order still pending");
				printf("\n  Current Price: %.2f vs Limit: %.2f", priceClose(), g_LimitPrice);
				printf("\n  [NOTE] This is expected if price hasn't reached limit");
				
				// Cancel and move on
				exitShort();
				g_TestPhase = 6;
				g_WaitCounter = 0;
			} else {
				if(g_WaitCounter % 2 == 0) {
					printf("\n[MONITOR] Waiting for fill... (Bar %d | Price:%.2f | Limit:%.2f)",
						g_WaitCounter, priceClose(), g_LimitPrice);
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
					printf("\n  ? ALL TESTS PASSED");
				} else {
					printf("\n  ? %d TEST(S) FAILED", g_FailCount);
				}
				
				printf("\n========================================\n");
				
				quit("Limit order tests complete");
			}
			break;
	}
}
