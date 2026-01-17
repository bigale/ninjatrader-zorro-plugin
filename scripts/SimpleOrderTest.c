// SimpleOrderTest.c - Minimal order test for Zorro 2.70 debugging
// This script tests ONE order at a time to diagnose the issue

function run()
{
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 2);  // Maximum logging
		asset("MESH26");
		printf("\n========================================");
		printf("\n=== Simple Order Test for Zorro 2.70 ===");
		printf("\n========================================\n");
	}
	
	// Wait for market data
	if(NumBars < 3) {
		printf("\n[Bar %d] Waiting for market data...", NumBars);
		printf(" Price: %.2f", priceClose());
		return;
	}
	
	// Test 1: Place ONE market order
	if(NumBars == 5) {
		printf("\n\n[TEST 1] Placing MARKET BUY order...");
		printf("\n  Asset: %s", Asset);
		printf("\n  Price: %.2f", priceClose());
		printf("\n  Spread: %.2f\n", Spread);
		
		int id = enterLong(1);
		
		printf("\n[RESULT] enterLong() returned: %d", id);
		
		if(id > 0) {
			printf(" (POSITIVE = should be filled)");
		} else if(id < 0) {
			printf(" (NEGATIVE = pending)");
		} else {
			printf(" (ZERO = failed!)");
		}
	}
	
	// Test 2: Check position after 3 seconds
	if(NumBars == 8) {
		printf("\n\n[TEST 2] Checking position...");
		
		int pos = brokerCommand(GET_POSITION, (long)Asset);
		printf("\n  GET_POSITION returned: %d contracts", pos);
		printf("\n  NumOpenLong: %d", NumOpenLong);
		printf("\n  NumOpenShort: %d", NumOpenShort);
		printf("\n  NumOpenTotal: %d", NumOpenTotal);
		
		if(NumOpenLong == 1 && pos == 1) {
			printf("\n  ? Position CORRECT!");
		} else if(NumOpenLong == 1 && pos == 0) {
			printf("\n  ? ERROR: Zorro says OPEN, broker says FLAT!");
		} else if(NumOpenLong == 0 && pos == 1) {
			printf("\n  ? ERROR: Zorro says FLAT, broker says OPEN!");
		} else {
			printf("\n  ? ERROR: Mismatch! Zorro=%d Broker=%d", NumOpenLong, pos);
		}
	}
	
	// Test 3: Close position
	if(NumBars == 12) {
		printf("\n\n[TEST 3] Attempting to close...");
		
		if(NumOpenLong > 0) {
			printf("\n  Closing LONG position...");
			exitLong();
		} else {
			printf("\n  ? NO POSITION TO CLOSE (this is the problem!)");
		}
	}
	
	// Test 4: Verify closed
	if(NumBars == 15) {
		printf("\n\n[TEST 4] Final position check...");
		
		int pos = brokerCommand(GET_POSITION, (long)Asset);
		printf("\n  GET_POSITION: %d", pos);
		printf("\n  NumOpenTotal: %d", NumOpenTotal);
		
		if(pos == 0 && NumOpenTotal == 0) {
			printf("\n  ? ALL CLOSED");
		} else {
			printf("\n  ? ERROR: Still have positions!");
		}
	}
	
	// Done
	if(NumBars >= 18) {
		printf("\n\n========================================");
		printf("\n=== Test Complete ===");
		printf("\n========================================\n");
		quit("Simple order test finished");
	}
}
