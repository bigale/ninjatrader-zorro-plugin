// SimpleAutoTrade.c - Comprehensive automated trading test
// Tests all key broker plugin functionality

////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////
#define BARPERIOD 1./60  // 1 second bars
#define VERBOSE 2

////////////////////////////////////////////////////////////
// Test State
////////////////////////////////////////////////////////////
var g_LastTradeID = 0;
int g_TestPhase = 0;
int g_TotalTrades = 0;
var g_TotalProfit = 0;

function run()
{
	// Basic settings
	BarPeriod = BARPERIOD;
	LookBack = 0;
	
	asset("MES 0326");
	Lots = 1;
	
	if(is(INITRUN)) {
		printf("\n========================================");
		printf("\n  Comprehensive Plugin Test Suite");
		printf("\n========================================");
		printf("\nAsset: %s", Asset);
		printf("\nBar Period: %.0f seconds", BarPeriod * 60);
		printf("\n");
		printf("\nTests:");
		printf("\n  1. LONG market order entry/exit");
		printf("\n  2. SHORT market order entry/exit");
		printf("\n  3. Account data (Balance, Equity)");
		printf("\n  4. Position tracking");
		printf("\n  5. P&L calculation");
		printf("\n  6. Market data (Price, Spread)");
		printf("\n========================================\n");
	}
	
	// Test account data every 10 bars
	if(Bar % 10 == 0 && !is(LOOKBACK)) {
		printf("\n[ACCOUNT] Balance: $%.2f | Equity: $%.2f | Margin: $%.2f",
			Balance, Equity, MarginVal);
	}
	
	// Test market data
	if(Bar % 5 == 0 && !is(LOOKBACK)) {
		printf("\n[MARKET] Price: %.2f | Spread: %.2f | Vol: %.0f",
			priceClose(), Spread, marketVol());
	}
	
	// 40-bar trading cycle
	int cyclePos = Bar % 40;
	
	if(NumOpenLong == 0 && NumOpenShort == 0) {
		// No position open
		if(cyclePos < 10) {
			// Test LONG entry
			printf("\n[TEST 1] Opening LONG market order (Bar %d)...", Bar);
			
			int tradeID = enterLong();
			
			if(tradeID > 0) {
				g_LastTradeID = tradeID;
				g_TotalTrades++;
				printf(" SUCCESS! ID: %d", tradeID);
			} else {
				printf(" FAILED! enterLong returned 0");
			}
		} else if(cyclePos >= 10 && cyclePos < 20) {
			// Wait after LONG
			if(Bar % 40 == 10) {
				printf("\n[WAIT] Waiting 10 bars after LONG test");
			}
		} else if(cyclePos >= 20 && cyclePos < 30) {
			// Test SHORT entry
			printf("\n[TEST 2] Opening SHORT market order (Bar %d)...", Bar);
			
			int tradeID = enterShort();
			
			if(tradeID > 0) {
				g_LastTradeID = tradeID;
				g_TotalTrades++;
				printf(" SUCCESS! ID: %d", tradeID);
			} else {
				printf(" FAILED! enterShort returned 0");
			}
		} else {
			// Wait after SHORT
			if(Bar % 40 == 30) {
				printf("\n[WAIT] Waiting 10 bars after SHORT test");
			}
		}
	} else if(NumOpenLong > 0) {
		// Test LONG position tracking
		if(cyclePos % 2 == 0) {  // Every other bar to reduce spam
			printf("\n[TEST 4] LONG position - Entry: %.2f | Current: %.2f | P&L: $%.2f",
				TradePriceOpen, priceClose(), TradeProfit);
		}
		
		if(cyclePos >= 10) {
			// Test LONG exit
			printf("\n[TEST 1] Closing LONG position (Bar %d)", Bar);
			
			var profitBeforeExit = TradeProfit;
			exitLong();
			
			printf(" | Final P&L: $%.2f", profitBeforeExit);
			g_TotalProfit += profitBeforeExit;
			
			printf("\n[SUMMARY] Total Trades: %d | Cumulative P&L: $%.2f",
				g_TotalTrades, g_TotalProfit);
		}
	} else if(NumOpenShort > 0) {
		// Test SHORT position tracking
		if(cyclePos % 2 == 0) {
			printf("\n[TEST 4] SHORT position - Entry: %.2f | Current: %.2f | P&L: $%.2f",
				TradePriceOpen, priceClose(), TradeProfit);
		}
		
		if(cyclePos >= 30 || cyclePos < 20) {
			// Test SHORT exit
			printf("\n[TEST 2] Closing SHORT position (Bar %d)", Bar);
			
			var profitBeforeExit = TradeProfit;
			exitShort();
			
			printf(" | Final P&L: $%.2f", profitBeforeExit);
			g_TotalProfit += profitBeforeExit;
			
			printf("\n[SUMMARY] Total Trades: %d | Cumulative P&L: $%.2f",
				g_TotalTrades, g_TotalProfit);
		}
	}
	
	// Overall test summary every 40 bars
	if(Bar > 0 && Bar % 40 == 0 && !is(LOOKBACK)) {
		printf("\n");
		printf("\n========================================");
		printf("\n  Cycle %d Complete", Bar / 40);
		printf("\n========================================");
		printf("\n[TEST 3] Account Balance: $%.2f", Balance);
		printf("\n[TEST 3] Account Equity: $%.2f", Equity);
		printf("\n[TEST 5] Total P&L: $%.2f", g_TotalProfit);
		printf("\n[TEST 6] Current Price: %.2f", priceClose());
		printf("\n         Total Trades: %d", g_TotalTrades);
		printf("\n========================================\n");
	}
}
