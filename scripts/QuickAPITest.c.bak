// QuickAPITest.c - Simple API test that actually works
// No complex state machine - just execute and report

function run()
{
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);
		asset("MESH26");
		printf("\n=== Quick NT8 API Test ===\n");
	}
	
	// Wait for data

		printf("\n--- Market Data ---");
		printf("\nPrice: %.2f | Spread: %.2f", priceClose(), Spread);
		printf("\nBalance: $%.2f\n", Balance);
		
		// Test 1: Buy
		printf("\n--- Test 1: Market Order BUY ---");
		int id = enterLong();
		printf("\nOrder ID: %d", id);
		wait(2000); // Wait for fill
		
		// Test 2: Check position
		printf("\n\n--- Test 2: Position Query ---");
		int pos = brokerCommand(GET_POSITION, (long)Asset);
		wait(2000); // Wait for fill
		
		printf("\nPosition: %d contracts", pos);
		printf("\nZorro reports: %d long", NumOpenLong);
		
		// Test 3: Close
		printf("\n\n--- Test 3: Close Position ---");
		exitLong();
		int pos = brokerCommand(GET_POSITION, (long)Asset);
		wait(2000); // Wait for fill
		
		printf("\nPosition: %d contracts", pos);
		printf("\nZorro reports: %d long", NumOpenLong);		
		wait(2000); // Wait for fill
		
		printf("\n\n--- Test 4: SHORT order ---");
		enterShort();
		int pos = brokerCommand(GET_POSITION, (long)Asset);
		wait(2000); // Wait for fill
		
		printf("\nPosition: %d contracts", pos);
		printf("\nZorro reports: %d long", NumOpenLong);		
		wait(2000); // Wait for fill
		printf("\nShort position: %d", NumOpenShort);
		exitShort();
		int pos = brokerCommand(GET_POSITION, (long)Asset);
		printf("\nPosition: %d contracts", pos);
		printf("\nZorro reports: %d long", NumOpenLong);		
		printf("\n\n=== Tests Complete ===\n");
		quit("Done");
	
}
