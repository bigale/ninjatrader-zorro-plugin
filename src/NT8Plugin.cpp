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
#include <algorithm>  // For std::sort
#include <sstream>
#include <iomanip>

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

// Clean up old completed orders to prevent memory leaks
// Keeps last maxOrderHistory orders for debugging
static void CleanupOldOrders()
{
    // Count orders in terminal states (Filled, Cancelled, Rejected)
    std::vector<int> completedOrders;
    
    for (const auto& pair : g_state.orders) {
        const OrderInfo& order = pair.second;
        if (order.status == "Filled" || 
            order.status == "Cancelled" || 
            order.status == "Rejected") {
            completedOrders.push_back(pair.first);
        }
    }
    
    // If we have more than maxOrderHistory completed orders, remove oldest
    if (completedOrders.size() > (size_t)g_state.maxOrderHistory) {
        // Sort by order ID (older orders have lower IDs)
        std::sort(completedOrders.begin(), completedOrders.end());
        
        // Calculate how many to remove
        size_t toRemove = completedOrders.size() - g_state.maxOrderHistory;
        
        for (size_t i = 0; i < toRemove; i++) {
            int orderId = completedOrders[i];
            OrderInfo* order = GetOrder(orderId);
            
            if (order) {
                // Remove from both maps
                g_state.orderIdMap.erase(order->orderId);
                g_state.orders.erase(orderId);
                g_state.orderCleanupCount++;
            }
        }
        
        LogInfo("# Cleaned up %zu old orders (total cleaned: %d)", 
            toRemove, g_state.orderCleanupCount);
    }
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
        // Send SUBSCRIBE command and parse response
        std::string cmd = std::string("SUBSCRIBE:") + Asset;
        std::string response = g_bridge->SendCommand(cmd);
        
        if (response.find("OK") != std::string::npos) {
            g_state.currentSymbol = Asset;
            
            // **NEW: Parse contract specs from SUBSCRIBE response**
            // Format: OK:Subscribed:{instrument}:{tickSize}:{pointValue}
            auto parts = g_bridge->SplitResponse(response, ':');
            
            if (parts.size() >= 5 && parts[0] == "OK") {
                try {
                    double tickSize = std::stod(parts[3]);
                    double pointValue = std::stod(parts[4]);
                    
                    // Store contract specifications
                    g_state.assetSpecs[Asset].tickSize = tickSize;
                    g_state.assetSpecs[Asset].pointValue = pointValue;
                    
                    LogInfo("# Asset specs for %s: tick=%.4f value=%.2f", Asset, tickSize, pointValue);
                }
                catch (...) {
                    LogInfo("# Could not parse asset specs for %s, using defaults", Asset);
                }
            }
            
            LogMessage("# Subscribed to %s", Asset);
            return 1;
        }
        
        LogError("Failed to subscribe to %s", Asset);
        return 0;
    }
    
    // Make sure we're subscribed
    if (g_state.currentSymbol != Asset) {
        // Need to subscribe first - call ourselves recursively
        BrokerAsset(Asset, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
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
    
    // **FIXED: Return actual contract specs from NT8**
    if (pPip) {
        auto it = g_state.assetSpecs.find(Asset);
        if (it != g_state.assetSpecs.end() && it->second.tickSize > 0) {
            *pPip = it->second.tickSize;
            LogDebug("# Returning tick size for %s: %.4f", Asset, it->second.tickSize);
        } else {
            // **ZORRO 2.70 FIX: Must return non-zero default**
            // Default tick size for micro futures (MES, MNQ, etc.)
            // Zorro will use Assets.csv if available, otherwise this default
            *pPip = 0.25;
            LogInfo("# Using default tick size %.4f for %s (AddOn specs not available)", *pPip, Asset);
        }
    }
    
    if (pPipCost) {
        auto it = g_state.assetSpecs.find(Asset);
        if (it != g_state.assetSpecs.end() && it->second.pointValue > 0) {
            *pPipCost = it->second.pointValue;
            LogDebug("# Returning point value for %s: %.2f", Asset, it->second.pointValue);
        } else {
            // **ZORRO 2.70 FIX: Must return non-zero default**
            // Default point value for MES ($1.25 per 0.25 tick)
            // Zorro will use Assets.csv if available, otherwise this default
            *pPipCost = 1.25;
            LogInfo("# Using default point value $%.2f for %s (AddOn specs not available)", *pPipCost, Asset);
        }
    }
    
    // **CRITICAL: LotAmount must be non-zero for Zorro 2.70**
    if (pLotAmount) *pLotAmount = 1.0;
    
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
    
    // Get account values (now includes unrealized P&L as 4th field)
    double cashValue = g_bridge->CashValue(acct);
    double buyingPower = g_bridge->BuyingPower(acct);
    double realizedPnL = g_bridge->RealizedPnL(acct);
    double unrealizedPnL = g_bridge->UnrealizedPnL(acct);  // NEW: Get unrealized P&L
    
    if (pBalance) {
        *pBalance = cashValue;
    }
    
    if (pTradeVal) {
        // **FIXED: Return UNREALIZED P&L (from open positions)**
        // This is what Zorro manual specifies - current P&L from open trades
        *pTradeVal = unrealizedPnL;
        
        LogDebug("# Account P&L: Unrealized=%.2f, Realized=%.2f", unrealizedPnL, realizedPnL);
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
                
                // **CRITICAL: Update position cache IMMEDIATELY on fill**
                // This ensures GET_POSITION returns correct value instantly
                int signedQty = (Amount > 0) ? filled : -filled;  // Positive for long, negative for sell
                g_state.positions[Asset] += signedQty;
                
                LogInfo("# Order %d filled: %d @ %.2f (cached position now: %d)", 
                    numericId, filled, fillPrice, g_state.positions[Asset]);
                
                if (pPrice) *pPrice = fillPrice;
                if (pFill) *pFill = filled;
                
                // Poll for position update (NT needs time to update Positions collection)
                // Try up to 10 times with 100ms delay = 1 second max wait
                LogDebug("# Polling for position update...");
                int expectedChange = (Amount > 0) ? 1 : -1;  // +1 for buy, -1 for sell
                pollForPosition(Asset, g_state.account.c_str(), expectedChange, 10, 100);
                
                // Market order filled - return positive ID
                LogDebug("# [BrokerBuy2] Returning filled order ID: %d", numericId);
                return numericId;  // Positive = filled
            }
        }
        
        // Market order placed but not filled yet - shouldn't happen normally
        LogInfo("# Market order %d not filled after 1 second", numericId);
        return -numericId;  // Negative = pending
    } else {
        // Stop and limit orders: NOT filled immediately - return NEGATIVE ID
        LogInfo("# [BrokerBuy2] %s order placed, pending fill", orderType);
        LogDebug("# [BrokerBuy2] Returning pending order ID: -%d", numericId);
        return -numericId;  // Negative ID = pending order
    }
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
    
    // Handle negative IDs (pending orders)
    int orderId = abs(nTradeID);
    
    OrderInfo* order = GetOrder(orderId);
    if (!order) {
        return NAY;
    }
    
    // Get current order status from NinjaTrader
    const char* status = g_bridge->OrderStatus(order->orderId.c_str());
    order->status = status ? status : "";
    
    // Check for cancelled/rejected
    if (order->status == "Cancelled" || order->status == "Rejected") {
        // Trigger cleanup when orders reach terminal states
        CleanupOldOrders();
        return NAY;
    }
    
    // Get fill information
    int filled = g_bridge->Filled(order->orderId.c_str());
    double avgFill = g_bridge->AvgFillPrice(order->orderId.c_str());
    
    order->filled = filled;
    if (avgFill > 0) {
        order->avgFillPrice = avgFill;
    }
    
    // If order is fully filled, mark as complete and trigger cleanup
    if (filled > 0 && filled >= order->quantity) {
        order->status = "Filled";
        CleanupOldOrders();
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
    
    // Handle negative IDs (pending orders)
    int orderId = abs(nTradeID);
    
    OrderInfo* order = GetOrder(orderId);
    if (!order) {
        LogError("Order %d not found", orderId);
        return 0;
    }

    // ALWAYS update filled quantity from NinjaTrader (don't trust cached value)
    if (!order->orderId.empty()) {
        int currentFilled = g_bridge->Filled(order->orderId.c_str());
        
        if (currentFilled > 0) {
            // Order has filled
            order->filled = currentFilled;
            
            // Also update average fill price
            double avgFill = g_bridge->AvgFillPrice(order->orderId.c_str());
            if (avgFill > 0) {
                order->avgFillPrice = avgFill;
            }
        } else {
            // Order is still pending (not filled) - CANCEL IT instead of closing
            LogInfo("# Order %d is still pending (filled=0), canceling instead of closing", orderId);
            
            int cancelResult = g_bridge->CancelOrder(order->orderId.c_str());
            if (cancelResult == 0) {
                LogInfo("# Order %d cancelled successfully", orderId);
                return nTradeID;  // Success
            } else {
                LogError("# Failed to cancel order %d", orderId);
                return 0;  // Failed
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
                
                // **CRITICAL: Update position cache on close fill**
                int signedQty = (action == "BUY") ? filled : -filled;
                g_state.positions[order->instrument] += signedQty;
                
                LogMessage("# Trade %d closed: %d @ %.2f (cached position now: %d)", 
                    nTradeID, filled, fillPrice, g_state.positions[order->instrument]);
                
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
// BrokerHistory2 - Download historical price data
//=============================================================================

DLLFUNC int BrokerHistory2(char* Asset, DATE tStart, DATE tEnd,
    int nTickMinutes, int nTicks, T6* ticks)
{
    char msg[256];
    
    // ALWAYS log to file for debugging
    FILE* histLog = fopen("C:\\Zorro_2.66\\BrokerHistory2_debug.log", "a");
    if (histLog) {
        fprintf(histLog, "\n==== BrokerHistory2 CALL ====\n");
        fprintf(histLog, "Asset: %s\n", Asset ? Asset : "NULL");
        fprintf(histLog, "tStart: %.8f\n", tStart);
        fprintf(histLog, "tEnd: %.8f\n", tEnd);
        fprintf(histLog, "nTickMinutes: %d\n", nTickMinutes);
        fprintf(histLog, "nTicks (buffer): %d\n", nTicks);
        fflush(histLog);
    }
    
    sprintf_s(msg, sizeof(msg), "# [HIST] Called: Asset=%s, buf=%d", Asset ? Asset : "NULL", nTicks);
    LogMessage(msg);
    
    if (!g_bridge || !g_state.connected || !Asset || !ticks || nTicks <= 0) {
        LogError("[HIST] Invalid parameters");
        if (histLog) {
            fprintf(histLog, "ERROR: Invalid parameters\n");
            fclose(histLog);
        }
        return 0;
    }
    
    // Build command
    std::ostringstream cmd;
    cmd << "GETHISTORY:" << Asset << ":" 
        << std::fixed << std::setprecision(8) << tStart << ":"
        << tEnd << ":" << nTickMinutes << ":" << nTicks;
    
    if (histLog) {
        fprintf(histLog, "Sending: %s\n", cmd.str().c_str());
        fflush(histLog);
    }
    
    std::string response = g_bridge->SendCommand(cmd.str());
    
    sprintf_s(msg, sizeof(msg), "# [HIST] Response: %zu bytes", response.length());
    LogMessage(msg);
    
    if (histLog) {
        fprintf(histLog, "Response size: %zu bytes\n", response.length());
        fflush(histLog);
    }
    
    // Parse: HISTORY:{numBars}|time,o,h,l,c,v|...
    auto parts = g_bridge->SplitResponse(response, '|');
    
    sprintf_s(msg, sizeof(msg), "# [HIST] Split: %zu parts", parts.size());
    LogMessage(msg);
    
    if (histLog) {
        fprintf(histLog, "Split into: %zu parts\n", parts.size());
        fflush(histLog);
    }
    
    if (parts.size() < 1 || parts[0].find("HISTORY:") != 0) {
        LogError("[HIST] Bad response");
        if (histLog) {
            fprintf(histLog, "ERROR: Bad response format\n");
            if (parts.size() > 0) {
                fprintf(histLog, "First part: %s\n", parts[0].c_str());
            }
            fclose(histLog);
        }
        return 0;
    }
    
    // Extract count
    size_t colonPos = parts[0].find(':');
    if (colonPos == std::string::npos) {
        LogError("[HIST] Malformed");
        if (histLog) {
            fprintf(histLog, "ERROR: Malformed header\n");
            fclose(histLog);
        }
        return 0;
    }
    
    int barCount = std::stoi(parts[0].substr(colonPos + 1));
    
    sprintf_s(msg, sizeof(msg), "# [HIST] NT8=%d bars, buf=%d", barCount, nTicks);
    LogMessage(msg);
    
    if (histLog) {
        fprintf(histLog, "NT8 says: %d bars available\n", barCount);
        fprintf(histLog, "Buffer size: %d\n", nTicks);
    }
    
    if (barCount == 0) {
        if (histLog) {
            fprintf(histLog, "No bars available\n");
            fclose(histLog);
        }
        return 0;
    }
    
    // Parse bars - SKIP bars before tStart!
    int loaded = 0;
    int skipped = 0;
    
    for (size_t i = 1; i < parts.size() && loaded < nTicks; i++) {
        if (parts[i].empty()) continue;
        
        auto fields = g_bridge->SplitResponse(parts[i], ',');
        if (fields.size() < 6) continue;
        
        try {
            double barTime = std::stod(fields[0]);
            
            // CRITICAL: Skip bars before requested start time
            if (barTime < tStart) {
                skipped++;
                continue;
            }
            
            // Also skip bars after requested end time
            if (barTime > tEnd) {
                break;  // All remaining bars will be after tEnd
            }
            
            ticks[loaded].time = barTime;
            ticks[loaded].fOpen = (float)std::stod(fields[1]);
            ticks[loaded].fHigh = (float)std::stod(fields[2]);
            ticks[loaded].fLow = (float)std::stod(fields[3]);
            ticks[loaded].fClose = (float)std::stod(fields[4]);
            ticks[loaded].fVol = (float)std::stod(fields[5]);
            
            // Log first and last bar times
            if (histLog && (loaded == 0 || loaded == 299)) {
                fprintf(histLog, "Bar[%d] time=%.8f, close=%.2f\n", 
                    loaded, ticks[loaded].time, ticks[loaded].fClose);
            }
            
            loaded++;
        }
        catch (...) {
            continue;
        }
    }
    
    if (histLog) {
        fprintf(histLog, "Skipped %d bars before tStart (%.8f)\n", skipped, tStart);
        fprintf(histLog, "Successfully loaded: %d bars\n", loaded);
        fprintf(histLog, "Returning: %d to Zorro\n", loaded);
        fprintf(histLog, "==== BrokerHistory2 END ====\n\n");
        fclose(histLog);
    }
    
    return loaded;
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
            
            // **CRITICAL: Return cached position immediately**
            // Never return transient 0 - always return last known value
            int cachedPosition = g_state.positions[symbol];
            int absolutePosition = abs(cachedPosition);
            
            LogInfo("# GET_POSITION query for: %s (cached: %d signed, returning: %d absolute)", 
                symbol, cachedPosition, absolutePosition);
            
            // Return ABSOLUTE VALUE (Zorro handles direction via NumOpenLong/NumOpenShort)
            return (double)absolutePosition;
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
            // Cancel specific order - handle negative IDs from pending orders
            int orderId = abs((int)dwParameter);
            OrderInfo* order = GetOrder(orderId);
            if (order) {
                LogInfo("# Canceling order %d (NT ID: %s)", orderId, order->orderId.c_str());
                int result = g_bridge->CancelOrder(order->orderId.c_str());
                return (result == 0) ? 1 : 0;
            }
            LogError("# Order %d not found for cancellation", orderId);
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
