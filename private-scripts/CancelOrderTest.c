// CancelOrderTest.c - Test order cancellation
// Demonstrates proper way to cancel limit orders

function run()
{
	var currentPrice;
	var limitPrice;
	TRADE* tr;  // Use TRADE pointer, not int

	BarPeriod = 1;
	LookBack = 0;

	if (is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug
		asset("MESH26");
		printf("\n=== Order Cancellation Test ===\n");
	}

	// Get current price
	currentPrice = priceClose();
	printf("\nCurrent market price: %.2f\n", currentPrice);

	// Place limit BUY order below market (won't fill immediately)
	limitPrice = currentPrice - .75;  // 5 points below
	printf("\nPlacing limit BUY order at %.2f (%.2f below market)\n",
		limitPrice, currentPrice - limitPrice);

	OrderLimit = limitPrice;
	tr = enterLong(1);  // Returns TRADE pointer

	if (tr) {
		printf("Trade created - ID: %d\n", (int)tr->nID);
	}
	else {
		printf("ERROR: enterLong returned NULL\n");
		quit("Failed to create order");
	}

	printf("NumPendingLong: %d\n", NumPendingLong);
	printf("NumOpenLong: %d\n", NumOpenLong);

	// Wait a moment
	printf("\nWaiting 2 seconds...\n");
	wait(2000);

	printf("\nChecking order status in NT...\n");
	printf("NumPendingLong: %d\n", NumPendingLong);
	printf("NumOpenLong: %d\n", NumOpenLong);

	// Cancel using exitTrade
	printf("\n--- Calling exitTrade() to cancel ---\n");

	if (tr) {
		exitTrade(tr);
		printf("exitTrade() called for trade ID %d\n", (int)tr->nID);
	}

	wait(1000);  // Wait for cancel to process

	printf("\nAfter cancel:\n");
	printf("  NumPendingLong: %d\n", NumPendingLong);
	printf("  NumOpenLong: %d\n", NumOpenLong);

	// Verify order is gone
	if (NumPendingLong == 0 && NumOpenLong == 0) {
		printf("\n? Order successfully cancelled!\n");
	}
	else {
		printf("\n? Order still active (Pending:%d, Open:%d)\n",
			NumPendingLong, NumOpenLong);
		printf("Check NinjaTrader Orders tab to verify\n");
	}


	//SHORT LIMIT TEST
	// Get current price
	currentPrice = priceClose();
	printf("\nCurrent market price: %.2f\n", currentPrice);

	// Place limit BUY order below market (won't fill immediately)
	limitPrice = currentPrice + .75;  // 5 points below
	printf("\nPlacing limit BUY order at %.2f (%.2f below market)\n",
		limitPrice, currentPrice - limitPrice);

	OrderLimit = limitPrice;
	tr = enterShort(1);  // Returns TRADE pointer

	if (tr) {
		printf("Trade created - ID: %d\n", (int)tr->nID);
	}
	else {
		printf("ERROR: enterLong returned NULL\n");
		quit("Failed to create order");
	}

	printf("NumPendingShort: %d\n", NumPendingShort);
	printf("NumOpenLong: %d\n", NumOpenShort);

	// Wait a moment
	printf("\nWaiting 2 seconds...\n");
	wait(2000);

	printf("\nChecking order status in NT...\n");
	printf("NumPendingShort: %d\n", NumPendingShort);
	printf("NumOpenLong: %d\n", NumOpenShort);

	// Cancel using exitTrade
	printf("\n--- Calling exitTrade() to cancel ---\n");

	if (tr) {
		exitTrade(tr);
		printf("exitTrade() called for trade ID %d\n", (int)tr->nID);
	}

	wait(1000);  // Wait for cancel to process

	printf("\nAfter cancel:\n");
	printf("  NumPendingShort: %d\n", NumPendingShort);
	printf("  NumOpenLong: %d\n", NumOpenShort);

	// Verify order is gone
	if (NumPendingShort == 0 && NumOpenShort == 0) {
		printf("\n? Order successfully cancelled!\n");
	}
	else {
		printf("\n? Order still active (Pending:%d, Open:%d)\n",
			NumPendingShort, NumOpenShort);
		printf("Check NinjaTrader Orders tab to verify\n");
	}
	quit("Cancel test complete - check NT Output for CANCELORDER message");
}
