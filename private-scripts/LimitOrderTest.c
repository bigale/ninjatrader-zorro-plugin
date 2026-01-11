// LimitOrderTest.c - Test placing and canceling limit orders
// Demonstrates working with pending limit orders

function run()
{
	// Declare all variables at function start (Lite-C requirement)
	var currentPrice;
	var limitPrice;
	int tradeID;
	
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);  // Info level logging
		asset("MESH26");
		printf("\n=== Limit Order Test ===\n");
		printf("Strategy:\n");
		printf("  1. Get current price\n");
		printf("  2. Place limit buy order below market\n");
		printf("  3. Wait 2 seconds\n");
		printf("  4. Cancel the order\n");
		printf("  5. Place limit sell order above market\n");
		printf("  6. Wait 2 seconds\n");
		printf("  7. Cancel the order\n\n");
	}
	
	// Get current market price
	currentPrice = priceClose();
	printf("Current market price: %.2f\n", currentPrice);
	
	// Test 1: Place limit BUY order below market
	printf("\n--- Test 1: Limit BUY Order ---\n");
	limitPrice = currentPrice - 5.0;  // 5 points below market
	printf("Placing limit BUY order at %.2f (%.2f below market)\n", limitPrice, currentPrice - limitPrice);
	
	Limit = limitPrice;  // Set limit price
	tradeID = enterLong(1);  // Returns trade ID
	
	printf("Trade ID returned: %d\n", tradeID);
	printf("NumPendingTotal: %d\n", NumPendingTotal);
	
	// Wait 2 seconds
	printf("Waiting 2 seconds...\n");
	wait(2000);
	
	// Cancel the order using exitTrade
	printf("Canceling order (Trade ID: %d)...\n", tradeID);
	exitTrade(tradeID);
	
	printf("NumPendingTotal after cancel: %d\n", NumPendingTotal);
	
	// Test 2: Place limit SELL order above market
	printf("\n--- Test 2: Limit SELL Order ---\n");
	limitPrice = currentPrice + 5.0;  // 5 points above market
	printf("Placing limit SELL order at %.2f (%.2f above market)\n", limitPrice, limitPrice - currentPrice);
	
	Limit = limitPrice;  // Set limit price
	tradeID = enterShort(1);  // Returns trade ID
	
	printf("Trade ID returned: %d\n", tradeID);
	printf("NumPendingTotal: %d\n", NumPendingTotal);
	
	// Wait 2 seconds
	printf("Waiting 2 seconds...\n");
	wait(2000);
	
	// Cancel the order
	printf("Canceling order (Trade ID: %d)...\n", tradeID);
	exitTrade(tradeID);
	
	printf("NumPendingTotal after cancel: %d\n", NumPendingTotal);
	
	// Summary
	printf("\n=== Test Complete ===\n");
	printf("Final pending orders: %d\n", NumPendingTotal);
	printf("Final open positions: %d\n", NumOpenTotal);
	
	quit("Limit order test completed successfully");
}
