// FillAndCloseTest.c - Test limit order fills and position closing
// Demonstrates full order lifecycle: place limit -> wait for fill -> close position

function run()
{
	var currentPrice;
	var limitPrice;
	TRADE* tr;  // Use TRADE pointer, not int
	int attempts;
	int maxAttempts;
	
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug
		asset("MESH26");
		printf("\n=== Fill and Close Test ===\n");
		printf("This test places limit orders ABOVE/BELOW market\n");
		printf("to ensure they fill quickly, then closes the positions.\n");
	}
	
	//==========================================================================
	// TEST 1: LONG LIMIT ORDER - Place above market to ensure fill
	//==========================================================================
	printf("\n========================================\n");
	printf("TEST 1: LONG LIMIT ORDER (above market)\n");
	printf("========================================\n");
	
	currentPrice = priceClose();
	printf("\nCurrent market price: %.2f\n", currentPrice);
	
	// Place limit BUY order ABOVE market (will fill immediately or very quickly)
	limitPrice = currentPrice + 0.75;  // Above market to ensure fill
	printf("Placing limit BUY order at %.2f (%.2f ABOVE market)\n",
		limitPrice, limitPrice - currentPrice);
	
	OrderLimit = limitPrice;
	tr = enterLong(1);  // Returns TRADE pointer
	
	if(tr) {
		printf("Trade created - ID: %d\n", (int)tr->nID);
	} else {
		printf("ERROR: enterLong returned NULL\n");
		quit("Failed to create order");
	}
	
	printf("Initial: NumPendingLong=%d, NumOpenLong=%d\n", NumPendingLong, NumOpenLong);
	
	// Wait for fill (poll every 200ms, max 10 seconds)
	printf("\nWaiting for fill...\n");
	maxAttempts = 50;  // 50 * 200ms = 10 seconds max
	attempts = 0;
	
	while(attempts < maxAttempts) {
		wait(200);  // 200ms between checks
		attempts++;
		
		printf("  Attempt %d/%d: NumOpenLong=%d, NumPendingLong=%d\n", 
			attempts, maxAttempts, NumOpenLong, NumPendingLong);
		
		if(NumOpenLong > 0) {
			printf("\n? ORDER FILLED! Position: %d contracts\n", NumOpenLong);
			break;
		}
	}
	
	if(NumOpenLong == 0) {
		printf("\n? Order did not fill after %d seconds\n", maxAttempts * 200 / 1000);
		printf("Current status: NumOpenLong=%d, NumPendingLong=%d\n", NumOpenLong, NumPendingLong);
		
		// Cancel the pending order before moving to next test
		if(tr) {
			printf("Canceling unfilled order...\n");
			exitTrade(tr);
			wait(500);
		}
	} else {
		// Order filled - now close the position
		printf("\n--- Closing filled position ---\n");
		
		if(tr) {
			exitTrade(tr);
			printf("exitTrade() called for trade ID %d\n", (int)tr->nID);
		}
		
		wait(1000);  // Wait for close to process
		
		printf("\nAfter close:\n");
		printf("  NumPendingLong: %d\n", NumPendingLong);
		printf("  NumOpenLong: %d\n", NumOpenLong);
		
		// Verify position closed
		if(NumOpenLong == 0) {
			printf("\n? Position successfully closed!\n");
		} else {
			printf("\n? Position still open: %d contracts\n", NumOpenLong);
		}
	}
	
	//==========================================================================
	// TEST 2: SHORT LIMIT ORDER - Place below market to ensure fill
	//==========================================================================
	printf("\n========================================\n");
	printf("TEST 2: SHORT LIMIT ORDER (below market)\n");
	printf("========================================\n");
	
	currentPrice = priceClose();
	printf("\nCurrent market price: %.2f\n", currentPrice);
	
	// Place limit SELL order BELOW market (will fill immediately or very quickly)
	limitPrice = currentPrice - 0.75;  // Below market to ensure fill
	printf("Placing limit SELL order at %.2f (%.2f BELOW market)\n",
		limitPrice, currentPrice - limitPrice);
	
	OrderLimit = limitPrice;
	tr = enterShort(1);  // Returns TRADE pointer
	
	if(tr) {
		printf("Trade created - ID: %d\n", (int)tr->nID);
	} else {
		printf("ERROR: enterShort returned NULL\n");
		quit("Failed to create order");
	}
	
	printf("Initial: NumPendingShort=%d, NumOpenShort=%d\n", NumPendingShort, NumOpenShort);
	
	// Wait for fill (poll every 200ms, max 10 seconds)
	printf("\nWaiting for fill...\n");
	maxAttempts = 50;  // 50 * 200ms = 10 seconds max
	attempts = 0;
	
	while(attempts < maxAttempts) {
		wait(200);  // 200ms between checks
		attempts++;
		
		printf("  Attempt %d/%d: NumOpenShort=%d, NumPendingShort=%d\n", 
			attempts, maxAttempts, NumOpenShort, NumPendingShort);
		
		if(NumOpenShort > 0) {
			printf("\n? ORDER FILLED! Position: %d contracts\n", NumOpenShort);
			break;
		}
	}
	
	if(NumOpenShort == 0) {
		printf("\n? Order did not fill after %d seconds\n", maxAttempts * 200 / 1000);
		printf("Current status: NumOpenShort=%d, NumPendingShort=%d\n", NumOpenShort, NumPendingShort);
		
		// Cancel the pending order
		if(tr) {
			printf("Canceling unfilled order...\n");
			exitTrade(tr);
			wait(500);
		}
	} else {
		// Order filled - now close the position
		printf("\n--- Closing filled position ---\n");
		
		if(tr) {
			exitTrade(tr);
			printf("exitTrade() called for trade ID %d\n", (int)tr->nID);
		}
		
		wait(1000);  // Wait for close to process
		
		printf("\nAfter close:\n");
		printf("  NumPendingShort: %d\n", NumPendingShort);
		printf("  NumOpenShort: %d\n", NumOpenShort);
		
		// Verify position closed
		if(NumOpenShort == 0) {
			printf("\n? Position successfully closed!\n");
		} else {
			printf("\n? Position still open: %d contracts\n", NumOpenShort);
		}
	}
	
	//==========================================================================
	// SUMMARY
	//==========================================================================
	printf("\n========================================\n");
	printf("TEST SUMMARY\n");
	printf("========================================\n");
	printf("Test 1 (Long):  %s\n", NumOpenLong == 0 && NumPendingLong == 0 ? "PASS" : "FAIL");
	printf("Test 2 (Short): %s\n", NumOpenShort == 0 && NumPendingShort == 0 ? "PASS" : "FAIL");
	printf("\nFinal positions:\n");
	printf("  Long:  %d open, %d pending\n", NumOpenLong, NumPendingLong);
	printf("  Short: %d open, %d pending\n", NumOpenShort, NumPendingShort);
	printf("========================================\n");
	
	quit("Fill and close test complete");
}
