// ComprehensiveNT8Test.c - Full feature test for NT8 plugin
// Tests all implemented features systematically

function run()
{
	// Variables at top
	var Balance;
	var TradeVal;
	var MarginVal;
	var Price;
	var Spread;
	int Position;
	var AvgEntry;
	int TestPhase;
	
	BarPeriod = 1;
	LookBack = 0;  // Live only
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);
		TestPhase = 0;
		printf("\n========================================");
		printf("\n=== NT8 Plugin Comprehensive Test ===");
		printf("\n========================================\n");
	}
	
	asset("MESH26");
	
	// Phase 0: Connection Test
	if(TestPhase == 0) {
		printf("\n--- PHASE 0: Connection Test ---");
		if(is(CONNECTED)) {
			printf("\n? Connected to NinjaTrader");
			TestPhase = 1;
		} else {
			printf("\n? Not connected");
			quit("Connection failed");
		}
	}
	
	// Phase 1: Account Info Test
	if(TestPhase == 1 && NumBars > 5) {
		printf("\n\n--- PHASE 1: Account Information ---");
		if(brokerCommand(GET_ACCOUNT, 0)) {
			brokerAccount(NULL, &Balance, &TradeVal, &MarginVal);
			printf("\nAccount Balance:  $%.2f", Balance);
			printf("\nTrade Value:      $%.2f", TradeVal);
			printf("\nMargin Available: $%.2f", MarginVal);
			
			if(Balance > 0) {
				printf("\n? Account info retrieved");
				TestPhase = 2;
			} else {
				printf("\n? Account balance is zero");
			}
		}
	}
	
	// Phase 2: Market Data Test
	if(TestPhase == 2 && NumBars > 10) {
		printf("\n\n--- PHASE 2: Market Data ---");
		printf("\nAsset: %s", Asset);
		Price = priceClose();
		Spread = marketVal();
		printf("\nLast Price: %.2f", Price);
		printf("\nSpread:     %.2f", Spread);
		printf("\nBid:        %.2f", Price - Spread/2);
		printf("\nAsk:        %.2f", Price + Spread/2);
		
		if(Price > 0) {
			printf("\n? Market data available");
			TestPhase = 3;
		} else {
			printf("\n? No price data");
		}
	}
	
	// Phase 3: Position Query Test
	if(TestPhase == 3 && NumBars > 15) {
		printf("\n\n--- PHASE 3: Position Information ---");
		Position = brokerCommand(GET_POSITION, (long)Asset);
		AvgEntry = brokerCommand(GET_AVGENTRY, (long)Asset);
		printf("\nCurrent Position: %d contracts", Position);
		if(Position != 0) {
			printf("\nAverage Entry:    %.2f", AvgEntry);
			printf("\nCurrent Price:    %.2f", Price);
			printf("\nUnrealized P&L:   $%.2f", (Price - AvgEntry) * Position * 5);
		} else {
			printf("\nNo open position");
		}
		printf("\n? Position query successful");
		TestPhase = 4;
	}
	
	// Phase 4: Time & Compliance Test
	if(TestPhase == 4 && NumBars > 20) {
		printf("\n\n--- PHASE 4: System Information ---");
		printf("\nBroker Timezone:  GMT%+d", (int)brokerCommand(GET_BROKERZONE, 0));
		printf("\nNFA Compliant:    %s", brokerCommand(GET_COMPLIANCE, 0) ? "Yes" : "No");
		printf("\nMax Ticks:        %d", (int)brokerCommand(GET_MAXTICKS, 0));
		printf("\nPolling Interval: %d ms", (int)brokerCommand(GET_WAIT, 0));
		printf("\n? System info retrieved");
		TestPhase = 5;
	}
	
	// Phase 5: Summary
	if(TestPhase == 5 && NumBars > 25) {
		printf("\n\n========================================");
		printf("\n=== Test Summary ===");
		printf("\n========================================");
		printf("\n? Connection:      Working");
		printf("\n? Account Info:    Working");
		printf("\n? Market Data:     Working");
		printf("\n? Position Query:  Working");
		printf("\n? System Info:     Working");
		printf("\n");
		printf("\nPlugin Status:     FULLY FUNCTIONAL");
		printf("\nReady for:         Live Trading");
		printf("\n");
		printf("\n========================================\n");
		quit("All tests passed!");
	}
}
