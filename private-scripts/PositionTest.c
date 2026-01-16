// PositionTest.c - Debug position tracking issue
// Tests both enterLong() and enterShort() with position queries
// NOTE: GET_POSITION returns ABSOLUTE position size, NOT signed!
// Direction is tracked by Zorro via NumOpenLong/NumOpenShort

function run()
{
	int pos;
	
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug
		asset("MESH26");
		printf("\n=== Position Test ===\n");
		printf("NOTE: GET_POSITION returns absolute size (1), not direction (-1/+1)\n");
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
	printf("\nGET_POSITION returned: %d", pos);
	printf("\nZorro NumOpenLong: %d", NumOpenLong);
	
	// CORRECT: GET_POSITION returns absolute value (1), NOT direction
	if(pos == 1) {
		printf("\n? LONG position correct! (absolute size = 1)");
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
	printf("\nGET_POSITION returned: %d", pos);
	printf("\nZorro NumOpenShort: %d", NumOpenShort);
	
	// CORRECT: GET_POSITION returns absolute value (1), NOT -1
	if(pos == 1) {
		printf("\n? SHORT position correct! (absolute size = 1)");
	} else {
		printf("\n? SHORT position WRONG! Expected 1, got %d", pos);
	}
	
	// Close short
	printf("\nClosing short...");
	exitShort();
	wait(2000);
	
	pos = brokerCommand(GET_POSITION, (long)Asset);
	printf("\nPosition after exitShort(): %d\n", pos);
	
	printf("\n=== Summary ===");
	printf("\nGET_POSITION must return:");
	printf("\n  - Absolute position size (0, 1, 2, 3...)");
	printf("\n  - NEVER negative values");
	printf("\n  - Direction tracked by NumOpenLong/NumOpenShort\n");
	
	quit("Test complete");
}
