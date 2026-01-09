// PositionTest.c - Debug position tracking issue
// Tests both enterLong() and enterShort() with position queries

function run()
{
	int pos;
	
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug
		asset("MESH26");
		printf("\n=== Position Test ===\n");
	}
	
	// Test 1: Check initial position
	printf("\n--- Initial Position ---");
	pos = brokerCommand(GET_POSITION, (long)Asset);
	printf("\nInitial position: %d", pos);
	
	// Test 2: Enter LONG
	printf("\n\n--- Test LONG Position ---");
	printf("\nPlacing BUY order...");
	enterLong(1);
	wait(2000);  // Wait for fill
	
	pos = brokerCommand(GET_POSITION, (long)Asset);
	printf("\nPosition after enterLong(): %d", pos);
	printf("\nZorro NumOpenLong: %d", NumOpenLong);
	
	if(pos == 1) {
		printf("\n? LONG position correct!");
	} else {
		printf("\n? LONG position WRONG! Expected 1, got %d", pos);
	}
	
	// Close long
	printf("\nClosing long...");
	exitLong();
	wait(2000);
	
	pos = brokerCommand(GET_POSITION, (long)Asset);
	printf("\nPosition after exitLong(): %d", pos);
	
	// Test 3: Enter SHORT  
	printf("\n\n--- Test SHORT Position ---");
	printf("\nPlacing SELL order...");
	enterShort(1);
	wait(2000);
	
	pos = brokerCommand(GET_POSITION, (long)Asset);
	printf("\nPosition after enterShort(): %d", pos);
	printf("\nZorro NumOpenShort: %d", NumOpenShort);
	
	if(pos == -1) {
		printf("\n? SHORT position correct!");
	} else {
		printf("\n? SHORT position WRONG! Expected -1, got %d", pos);
	}
	
	// Close short
	printf("\nClosing short...");
	exitShort();
	wait(2000);
	
	pos = brokerCommand(GET_POSITION, (long)Asset);
	printf("\nPosition after exitShort(): %d\n", pos);
	
	quit("Test complete");
}
