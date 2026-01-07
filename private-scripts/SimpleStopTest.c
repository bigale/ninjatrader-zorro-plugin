// SimpleStopTest.c - Basic Stop Order Testing
// This tests if we can manually trigger stop orders via the broker API

#define BARPERIOD 5./60
#define VERBOSE 2

var g_BuyStopPrice = 0;
var g_SellStopPrice = 0;
int g_TestPhase = 0;

function run()
{
	BarPeriod = BARPERIOD;
	LookBack = 0;
	asset("MES 0326");
	Lots = 1;
	
	if(is(INITRUN)) {
		printf("\n===== Simple Stop Test =====");
		printf("\nWe will test if Stop parameter reaches BrokerBuy2");
		printf("\n================================\n");
	}
	
	switch(g_TestPhase) {
		case 0:
			if(priceClose() > 0) {
				printf("\n[Test 1] Placing order WITH Stop set");
				printf("\n  Current price: %.2f", priceClose());
				printf("\n  PIP: %.2f", PIP);
				
				// Try setting Stop before enterLong
				Stop = 10 * PIP;  // Should be 2.50 if PIP=0.25
				printf("\n  Stop variable set to: %.2f (10 * %.2f)", Stop, PIP);
				printf("\n  Calling enterLong()...");
				
				int id = enterLong(1);
				printf("\n  enterLong returned: %d", id);
				printf("\n  Check plugin log for StopDist value!");
				
				Stop = 0;
				g_TestPhase = 1;
			}
			break;
			
		case 1:
			printf("\n[Test 1] Waiting 5 bars...");
			if(Bar > 5) {
				printf("\n[Test 1] Closing position");
				exitLong();
				g_TestPhase = 2;
			}
			break;
			
		case 2:
			printf("\n[Test 2] Placing order WITHOUT Stop");
			printf("\n  Stop = 0");
			printf("\n  Calling enterShort()...");
			
			Stop = 0;
			int id2 = enterShort(1);
			printf("\n  enterShort returned: %d", id2);
			
			g_TestPhase = 3;
			break;
			
		case 3:
			if(Bar > 10) {
				printf("\n[Test 2] Closing position");
				exitShort();
				quit("Test complete - check Zorro console and NT output!");
			}
			break;
	}
}
