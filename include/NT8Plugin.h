// NT8Plugin.h - Zorro Broker Plugin for NinjaTrader 8
// Copyright (c) 2025

#pragma once

#ifndef NT8PLUGIN_H
#define NT8PLUGIN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <map>

#include "trading.h"
#include "TcpBridge.h"  // Changed from NtDirect.h

// DLL export macro
#define DLLFUNC extern "C" __declspec(dllexport)

// Plugin info
#define PLUGIN_NAME    "NT8"

// Global state
extern TcpBridge* g_bridge;  // Changed from NtDirect
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
