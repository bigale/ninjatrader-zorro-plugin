// NT8Plugin.cpp - Zorro Broker Plugin for NinjaTrader 8
// Copyright (c) 2025
//
// A full-featured Zorro broker plugin using TCP bridge architecture
// Supports: Market data, order placement, position tracking, account info

#include "NT8Plugin.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <map>
#include <vector>
#include <sstream>

//=============================================================================
// Global State
//=============================================================================

std::unique_ptr<TcpBridge> g_bridge;  // TCP communication bridge
int (__cdecl *BrokerMessage)(const char* text) = nullptr;
int (__cdecl *BrokerProgress)(const int progress) = nullptr;

static PluginState g_state;  // All plugin state consolidated here

//=============================================================================
// Utility Functions
//=============================================================================

DATE ConvertUnixToDATE(__time64_t unixTime)
{
    // DATE is days since Dec 30, 1899
    // Unix epoch is Jan 1, 1970 = 25569 days after Dec 30, 1899
    return (double)unixTime / (24.0 * 60.0 * 60.0) + 25569.0;
}

__time64_t ConvertDATEToUnix(DATE date)
{
    return (__time64_t)((date - 25569.0) * 24.0 * 60.0 * 60.0);
}

void LogMessage(const char* format, ...)
{
    if (!BrokerMessage) return;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    BrokerMessage(buffer);
}

void LogError(const char* format, ...)
{
    if (!BrokerMessage) return;
    
    char buffer[1024];
    buffer[0] = '!';  // Alert prefix for errors
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + 1, sizeof(buffer) - 1, format, args);
    va_end(args);
    
    BrokerMessage(buffer);
}

// Conditional logging based on diagnostic level
void LogInfo(const char* format, ...)
{
    if (!BrokerMessage || g_state.diagLevel < 1) return;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    BrokerMessage(buffer);
}

void LogDebug(const char* format, ...)
{
    if (!BrokerMessage || g_state.diagLevel < 2) return;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    BrokerMessage(buffer);
}

// Keep Zorro responsive during waits - allows user cancellation
static int responsiveSleep(int ms)
{
    Sleep(ms);
    if (BrokerProgress) {
        return BrokerProgress(0);  // Returns 0 if user wants to abort
    }
    return 1;  // Continue
}

// Poll for position update after order fill
// Retries up to maxAttempts times with delayMs between attempts
// Returns actual position or 0 if timeout/error
static int pollForPosition(const char* symbol, const char* account, int expectedChange, int maxAttempts, int delayMs)
{
    int previousPos = g_bridge->MarketPosition(symbol, account);
    
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        if (!responsiveSleep(delayMs)) {
            LogInfo("# Position poll cancelled by user");
            break;
        }
        
        int currentPos = g_bridge->MarketPosition(symbol, account);
        
        // Check if position changed in expected direction
        if (expectedChange > 0 && currentPos > previousPos) {
            LogInfo("# Position updated: %d (after %d ms)", currentPos, (attempt + 1) * delayMs);
            return currentPos;
        } else if (expectedChange < 0 && currentPos < previousPos) {
            LogInfo("# Position updated: %d (after %d ms)", currentPos, (attempt + 1) * delayMs);
            return currentPos;
        } else if (expectedChange == 0 && currentPos != previousPos) {
            // Any change detected
            LogInfo("# Position updated: %d (after %d ms)", currentPos, (attempt + 1) * delayMs);
            return currentPos;
        }
        
        LogDebug("# Poll attempt %d/%d: position still %d", attempt + 1, maxAttempts, currentPos);
    }
    
    // Timeout - return last known position
    int finalPos = g_bridge->MarketPosition(symbol, account);
    LogInfo("# Position poll timeout after %d ms, returning: %d", maxAttempts * delayMs, finalPos);
    return finalPos;
}

// Calculate stop price from current market price and stop distance
static double CalculateStopPrice(int amount, double currentPrice, double stopDist)
{
    if (amount > 0) {
        // Buy stop: trigger above current market price
        return currentPrice + stopDist;
    } else {
        // Sell stop: trigger below current market price
        return currentPrice - stopDist;
    }
}

// Get time in force string from order type
static const char* GetTimeInForce(int orderType)
{
    switch (orderType) {
        case ORDER_GTC: return "GTC";
        case ORDER_IOC: return "IOC";
        case ORDER_FOK: return "FOK";
        default:        return "DAY";
    }
}

// Generate unique numeric order ID and store mapping
static int RegisterOrder(const char* ntOrderId, const OrderInfo& info)
{
    int numId = g_state.nextOrderNum++;
    g_state.orders[numId] = info;
    g_state.orders[numId].orderId = ntOrderId;
    g_state.orderIdMap[ntOrderId] = numId;
    return numId;
}

// Look up order by numeric ID
static OrderInfo* GetOrder(int numId)
{
    auto it = g_state.orders.find(numId);
    if (it != g_state.orders.end()) {
        return &it->second;
    }
    return nullptr;
}

//=============================================================================
// BrokerOpen - Initialize plugin
//=============================================================================

DLLFUNC int BrokerOpen(char* Name, FARPROC fpMessage, FARPROC fpProgress)
{
    // Store callback functions
    (FARPROC&)BrokerMessage = fpMessage;
    (FARPROC&)BrokerProgress = fpProgress;
    
    // Set plugin name
    if (Name) {
        strcpy_s(Name, 32, PLUGIN_NAME);
    }
    
    // Create TCP Bridge wrapper
    if (!g_bridge) {
        g_bridge = std::make_unique<TcpBridge>();
    }
    
    // Force output to console for debugging
    printf("[NT8] Plugin loading v%s\n", PLUGIN_VERSION_STRING);
    fflush(stdout);
    
    LogMessage("# NT8 plugin v%s (TCP Bridge for NT8 8.1+)", PLUGIN_VERSION_STRING);
    
    return PLUGIN_VERSION;
}

//=============================================================================
// BrokerLogin - Connect to NinjaTrader
//=============================================================================

DLLFUNC int BrokerLogin(char* User, char* Pwd, char* Type, char* Accounts)
{
    // ALWAYS log to file for debugging
    FILE* debugLog = fopen("C:\\Zorro_2.66\\NT8_debug.log", "a");
    if (debugLog) {
        fprintf(debugLog, "[BrokerLogin] Called with User='%s'\n", User ? User : "NULL");
        fflush(debugLog);
        fclose(debugLog);
    }
    
    // Logout request
    if (!User || !*User) {
        if (g_bridge) {
            g_bridge->TearDown();
        }
        g_state.connected = false;
        g_state.account.clear();
        LogMessage("# NT8 disconnected");
        return 0;
    }
    
    // Connect to NinjaTrader via TCP
    if (!g_bridge->IsConnected()) {
        if (!g_bridge->Connect()) {
            LogError("Failed to connect to NinjaTrader AddOn on localhost:8888");
            LogError("Make sure:");
            LogError("  1. NinjaTrader 8 is running");
            LogError("  2. ZorroBridge AddOn is installed and compiled");
            LogError("  3. AddOn is enabled (check Output window in NT8)");
            return 0;
        }
    }
    
    // Send login command
    std::string loginCmd = std::string("LOGIN:") + User;
    std::string response = g_bridge->SendCommand(loginCmd);
    
    if (response.find("ERROR") != std::string::npos) {
        LogError("Login failed: %s", response.c_str());
        LogError("Check account name is correct in NinjaTrader");
        return 0;
    }
    
    // Store account name
    g_state.account = User;
    g_state.connected = true;
    
    // Returned account name in Accounts parameter
    if (Accounts) {
        strcpy_s(Accounts, 1024, User);
    }
    
    LogMessage("# NT8 connected to account: %s (via TCP)", g_state.account.c_str());
    
    if (debugLog) {
        debugLog = fopen("C:\\Zorro_2.66\\NT8_debug.log", "a");
        fprintf(debugLog, "[BrokerLogin] Connected successfully to: %s\n", User);
        fflush(debugLog);
        fclose(debugLog);
    }
    
    return 1;
}

//=============================================================================
// BrokerTime - Keep connection alive, get server time
//=============================================================================

DLLFUNC int BrokerTime(DATE* pTimeUTC)
{
    if (!g_bridge || !g_state.connected) {
        return 0;
    }
    
    // Progress callback to keep UI responsive
    if (BrokerProgress) {
        BrokerProgress(0);
    }
    
    // Check still connected
    if (g_bridge->Connected(0) != 0) {
        g_state.connected = false;
        return 0;
    }
    
    // NinjaTrader doesn't expose server time via ATI
    // Return current local time in UTC
    if (pTimeUTC) {
        time_t now = time(nullptr);
        *pTimeUTC = ConvertUnixToDATE(now);
    }
    
    // Return 2 = connected and market likely open
    // Could add real market hours check here
    return 2;
}

//=============================================================================
// BrokerAsset - Subscribe to market data, get prices
//=============================================================================

DLLFUNC int BrokerAsset(char* Asset, double* pPrice, double* pSpread,
    double* pVolume, double* pPip, double* pPipCost, double* pLotAmount,
    double* pMargin, double* pRollLong, double* pRollShort, double* pCommission)
{
    if (!g_bridge || !g_state.connected || !Asset) {
        return 0;
    }
    
    // Subscribe mode (pPrice == NULL) - just subscribe to data
    if (!pPrice) {
        int result = g_bridge->SubscribeMarketData(Asset);
        if (result == 0) {
            g_state.currentSymbol = Asset;
            LogMessage("# Subscribed to %s", Asset);
            return 1;
        }
        LogError("Failed to subscribe to %s", Asset);
        return 0;
    }
    
    // Make sure we're subscribed
    if (g_state.currentSymbol != Asset) {
        g_bridge->SubscribeMarketData(Asset);
        g_state.currentSymbol = Asset;
        Sleep(100);  // Brief delay for data to arrive
    }
    
    // Get market data
    double bid = g_bridge->GetBid(Asset);
    double ask = g_bridge->GetAsk(Asset);
    double last = g_bridge->GetLast(Asset);
    double volume = g_bridge->GetVolume(Asset);
    
    // Return price (use ask for consistency)
    *pPrice = ask > 0 ? ask : last;
    
    // Spread
    if (pSpread && bid > 0 && ask > 0) {
        *pSpread = ask - bid;
    }
    
    // Volume
    if (pVolume) {
        *pVolume = volume;
    }
    
    // Contract specs - these need to be set based on instrument
    // NinjaTrader ATI doesn't expose contract specs, so we use defaults
    // User should configure these in Zorro's asset file
    
    if (pPip) *pPip = 0;           // Let Zorro use asset file
    if (pPipCost) *pPipCost = 0;   // Let Zorro use asset file
    if (pLotAmount) *pLotAmount = 1;
    if (pMargin) *pMargin = 0;
    
    // We got valid price data
    return (*pPrice > 0) ? 1 : 0;
}

//=============================================================================
// BrokerAccount - Get account information
//=============================================================================

DLLFUNC int BrokerAccount(char* Account, double* pBalance, double* pTradeVal,
    double* pMarginVal)
{
    if (!g_bridge || !g_state.connected) {
        return 0;
    }
    
    // Switch account if specified
    const char* acct = (Account && *Account) ? Account : g_state.account.c_str();
    
    // Get account values
    double cashValue = g_bridge->CashValue(acct);
    double buyingPower = g_bridge->BuyingPower(acct);
    double realizedPnL = g_bridge->RealizedPnL(acct);
    
    if (pBalance) {
        *pBalance = cashValue;
    }
    
    if (pTradeVal) {
        // TradeVal is typically unrealized P&L
        // NinjaTrader ATI doesn't directly expose this
        // Could calculate from positions if needed
        *pTradeVal = realizedPnL;
    }
    
    if (pMarginVal) {
        // Available margin approximated from buying power
        *pMarginVal = buyingPower;
    }
    
    return 1;
}

//=============================================================================
// BrokerBuy2 - Place orders
//=============================================================================

DLLFUNC int BrokerBuy2(char* Asset, int Amount, double StopDist, double Limit,
    double* pPrice, int* pFill)
{
    LogDebug("# [BrokerBuy2] Called with Asset=%s, Amount=%d, StopDist=%.2f, Limit=%.2f", 
        Asset ? Asset : "NULL", Amount, StopDist, Limit);
    
    if (!g_bridge || !g_state.connected || !Asset || Amount == 0) {
        LogError("[BrokerBuy2] Pre-check failed: connected=%d, Asset=%s, Amount=%d",
            g_state.connected, Asset ? Asset : "NULL", Amount);
        return 0;
    }
    
    // Determine order direction
    const char* action = (Amount > 0) ? "BUY" : "SELL";
    int quantity = abs(Amount);
    
    // Determine order type and prices
    const char* orderType = "MARKET";
    double limitPrice = 0.0;
    double stopPrice = 0.0;
    
    // Entry stop orders: Stop distance indicates trigger price for entry
    // BUY STOP: Enter long when price rises to stop (stop is ABOVE market)
    // SELL STOP: Enter short when price falls to stop (stop is BELOW market)
    if (StopDist > 0) {
        // Get current market price for stop calculation
        double currentPrice = g_bridge->GetLast(Asset);
        if (currentPrice <= 0) {
            currentPrice = g_bridge->GetAsk(Asset);  // Fallback to ask
        }
        
        if (currentPrice > 0) {
            // For entry stops, calculate trigger price
            // BUY: stop above current price (currentPrice + StopDist)
            // SELL: stop below current price (currentPrice - StopDist)
            stopPrice = CalculateStopPrice(Amount, currentPrice, StopDist);
            
            if (Limit > 0) {
                // Stop-Limit order
                orderType = "STOPLIMIT";
                limitPrice = Limit;
            } else {
                // Stop-Market order
                orderType = "STOP";
            }
            
            LogDebug("# [BrokerBuy2] Entry stop order: %s STOP @ %.2f (current: %.2f, dist: %.2f)",
                action, stopPrice, currentPrice, StopDist);
        } else {
            LogError("Cannot calculate stop price - no market data");
            return 0;
        }
    } else if (Limit > 0) {
        // Limit order (no stop)
        orderType = "LIMIT";
        limitPrice = Limit;
        LogInfo("# [BrokerBuy2] Limit order: %s @ %.2f (current market: %.2f)", 
            action, limitPrice, g_bridge->GetLast(Asset));
    }
    // else: Market order (defaults set above)
    
    LogDebug("# [BrokerBuy2] Order params: %s %d %s @ %s (limit=%.2f, stop=%.2f)",
        action, quantity, Asset, orderType, limitPrice, stopPrice);
    
    LogInfo("# [BrokerBuy2] Placing order: %s %d %s @ %s (stopPrice=%.2f)",
        action, quantity, Asset, orderType, stopPrice);
    
    // Get a new order ID from NinjaTrader
    const char* ntOrderId = g_bridge->NewOrderId();
    if (!ntOrderId || !*ntOrderId) {
        LogError("Failed to get order ID from NinjaTrader");
        return 0;
    }
    
    // Copy order ID (NT returns pointer to static buffer)
    std::string orderId = ntOrderId;
    LogDebug("# [BrokerBuy2] Generated order ID: %s", orderId.c_str());
    
    // Get time in force
    const char* tif = GetTimeInForce(g_state.orderType);
    LogDebug("# [BrokerBuy2] Time in force: %s", tif);
    
    // Place the order
    LogDebug("# [BrokerBuy2] Calling Command(PLACE)...");
    int result = g_bridge->Command(
        "PLACE",
        g_state.account.c_str(),
        Asset,
        action,
        quantity,
        orderType,
        limitPrice,
        stopPrice,
        tif,
        "",           // OCO
        orderId.c_str(),
        "",           // Strategy ID
        ""            // Strategy Name
    );
    
    LogDebug("# [BrokerBuy2] Command returned: %d", result);
    
    if (result != 0) {
        LogError("Order placement failed: %s %d %s @ %s (result=%d)",
            action, quantity, Asset, orderType, result);
        return 0;
    }
    
    // Get the NT order ID from the response
    const char* ntActualOrderId = g_bridge->GetLastNtOrderId();
    if (!ntActualOrderId || !*ntActualOrderId) {
        LogError("Failed to get NT order ID from response");
        return 0;
    }
    
    LogInfo("# [BrokerBuy2] Order placed successfully! NT ID: %s", ntActualOrderId);
    
    // Create order tracking info using the REAL NT order ID
    OrderInfo info;
    info.orderId = ntActualOrderId;  // Use the NT GUID, not our ZORRO_xxxx
    info.instrument = Asset;
    info.action = action;
    info.quantity = quantity;
    info.limitPrice = limitPrice;
    info.stopPrice = stopPrice;
    info.filled = 0;
    info.avgFillPrice = 0;
    info.status = "Submitted";
    
    // Register order and get numeric ID
    int numericId = RegisterOrder(ntActualOrderId, info);
    
    LogInfo("# Order %d (%s): %s %d %s @ %s",
        numericId, ntActualOrderId, action, quantity, Asset,
        orderType);
    
    // For market orders, wait briefly for fill
    if (strcmp(orderType, "MARKET") == 0) {
        LogDebug("# [BrokerBuy2] Waiting for market order fill...");
        for (int i = 0; i < 10; i++) {
            if (!responsiveSleep(100)) {
                LogInfo("# [BrokerBuy2] User cancelled wait for fill");
                break;  // User wants to abort
            }
            
            int filled = g_bridge->Filled(ntActualOrderId);  // Use NT order ID
            if (filled > 0) {
                double fillPrice = g_bridge->AvgFillPrice(ntActualOrderId);
                
                // Update order info
                OrderInfo* orderInfo = GetOrder(numericId);
                if (orderInfo) {
                    orderInfo->filled = filled;
                    orderInfo->avgFillPrice = fillPrice;
                    orderInfo->status = "Filled";
                }
                
                if (pPrice) *pPrice = fillPrice;
                if (pFill) *pFill = filled;
                
                LogInfo("# Order %d filled: %d @ %.2f", numericId, filled, fillPrice);
                
                // Poll for position update (NT needs time to update Positions collection)
                // Try up to 10 times with 100ms delay = 1 second max wait
                LogDebug("# Polling for position update...");
                int expectedChange = (Amount > 0) ? 1 : -1;  // +1 for buy, -1 for sell
                pollForPosition(Asset, g_state.account.c_str(), expectedChange, 10, 100);
                
                break;
            }
        }
    } else {
        // Stop and limit orders: don't wait for fill
        LogDebug("# [BrokerBuy2] %s order placed, will fill when triggered", orderType);
    }
    
    LogDebug("# [BrokerBuy2] Returning order ID: %d", numericId);
    return numericId;
}

//=============================================================================
// BrokerTrade - Get trade/order status
//=============================================================================

DLLFUNC int BrokerTrade(int nTradeID, double* pOpen, double* pClose,
    double* pCost, double* pProfit)
{
    if (!g_bridge || !g_state.connected) {
        return NAY;
    }
    
    OrderInfo* order = GetOrder(nTradeID);
    if (!order) {
        return NAY;
    }
    
    // Get current order status from NinjaTrader
    const char* status = g_bridge->OrderStatus(order->orderId.c_str());
    order->status = status ? status : "";
    
    // Check for cancelled/rejected
    if (order->status == "Cancelled" || order->status == "Rejected") {
        return NAY;
    }
    
    // Get fill information
    int filled = g_bridge->Filled(order->orderId.c_str());
    double avgFill = g_bridge->AvgFillPrice(order->orderId.c_str());
    
    order->filled = filled;
    if (avgFill > 0) {
        order->avgFillPrice = avgFill;
    }
    
    // Return entry price
    if (pOpen && order->avgFillPrice > 0) {
        *pOpen = order->avgFillPrice;
    }
    
    // Current price for P&L calculation
    if (pClose && !order->instrument.empty()) {
        double currentPrice = g_bridge->GetLast(order->instrument.c_str());
        if (currentPrice > 0) {
            *pClose = currentPrice;
        }
    }
    
    // Calculate profit (simplified - doesn't account for tick value)
    if (pProfit && pOpen && pClose && *pOpen > 0 && *pClose > 0) {
        double direction = (order->action == "BUY") ? 1.0 : -1.0;
        *pProfit = (*pClose - *pOpen) * order->filled * direction;
    }
    
    return order->filled;
}

//=============================================================================
// BrokerSell2 - Close positions / exit trades
//=============================================================================

DLLFUNC int BrokerSell2(int nTradeID, int nAmount, double Limit,
    double* pClose, double* pCost, double* pProfit, int* pFill)
{
    if (!g_bridge || !g_state.connected) {
        return 0;
    }
    
    OrderInfo* order = GetOrder(nTradeID);
    if (!order) {
        LogError("Order %d not found", nTradeID);
        return 0;
    }
    
    // ALWAYS update filled quantity from NinjaTrader (don't trust cached value)
    if (!order->orderId.empty()) {
        int currentFilled = g_bridge->Filled(order->orderId.c_str());
        if (currentFilled > 0) {
            order->filled = currentFilled;
            
            // Also update average fill price
            double avgFill = g_bridge->AvgFillPrice(order->orderId.c_str());
            if (avgFill > 0) {
                order->avgFillPrice = avgFill;
            }
        }
    }
    
    // Determine close action (opposite of original)
    const char* action = (order->action == "BUY") ? "SELL" : "BUY";
    
    // Determine quantity to close
    int quantity = 0;
    if (nAmount > 0) {
        // Specific amount requested
        quantity = nAmount;
    } else {
        // Close all - use filled quantity
        quantity = order->filled;
        
        // If filled is still 0, check current position from NinjaTrader
        if (quantity <= 0 && !order->instrument.empty()) {
            int position = g_bridge->MarketPosition(order->instrument.c_str(), g_state.account.c_str());
            quantity = abs(position);
            
            if (quantity > 0) {
                LogMessage("# Using current position %d for close order %d", quantity, nTradeID);
            }
        }
    }
    
    if (quantity <= 0) {
        LogError("Invalid close quantity for order %d (filled=%d, nAmount=%d)", 
            nTradeID, order->filled, nAmount);
        return 0;
    }
    
    // Determine order type
    const char* orderType = "MARKET";
    double limitPrice = 0.0;
    
    if (Limit > 0) {
        orderType = "LIMIT";
        limitPrice = Limit;
    }
    
    // Get new order ID for the closing order
    const char* ntOrderId = g_bridge->NewOrderId();
    std::string closeOrderId = ntOrderId ? ntOrderId : "";
    
    LogMessage("# Closing order %d: %s %d %s @ %s", 
        nTradeID, action, quantity, order->instrument.c_str(), orderType);
    
    // Place closing order
    int result = g_bridge->Command(
        "PLACE",
        g_state.account.c_str(),
        order->instrument.c_str(),
        action,
        quantity,
        orderType,
        limitPrice,
        0.0,
        GetTimeInForce(g_state.orderType),
        "",
        closeOrderId.c_str(),
        "",
        ""
    );
    
    if (result != 0) {
        LogError("Close order failed for trade %d", nTradeID);
        return 0;
    }
    
    // Get the actual NT order ID from the response
    const char* ntCloseOrderId = g_bridge->GetLastNtOrderId();
    if (!ntCloseOrderId || !*ntCloseOrderId) {
        LogError("Failed to get NT close order ID");
        return 0;
    }
    
    LogInfo("# Close order placed: NT ID %s", ntCloseOrderId);
    
    // Wait for fill (market orders)
    if (strcmp(orderType, "MARKET") == 0) {
        for (int i = 0; i < 10; i++) {
            if (!responsiveSleep(100)) {
                LogInfo("# [BrokerSell2] User cancelled wait for fill");
                break;  // User wants to abort
            }
            
            int filled = g_bridge->Filled(ntCloseOrderId);  // Use NT order ID
            if (filled > 0) {
                double fillPrice = g_bridge->AvgFillPrice(ntCloseOrderId);
                
                if (pClose) *pClose = fillPrice;
                if (pFill) *pFill = filled;
                
                // Calculate profit
                if (pProfit && order->avgFillPrice > 0) {
                    double direction = (order->action == "BUY") ? 1.0 : -1.0;
                    *pProfit = (fillPrice - order->avgFillPrice) * filled * direction;
                }
                
                LogMessage("# Trade %d closed: %d @ %.2f", nTradeID, filled, fillPrice);
                
                // Poll for position update to confirm close
                LogDebug("# Polling for position update after close...");
                int expectedChange = (action == "BUY") ? 1 : -1;  // Inverse of original position
                pollForPosition(order->instrument.c_str(), g_state.account.c_str(), expectedChange, 10, 100);
                
                break;
            }
        }
    }
    
    return nTradeID;
}

//=============================================================================
// BrokerCommand - Extended broker commands
//=============================================================================

DLLFUNC double BrokerCommand(int Command, DWORD dwParameter)
{
    if (!g_bridge) return 0;
    
    switch (Command) {
        case GET_COMPLIANCE:
            return NFA_COMPLIANT;
            
        case GET_BROKERZONE:
            return -5;  // EST timezone
            
        case GET_MAXTICKS:
            return 0;   // No historical data via ATI
            
        case GET_POSITION: {
            if (!dwParameter || !g_state.connected) return 0;
            const char* symbol = (const char*)dwParameter;
            
            LogInfo("# GET_POSITION query for: %s", symbol);
            
            // Try multiple times with longer delays - NinjaTrader Positions collection
            // updates asynchronously and can take 500ms+ to reflect order fills
            int position = 0;
            int maxAttempts = 10;  // Increased to 10 attempts
            int delayMs = 250;      // Increased to 250ms per attempt (2.5 seconds total)
            
            for (int attempt = 0; attempt < maxAttempts; attempt++) {
                position = g_bridge->MarketPosition(symbol, g_state.account.c_str());
                
                LogDebug("# Position query attempt %d/%d: %d", attempt + 1, maxAttempts, position);
                
                // Don't break early - always poll the full duration to ensure
                // we get the latest position after recent order fills
                if (attempt < maxAttempts - 1) {
                    if (!responsiveSleep(delayMs)) {
                        LogInfo("# Position poll cancelled by user");
                        break;
                    }
                }
            }
            
            LogInfo("# Position returned: %d (after %d ms)", position, maxAttempts * delayMs);
            
            return (double)position;
        }
        
        case GET_AVGENTRY: {
            if (!dwParameter || !g_state.connected) return 0;
            const char* symbol = (const char*)dwParameter;
            
            LogInfo("# GET_AVGENTRY query for: %s", symbol);
            
            double avgEntry = g_bridge->AvgEntryPrice(symbol, g_state.account.c_str());
            
            LogInfo("# Avg entry returned: %.2f", avgEntry);
            
            return avgEntry;
        }
        
        case SET_ORDERTYPE:
            g_state.orderType = (int)dwParameter;
            return 1;
            
        case SET_SYMBOL:
            if (dwParameter) {
                g_state.currentSymbol = (const char*)dwParameter;
            }
            return 1;
            
        case DO_CANCEL: {
            // Cancel specific order
            int orderId = (int)dwParameter;
            OrderInfo* order = GetOrder(orderId);
            if (order) {
                int result = g_bridge->CancelOrder(order->orderId.c_str());
                return (result == 0) ? 1 : 0;
            }
            return 0;
        }
        
        case SET_DIAGNOSTICS:
            {
                FILE* debugLog = fopen("C:\\Zorro_2.66\\NT8_debug.log", "a");
                if (debugLog) {
                    fprintf(debugLog, "[SET_DIAGNOSTICS] Called with level=%d\n", (int)dwParameter);
                    fflush(debugLog);
                    fclose(debugLog);
                }
            }
            g_state.diagLevel = (int)dwParameter;
            LogMessage("# Diagnostic level set to %d (0=errors, 1=info, 2=debug)", g_state.diagLevel);
            return 1;
        
        case GET_DIAGNOSTICS:
            return g_state.diagLevel;
        
        case GET_MAXREQUESTS:
            // TCP to localhost is very fast, allow many requests
            return 20.0;  // 20 requests per second
        
        case GET_WAIT:
            return 50;  // 50ms polling interval
            
        default:
            return 0;
    }
}

//=============================================================================
// DLL Entry Point
//=============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            break;
            
        case DLL_PROCESS_DETACH:
            // Cleanup
            if (g_bridge) {
                // Unsubscribe from market data
                if (!g_state.currentSymbol.empty()) {
                    g_bridge->UnSubscribeMarketData(g_state.currentSymbol.c_str());
                }
                g_bridge.reset();
            }
            g_state.orders.clear();
            g_state.orderIdMap.clear();
            break;
    }
    return TRUE;
}
