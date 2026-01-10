// PositionTest2.c - Test position tracking using Zorro's built-in variables
// This is the CORRECT way to test positions - use NumOpenLong/NumOpenShort

function run()
{
	BarPeriod = 1;
	LookBack = 0;
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 2);  // Full debug
		asset("MESH26");
		printf("\n=== Zorro Position Tracking Test ===\n");
		printf("Testing with Zorro's built-in position variables\n");
	}
	
	// Test 1: Enter LONG
	printf("\n--- Test LONG Position ---");
	printf("\nCalling enterLong(1)...");
	
	enterLong(1);
	
	printf("\nZorro NumOpenLong: %d", NumOpenLong);
	printf("\nZorro NumOpenShort: %d", NumOpenShort);
	printf("\nZorro NumOpenTotal: %d", NumOpenTotal);
	
	if(NumOpenLong == 1) {
		printf("\n? LONG position CORRECT (NumOpenLong = 1)");
	} else {
		printf("\n? LONG position WRONG! Expected NumOpenLong=1, got %d", NumOpenLong);
	}
	
	// Close the long
	printf("\n\nClosing long position...");
	exitLong();
	
	printf("\nAfter exitLong:");
	printf("\n  NumOpenLong: %d", NumOpenLong);
	printf("\n  NumOpenTotal: %d", NumOpenTotal);
	
	// Test 2: Enter SHORT
	printf("\n\n--- Test SHORT Position ---");
	printf("\nCalling enterShort(1)...");
	
	enterShort(1);
	
	printf("\nZorro NumOpenLong: %d", NumOpenLong);
	printf("\nZorro NumOpenShort: %d", NumOpenShort);
	printf("\nZorro NumOpenTotal: %d", NumOpenTotal);
	
	if(NumOpenShort == 1) {
		printf("\n? SHORT position CORRECT (NumOpenShort = 1)");
	} else {
		printf("\n? SHORT position WRONG! Expected NumOpenShort=1, got %d", NumOpenShort);
	}
	
	// Close the short
	printf("\n\nClosing short position...");
	exitShort();
	
	printf("\nAfter exitShort:");
	printf("\n  NumOpenShort: %d", NumOpenShort);
	printf("\n  NumOpenTotal: %d", NumOpenTotal);
	
	printf("\n\n=== Summary ===");
	printf("\nZorro's position tracking works if:");
	printf("\n  - NumOpenLong = 1 after enterLong()");
	printf("\n  - NumOpenShort = 1 after enterShort()");
	printf("\n  - Both return to 0 after exits");
	printf("\n\nNote: Zorro handles GET_POSITION internally.");
	printf("\nYou should NOT call brokerCommand(GET_POSITION) in scripts!\n");
	
	quit("Test complete - Check if NumOpen values were correct");
}
