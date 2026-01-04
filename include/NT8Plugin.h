// NT8Plugin.h - Zorro Broker Plugin for NinjaTrader 8
// Copyright (c) 2025

#pragma once

#ifndef NT8PLUGIN_H
#define NT8PLUGIN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <map>
#include <memory>

#include "trading.h"
#include "TcpBridge.h"  // Changed from NtDirect.h

// DLL export macro
#define DLLFUNC extern "C" __declspec(dllexport)

// Plugin info
#define PLUGIN_NAME    "NT8"

// Version tracking (undef trading.h version first)
#ifdef PLUGIN_VERSION
#undef PLUGIN_VERSION
#endif

#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 0
#define PLUGIN_VERSION_PATCH 0
#define PLUGIN_VERSION ((PLUGIN_VERSION_MAJOR << 16) | (PLUGIN_VERSION_MINOR << 8) | PLUGIN_VERSION_PATCH)

// Version string helper
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define PLUGIN_VERSION_STRING \
    TOSTRING(PLUGIN_VERSION_MAJOR) "." \
    TOSTRING(PLUGIN_VERSION_MINOR) "." \
    TOSTRING(PLUGIN_VERSION_PATCH)

// Global state (std::unique_ptr forward declaration)
extern std::unique_ptr<TcpBridge> g_bridge;
extern int (__cdecl *BrokerMessage)(const char* text);
extern int (__cdecl *BrokerProgress)(const int progress);

// Utility functions
DATE ConvertUnixToDATE(__time64_t unixTime);
__time64_t ConvertDATEToUnix(DATE date);
void LogMessage(const char* format, ...);
void LogError(const char* format, ...);

//=============================================================================
// Order tracking structure
//=============================================================================

struct OrderInfo {
    std::string orderId;
    std::string instrument;
    std::string action;      // "BUY" or "SELL"
    int quantity;
    double limitPrice;
    double stopPrice;
    int filled;
    double avgFillPrice;
    std::string status;
};

//=============================================================================
// Plugin State - consolidates all global configuration and state
//=============================================================================

struct PluginState {
    // Configuration
    int diagLevel = 0;              // Diagnostic level (0=errors, 1=info, 2=debug)
    int orderType = ORDER_GTC;      // Default order time-in-force
    
    // Connection state
    bool connected = false;         // Connected to NinjaTrader
    
    // Account state
    std::string account;            // Current account name
    std::string currentSymbol;      // Last subscribed symbol
    
    // Order tracking
    std::map<int, OrderInfo> orders;            // Track orders by numeric ID
    std::map<std::string, int> orderIdMap;      // Map NT order ID to numeric ID
    int nextOrderNum = 1000;                    // Next numeric order ID to assign
    
    // Reset all state (called on logout)
    void reset() {
        diagLevel = 0;
        orderType = ORDER_GTC;
        connected = false;
        account.clear();
        currentSymbol.clear();
        orders.clear();
        orderIdMap.clear();
        nextOrderNum = 1000;
    }
};

//=============================================================================
// Zorro Broker Plugin Functions
//=============================================================================

// Required functions
DLLFUNC int BrokerOpen(char* Name, FARPROC fpMessage, FARPROC fpProgress);
DLLFUNC int BrokerLogin(char* User, char* Pwd, char* Type, char* Accounts);
DLLFUNC int BrokerAsset(char* Asset, double* pPrice, double* pSpread,
    double* pVolume, double* pPip, double* pPipCost, double* pLotAmount,
    double* pMargin, double* pRollLong, double* pRollShort, double* pCommission);
DLLFUNC int BrokerBuy2(char* Asset, int Amount, double StopDist, double Limit,
    double* pPrice, int* pFill);

// Optional but recommended functions
DLLFUNC int BrokerTime(DATE* pTimeUTC);
DLLFUNC int BrokerAccount(char* Account, double* pBalance, double* pTradeVal,
    double* pMarginVal);
DLLFUNC int BrokerTrade(int nTradeID, double* pOpen, double* pClose,
    double* pCost, double* pProfit);
DLLFUNC int BrokerSell2(int nTradeID, int nAmount, double Limit,
    double* pClose, double* pCost, double* pProfit, int* pFill);
DLLFUNC double BrokerCommand(int Command, DWORD dwParameter);

#endif // NT8PLUGIN_H
