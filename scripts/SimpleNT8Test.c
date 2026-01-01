// SimpleNT8Test.c - Live-only test for NT8 plugin
// No historical data required

function run()
{
	BarPeriod = 1;
	LookBack = 0;  // No historical data needed!
	
	if(is(INITRUN)) {
		brokerCommand(SET_DIAGNOSTICS, 1);
		printf("\n=== NT8 Live Test ===\n");
		printf("No historical data - live only\n");
	}
	
	asset("MESH26");
	
	printf("\nAsset: %s\n", Asset);
	printf("Price: %.2f\n", priceClose());
	printf("Spread: %.2f\n", Spread);
	
	if(NumBars > 10) {
		printf("\n=== Success! ===\n");
		printf("Generated %d bars\n", NumBars);
		quit("Live data working!");
	}
}
