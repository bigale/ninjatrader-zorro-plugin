// NT8Plugin.cpp - Zorro Broker Plugin for NinjaTrader 8
// Copyright (c) 2025
//
// A full-featured Zorro broker plugin using NinjaTrader's ATI (NtDirect.dll)
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

TcpBridge* g_bridge = nullptr;  // Changed from NtDirect
int (__cdecl *BrokerMessage)(const char* text) = nullptr;
int (__cdecl *BrokerProgress)(const int progress) = nullptr;

static std::string g_account;                    // Current account name
static std::string g_currentSymbol;              // Current asset symbol
static int g_orderType = ORDER_GTC;              // Default order TIF
static std::map<int, OrderInfo> g_orders;        // Track orders by numeric ID
static std::map<std::string, int> g_orderIdMap;  // Map NT orderId to numeric ID
static int g_nextOrderNum = 1000;                // Next numeric order ID
static bool g_connected = false;

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
    int numId = g_nextOrderNum++;
    g_orders[numId] = info;
    g_orders[numId].orderId = ntOrderId;
    g_orderIdMap[ntOrderId] = numId;
    return numId;
}

// Look up order by numeric ID
static OrderInfo* GetOrder(int numId)
{
    auto it = g_orders.find(numId);
    if (it != g_orders.end()) {
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
        g_bridge = new TcpBridge();  // Changed from NtDirect
    }
    
    LogMessage("# NT8 plugin initialized (TCP Bridge for NT8 8.1+)");
    
    return PLUGIN_VERSION;
}

//=============================================================================
// BrokerLogin - Connect to NinjaTrader
//=============================================================================

DLLFUNC int BrokerLogin(char* User, char* Pwd, char* Type, char* Accounts)
{
    // Logout request
    if (!User || !*User) {
        if (g_bridge) {
            g_bridge->TearDown();
        }
        g_connected = false;
        g_account.clear();
        LogMessage("# NT8 disconnected");
        return 0;
    }
    
    // Connect to NinjaTrader via TCP
    if (!g_bridge->IsConnected()) {
        if (!g_bridge->Connect()) {
            LogError("Failed to connect to NinjaTrader AddOn on localhost:8888");
            LogError("Make sure:");
            LogError("  1. NinjaTrader 8 is running");
            LogError("  2. ZorroATI AddOn is installed and compiled");
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
    g_account = User;
    g_connected = true;
    
    // Return account name in Accounts parameter
    if (Accounts) {
        strcpy_s(Accounts, 1024, User);
    }
    
    LogMessage("# NT8 connected to account: %s (via TCP)", g_account.c_str());
    
    return 1;
}

//=============================================================================
// BrokerTime - Keep connection alive, get server time
//=============================================================================

DLLFUNC int BrokerTime(DATE* pTimeUTC)
{
    if (!g_bridge || !g_connected) {
        return 0;
    }
    
    // Progress callback to keep UI responsive
    if (BrokerProgress) {
        BrokerProgress(0);
    }
    
    // Check still connected
    if (g_bridge->Connected(0) != 0) {
        g_connected = false;
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
    if (!g_bridge || !g_connected || !Asset) {
        return 0;
    }
    
    // Subscribe mode (pPrice == NULL) - just subscribe to data
    if (!pPrice) {
        int result = g_bridge->SubscribeMarketData(Asset);
        if (result == 0) {
            g_currentSymbol = Asset;
            LogMessage("# Subscribed to %s", Asset);
            return 1;
        }
        LogError("Failed to subscribe to %s", Asset);
        return 0;
    }
    
    // Make sure we're subscribed
    if (g_currentSymbol != Asset) {
        g_bridge->SubscribeMarketData(Asset);
        g_currentSymbol = Asset;
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
    if (!g_bridge || !g_connected) {
        return 0;
    }
    
    // Switch account if specified
    const char* acct = (Account && *Account) ? Account : g_account.c_str();
    
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
    if (!g_bridge || !g_connected || !Asset || Amount == 0) {
        return 0;
    }
    
    // Determine order direction
    const char* action = (Amount > 0) ? "BUY" : "SELL";
    int quantity = abs(Amount);
    
    // Determine order type
    const char* orderType = "MARKET";
    double limitPrice = 0.0;
    double stopPrice = 0.0;
    
    if (Limit > 0) {
        orderType = "LIMIT";
        limitPrice = Limit;
    }
    
    // Get a new order ID from NinjaTrader
    const char* ntOrderId = g_bridge->NewOrderId();
    if (!ntOrderId || !*ntOrderId) {
        LogError("Failed to get order ID from NinjaTrader");
        return 0;
    }
    
    // Copy order ID (NT returns pointer to static buffer)
    std::string orderId = ntOrderId;
    
    // Get time in force
    const char* tif = GetTimeInForce(g_orderType);
    
    // Place the order
    int result = g_bridge->Command(
        "PLACE",
        g_account.c_str(),
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
    
    if (result != 0) {
        LogError("Order placement failed: %s %d %s @ %s",
            action, quantity, Asset, orderType);
        return 0;
    }
    
    // Create order tracking info
    OrderInfo info;
    info.orderId = orderId;
    info.instrument = Asset;
    info.action = action;
    info.quantity = quantity;
    info.limitPrice = limitPrice;
    info.stopPrice = stopPrice;
    info.filled = 0;
    info.avgFillPrice = 0;
    info.status = "Submitted";
    
    // Register order and get numeric ID
    int numericId = RegisterOrder(orderId.c_str(), info);
    
    LogMessage("# Order %d (%s): %s %d %s @ %s",
        numericId, orderId.c_str(), action, quantity, Asset,
        orderType);
    
    // For market orders, wait briefly for fill
    if (strcmp(orderType, "MARKET") == 0) {
        for (int i = 0; i < 10; i++) {
            Sleep(100);
            
            int filled = g_bridge->Filled(orderId.c_str());
            if (filled > 0) {
                double fillPrice = g_bridge->AvgFillPrice(orderId.c_str());
                
                // Update order info
                OrderInfo* orderInfo = GetOrder(numericId);
                if (orderInfo) {
                    orderInfo->filled = filled;
                    orderInfo->avgFillPrice = fillPrice;
                    orderInfo->status = "Filled";
                }
                
                if (pPrice) *pPrice = fillPrice;
                if (pFill) *pFill = filled;
                
                LogMessage("# Order %d filled: %d @ %.2f", numericId, filled, fillPrice);
                break;
            }
        }
    }
    
    return numericId;
}

//=============================================================================
// BrokerTrade - Get trade/order status
//=============================================================================

DLLFUNC int BrokerTrade(int nTradeID, double* pOpen, double* pClose,
    double* pCost, double* pProfit)
{
    if (!g_bridge || !g_connected) {
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
    if (!g_bridge || !g_connected) {
        return 0;
    }
    
    OrderInfo* order = GetOrder(nTradeID);
    if (!order) {
        LogError("Order %d not found", nTradeID);
        return 0;
    }
    
    // Determine close action (opposite of original)
    const char* action = (order->action == "BUY") ? "SELL" : "BUY";
    int quantity = (nAmount > 0) ? nAmount : order->filled;
    
    if (quantity <= 0) {
        LogError("Invalid close quantity for order %d", nTradeID);
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
    
    // Place closing order
    int result = g_bridge->Command(
        "PLACE",
        g_account.c_str(),
        order->instrument.c_str(),
        action,
        quantity,
        orderType,
        limitPrice,
        0.0,
        GetTimeInForce(g_orderType),
        "",
        closeOrderId.c_str(),
        "",
        ""
    );
    
    if (result != 0) {
        LogError("Close order failed for trade %d", nTradeID);
        return 0;
    }
    
    // Wait for fill (market orders)
    if (strcmp(orderType, "MARKET") == 0) {
        for (int i = 0; i < 10; i++) {
            Sleep(100);
            
            int filled = g_bridge->Filled(closeOrderId.c_str());
            if (filled > 0) {
                double fillPrice = g_bridge->AvgFillPrice(closeOrderId.c_str());
                
                if (pClose) *pClose = fillPrice;
                if (pFill) *pFill = filled;
                
                // Calculate profit
                if (pProfit && order->avgFillPrice > 0) {
                    double direction = (order->action == "BUY") ? 1.0 : -1.0;
                    *pProfit = (fillPrice - order->avgFillPrice) * filled * direction;
                }
                
                LogMessage("# Trade %d closed: %d @ %.2f", nTradeID, filled, fillPrice);
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
            if (!dwParameter || !g_connected) return 0;
            const char* symbol = (const char*)dwParameter;
            return (double)g_bridge->MarketPosition(symbol, g_account.c_str());
        }
        
        case GET_AVGENTRY: {
            if (!dwParameter || !g_connected) return 0;
            const char* symbol = (const char*)dwParameter;
            return g_bridge->AvgEntryPrice(symbol, g_account.c_str());
        }
        
        case SET_ORDERTYPE:
            g_orderType = (int)dwParameter;
            return 1;
            
        case SET_SYMBOL:
            if (dwParameter) {
                g_currentSymbol = (const char*)dwParameter;
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
                if (!g_currentSymbol.empty()) {
                    g_bridge->UnSubscribeMarketData(g_currentSymbol.c_str());
                }
                delete g_bridge;
                g_bridge = nullptr;
            }
            g_orders.clear();
            g_orderIdMap.clear();
            break;
    }
    return TRUE;
}
