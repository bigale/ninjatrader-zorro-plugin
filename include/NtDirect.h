// NtDirect.h - NinjaTrader ATI DLL Interface
// Function declarations for NtDirect.dll
// Reference: https://ninjatrader.com/support/helpGuides/nt8/functions.htm

#pragma once

#ifndef NTDIRECT_H
#define NTDIRECT_H

#include <windows.h>

//=============================================================================
// Function pointer types for NtDirect.dll
// All string parameters are LPCSTR (const char*)
// All string returns are LPCSTR (static buffer, copy immediately)
//=============================================================================

// Connection
typedef int     (__stdcall *pfnConnected)(int showMessage);
typedef int     (__stdcall *pfnTearDown)();

// Market Data
typedef int     (__stdcall *pfnSubscribeMarketData)(LPCSTR instrument);
typedef int     (__stdcall *pfnUnSubscribeMarketData)(LPCSTR instrument);
typedef double  (__stdcall *pfnMarketData)(LPCSTR instrument, int dataType);
// dataType: 0=Last, 1=Bid, 2=Ask, 3=Volume (daily), 4=LastSize, 5=BidSize, 6=AskSize

// Account Information
typedef double  (__stdcall *pfnCashValue)(LPCSTR account);
typedef double  (__stdcall *pfnBuyingPower)(LPCSTR account);
typedef double  (__stdcall *pfnRealizedPnL)(LPCSTR account);

// Position Information
typedef int     (__stdcall *pfnMarketPosition)(LPCSTR instrument, LPCSTR account);
// Returns: 0=Flat, positive=Long, negative=Short

typedef double  (__stdcall *pfnAvgEntryPrice)(LPCSTR instrument, LPCSTR account);
typedef double  (__stdcall *pfnAvgEntryPriceByOrderId)(LPCSTR orderId);

// Order Management
typedef LPCSTR  (__stdcall *pfnNewOrderId)();
typedef int     (__stdcall *pfnCommand)(
    LPCSTR command,         // "PLACE", "CANCEL", "CHANGE", "CLOSEPOSITION", etc.
    LPCSTR account,
    LPCSTR instrument,
    LPCSTR action,          // "BUY", "SELL"
    int quantity,
    LPCSTR orderType,       // "MARKET", "LIMIT", "STOP", "STOPLIMIT"
    double limitPrice,
    double stopPrice,
    LPCSTR timeInForce,     // "DAY", "GTC", "IOC", etc.
    LPCSTR oco,             // OCO group ID (optional)
    LPCSTR orderId,         // Custom order ID (optional)
    LPCSTR strategyId,      // ATM strategy ID (optional)
    LPCSTR strategyName     // ATM strategy template name (optional)
);

typedef int     (__stdcall *pfnFilled)(LPCSTR orderId);
typedef double  (__stdcall *pfnAvgFillPrice)(LPCSTR orderId);
typedef LPCSTR  (__stdcall *pfnOrderStatus)(LPCSTR orderId);
// Returns: "Accepted", "Working", "Filled", "Cancelled", "Rejected", "PartFilled", etc.

typedef int     (__stdcall *pfnConfirmOrders)(int confirm);
// 0 = no confirmation dialogs, 1 = show confirmation dialogs

// Orders and Strategies lists
typedef LPCSTR  (__stdcall *pfnOrders)(LPCSTR account);
// Returns pipe-delimited list of order IDs

typedef LPCSTR  (__stdcall *pfnStrategies)(LPCSTR account);
// Returns pipe-delimited list of strategy IDs

typedef LPCSTR  (__stdcall *pfnStrategyPosition)(LPCSTR strategyId);
// Returns position for ATM strategy

// ATM Strategy specific
typedef LPCSTR  (__stdcall *pfnStopOrders)(LPCSTR strategyId);
typedef LPCSTR  (__stdcall *pfnTargetOrders)(LPCSTR strategyId);

// Price injection (for external data feeds)
typedef int     (__stdcall *pfnAsk)(LPCSTR instrument, double price, int size);
typedef int     (__stdcall *pfnBid)(LPCSTR instrument, double price, int size);
typedef int     (__stdcall *pfnLast)(LPCSTR instrument, double price, int size);

// Playback synchronization
typedef int     (__stdcall *pfnAskPlayback)(LPCSTR instrument, double price, int size, LPCSTR timestamp);
typedef int     (__stdcall *pfnBidPlayback)(LPCSTR instrument, double price, int size, LPCSTR timestamp);
typedef int     (__stdcall *pfnLastPlayback)(LPCSTR instrument, double price, int size, LPCSTR timestamp);

//=============================================================================
// NtDirect class - Wrapper for dynamic loading of NtDirect.dll
//=============================================================================

class NtDirect
{
public:
    NtDirect();
    ~NtDirect();
    
    bool Load();
    void Unload();
    bool IsLoaded() const { return m_hDll != nullptr; }
    
    // Connection
    int Connected(int showMessage = 0);
    int TearDown();
    
    // Market Data
    int SubscribeMarketData(const char* instrument);
    int UnSubscribeMarketData(const char* instrument);
    double MarketData(const char* instrument, int dataType);
    
    // Convenience market data functions
    double GetLast(const char* instrument)    { return MarketData(instrument, 0); }
    double GetBid(const char* instrument)     { return MarketData(instrument, 1); }
    double GetAsk(const char* instrument)     { return MarketData(instrument, 2); }
    double GetVolume(const char* instrument)  { return MarketData(instrument, 3); }
    int    GetLastSize(const char* instrument){ return (int)MarketData(instrument, 4); }
    int    GetBidSize(const char* instrument) { return (int)MarketData(instrument, 5); }
    int    GetAskSize(const char* instrument) { return (int)MarketData(instrument, 6); }
    
    // Account
    double CashValue(const char* account);
    double BuyingPower(const char* account);
    double RealizedPnL(const char* account);
    
    // Position
    int MarketPosition(const char* instrument, const char* account);
    double AvgEntryPrice(const char* instrument, const char* account);
    double AvgEntryPriceByOrderId(const char* orderId);
    
    // Orders
    const char* NewOrderId();
    int Command(const char* command, const char* account, const char* instrument,
                const char* action, int quantity, const char* orderType,
                double limitPrice, double stopPrice, const char* timeInForce,
                const char* oco, const char* orderId, const char* strategyId,
                const char* strategyName);
    
    int Filled(const char* orderId);
    double AvgFillPrice(const char* orderId);
    const char* OrderStatus(const char* orderId);
    
    int ConfirmOrders(int confirm);
    
    const char* Orders(const char* account);
    const char* Strategies(const char* account);
    
    // Convenience order functions
    int PlaceMarketOrder(const char* account, const char* instrument, 
                         const char* action, int quantity, const char* orderId = "");
    int PlaceLimitOrder(const char* account, const char* instrument,
                        const char* action, int quantity, double limitPrice,
                        const char* orderId = "");
    int PlaceStopOrder(const char* account, const char* instrument,
                       const char* action, int quantity, double stopPrice,
                       const char* orderId = "");
    int CancelOrder(const char* orderId);
    int ClosePosition(const char* account, const char* instrument);
    int FlattenEverything();

private:
    HMODULE m_hDll;
    
    // Function pointers
    pfnConnected            m_pfnConnected;
    pfnTearDown             m_pfnTearDown;
    pfnSubscribeMarketData  m_pfnSubscribeMarketData;
    pfnUnSubscribeMarketData m_pfnUnSubscribeMarketData;
    pfnMarketData           m_pfnMarketData;
    pfnCashValue            m_pfnCashValue;
    pfnBuyingPower          m_pfnBuyingPower;
    pfnRealizedPnL          m_pfnRealizedPnL;
    pfnMarketPosition       m_pfnMarketPosition;
    pfnAvgEntryPrice        m_pfnAvgEntryPrice;
    pfnAvgEntryPriceByOrderId m_pfnAvgEntryPriceByOrderId;
    pfnNewOrderId           m_pfnNewOrderId;
    pfnCommand              m_pfnCommand;
    pfnFilled               m_pfnFilled;
    pfnAvgFillPrice         m_pfnAvgFillPrice;
    pfnOrderStatus          m_pfnOrderStatus;
    pfnConfirmOrders        m_pfnConfirmOrders;
    pfnOrders               m_pfnOrders;
    pfnStrategies           m_pfnStrategies;
};

#endif // NTDIRECT_H
