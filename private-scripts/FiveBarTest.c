// FiveBarTest.c - Enter 5 long positions, close all, then 5 short positions
// Uses tick() to work without waiting for bar generation

var positionsEntered = 0;  // Track how many positions we've entered
var phase = 1;             // Current phase (1=long entry, 2=long exit, 3=short entry, 4=short exit, 5=done)

function run()
{
	BarPeriod = 1;  // 1 minute bars
	LookBack = 0;   // Live-only trading
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);  // Info level logging
		asset("MESH26");
		printf("\n=== Five Position Test ===\n");
		printf("Strategy:\n");
		printf("  1. Enter long 5 times\n");
		printf("  2. Close all long positions\n");
		printf("  3. Enter short 5 times\n");
		printf("  4. Close all short positions\n");
		printf("  5. Stop\n\n");
		
		positionsEntered = 0;
		phase = 1;
	}
	
	// Phase 1: Enter 5 long positions
	if(phase == 1) {
		if(positionsEntered < 5) {
			printf("[LONG] Entering position #%d\n", (int)positionsEntered + 1);
			enterLong(1);
			positionsEntered++;
			printf("Total long positions: %d\n", NumOpenLong);
		} else {
			printf("\n[PHASE COMPLETE] Entered 5 long positions\n");
			printf("Moving to phase 2 (close longs)\n\n");
			phase = 2;
		}
	}
	
	// Phase 2: Close all long positions
	else if(phase == 2) {
		printf("[CLOSE LONG] Closing all %d long positions\n", NumOpenLong);
		
		while(NumOpenLong > 0) {
			exitLong();
		}
		
		printf("Positions after close: %d\n\n", NumOpenTotal);
		printf("[PHASE COMPLETE] All longs closed\n");
		printf("Moving to phase 3 (enter shorts)\n\n");
		positionsEntered = 0;
		phase = 3;
	}
	
	// Phase 3: Enter 5 short positions
	else if(phase == 3) {
		if(positionsEntered < 5) {
			printf("[SHORT] Entering position #%d\n", (int)positionsEntered + 1);
			enterShort(1);
			positionsEntered++;
			printf("Total short positions: %d\n", NumOpenShort);
		} else {
			printf("\n[PHASE COMPLETE] Entered 5 short positions\n");
			printf("Moving to phase 4 (close shorts)\n\n");
			phase = 4;
		}
	}
	
	// Phase 4: Close all short positions
	else if(phase == 4) {
		printf("[CLOSE SHORT] Closing all %d short positions\n", NumOpenShort);
		
		while(NumOpenShort > 0) {
			exitShort();
		}
		
		printf("Positions after close: %d\n\n", NumOpenTotal);
		printf("[PHASE COMPLETE] All shorts closed\n\n");
		phase = 5;
	}
	
	// Phase 5: Done
	else if(phase == 5) {
		printf("=== TEST COMPLETE ===\n");
		printf("Total phases executed: 5\n");
		printf("Final positions: %d\n", NumOpenTotal);
		quit("Five position test completed successfully");
	}
}
