// LimitOrderDebug.c - Simple limit order test with monitoring
// Shows limit orders being placed and monitors their status

var g_limitPrice = 0;
int g_tradeID = 0;  // Changed from var to int
int g_phase = 0;    // Changed from var to int

function run()
{
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);
		asset("MESH26");
		printf("\n=== Limit Order Debug Test ===\n");
		g_phase = 0;
	}
	
	// Phase 0: Place limit BUY below market
	if(g_phase == 0) {
		var currentPrice = priceClose();
		g_limitPrice = currentPrice - 2.0;  // 2 points below market
		
		printf("\n--- Placing Limit BUY Order ---");
		printf("\nCurrent Price: %.2f", currentPrice);
		printf("\nLimit Price: %.2f (%.2f below market)", g_limitPrice, currentPrice - g_limitPrice);
		
		OrderLimit = g_limitPrice;  // Set limit price BEFORE entering
		g_tradeID = enterLong(1);
		
		printf("\nTrade ID: %d", g_tradeID);
		printf("\nNumOpenLong: %d (filled positions)", NumOpenLong);
		printf("\nNumPendingLong: %d (pending orders)", NumPendingLong);
		
		if(NumPendingLong > 0) {
			printf("\n? ORDER IS PENDING (waiting for price to drop to %.2f)", g_limitPrice);
		}
		if(NumOpenLong > 0) {
			printf("\n? ORDER FILLED IMMEDIATELY (price already at limit?)");
		}
		
		g_phase = 1;
	}
	
	// Phase 1: Monitor for 5 seconds
	else if(g_phase == 1) {
		printf("\n[MONITOR] Price: %.2f | Limit: %.2f | Pending: %d | Filled: %d",
			priceClose(), g_limitPrice, NumPendingLong, NumOpenLong);
		
		wait(5000);  // Wait 5 seconds total
		g_phase = 2;
	}
	
	// Phase 2: Cancel or close
	else if(g_phase == 2) {
		printf("\n--- Cleanup ---");
		
		if(NumPendingLong > 0) {
			printf("\nCanceling pending order...");
			exitLong();  // Cancels pending orders
			printf("\nOrder canceled");
		}
		
		if(NumOpenLong > 0) {
			printf("\nClosing filled position...");
			exitLong();  // Closes filled position
			printf("\nPosition closed");
		}
		
		printf("\n\nFinal Status:");
		printf("\n  Pending: %d", NumPendingLong);
		printf("\n  Filled: %d", NumOpenLong);
		
		quit("Limit order test complete");
	}
}
