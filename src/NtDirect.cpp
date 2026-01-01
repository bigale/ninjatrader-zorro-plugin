// NtDirect.cpp - NinjaTrader ATI DLL Wrapper Implementation
// Copyright (c) 2025

#include "NtDirect.h"
#include <string>

//=============================================================================
// Helper macro for loading functions
//=============================================================================

#define LOAD_FUNC(name, type) \
    m_pfn##name = (type)GetProcAddress(m_hDll, #name); \
    if (!m_pfn##name) { \
        /* Function not found - may be optional */ \
    }

//=============================================================================
// Constructor / Destructor
//=============================================================================

NtDirect::NtDirect()
    : m_hDll(nullptr)
    , m_pfnConnected(nullptr)
    , m_pfnTearDown(nullptr)
    , m_pfnSubscribeMarketData(nullptr)
    , m_pfnUnSubscribeMarketData(nullptr)
    , m_pfnMarketData(nullptr)
    , m_pfnCashValue(nullptr)
    , m_pfnBuyingPower(nullptr)
    , m_pfnRealizedPnL(nullptr)
    , m_pfnMarketPosition(nullptr)
    , m_pfnAvgEntryPrice(nullptr)
    , m_pfnAvgEntryPriceByOrderId(nullptr)
    , m_pfnNewOrderId(nullptr)
    , m_pfnCommand(nullptr)
    , m_pfnFilled(nullptr)
    , m_pfnAvgFillPrice(nullptr)
    , m_pfnOrderStatus(nullptr)
    , m_pfnConfirmOrders(nullptr)
    , m_pfnOrders(nullptr)
    , m_pfnStrategies(nullptr)
{
}

NtDirect::~NtDirect()
{
    Unload();
}

//=============================================================================
// Load / Unload
//=============================================================================

bool NtDirect::Load()
{
    if (m_hDll) return true;  // Already loaded
    
    // Try multiple locations for NtDirect.dll
    // NT8 installs it in System32/SysWOW64
    const char* paths[] = {
        "NtDirect.dll",                                    // System path
        "C:\\Windows\\SysWOW64\\NtDirect.dll",            // 32-bit on 64-bit Windows
        "C:\\Windows\\System32\\NtDirect.dll",            // 32-bit Windows
        nullptr
    };
    
    for (int i = 0; paths[i] != nullptr; i++) {
        m_hDll = LoadLibraryA(paths[i]);
        if (m_hDll) break;
    }
    
    if (!m_hDll) {
        return false;
    }
    
    // Load all function pointers
    LOAD_FUNC(Connected, pfnConnected);
    LOAD_FUNC(TearDown, pfnTearDown);
    LOAD_FUNC(SubscribeMarketData, pfnSubscribeMarketData);
    LOAD_FUNC(UnSubscribeMarketData, pfnUnSubscribeMarketData);
    LOAD_FUNC(MarketData, pfnMarketData);
    LOAD_FUNC(CashValue, pfnCashValue);
    LOAD_FUNC(BuyingPower, pfnBuyingPower);
    LOAD_FUNC(RealizedPnL, pfnRealizedPnL);
    LOAD_FUNC(MarketPosition, pfnMarketPosition);
    LOAD_FUNC(AvgEntryPrice, pfnAvgEntryPrice);
    LOAD_FUNC(NewOrderId, pfnNewOrderId);
    LOAD_FUNC(Command, pfnCommand);
    LOAD_FUNC(Filled, pfnFilled);
    LOAD_FUNC(AvgFillPrice, pfnAvgFillPrice);
    LOAD_FUNC(OrderStatus, pfnOrderStatus);
    LOAD_FUNC(ConfirmOrders, pfnConfirmOrders);
    LOAD_FUNC(Orders, pfnOrders);
    LOAD_FUNC(Strategies, pfnStrategies);
    
    // Check critical functions
    if (!m_pfnConnected || !m_pfnCommand || !m_pfnMarketData) {
        Unload();
        return false;
    }
    
    return true;
}

void NtDirect::Unload()
{
    if (m_hDll) {
        // Disconnect cleanly
        if (m_pfnTearDown) {
            m_pfnTearDown();
        }
        
        FreeLibrary(m_hDll);
        m_hDll = nullptr;
    }
    
    // Clear all function pointers
    m_pfnConnected = nullptr;
    m_pfnTearDown = nullptr;
    m_pfnSubscribeMarketData = nullptr;
    m_pfnUnSubscribeMarketData = nullptr;
    m_pfnMarketData = nullptr;
    m_pfnCashValue = nullptr;
    m_pfnBuyingPower = nullptr;
    m_pfnRealizedPnL = nullptr;
    m_pfnMarketPosition = nullptr;
    m_pfnAvgEntryPrice = nullptr;
    m_pfnAvgEntryPriceByOrderId = nullptr;
    m_pfnNewOrderId = nullptr;
    m_pfnCommand = nullptr;
    m_pfnFilled = nullptr;
    m_pfnAvgFillPrice = nullptr;
    m_pfnOrderStatus = nullptr;
    m_pfnConfirmOrders = nullptr;
    m_pfnOrders = nullptr;
    m_pfnStrategies = nullptr;
}

//=============================================================================
// Connection
//=============================================================================

int NtDirect::Connected(int showMessage)
{
    if (!m_pfnConnected) return -1;
    return m_pfnConnected(showMessage);
}

int NtDirect::TearDown()
{
    if (!m_pfnTearDown) return -1;
    return m_pfnTearDown();
}

//=============================================================================
// Market Data
//=============================================================================

int NtDirect::SubscribeMarketData(const char* instrument)
{
    if (!m_pfnSubscribeMarketData || !instrument) return -1;
    return m_pfnSubscribeMarketData(instrument);
}

int NtDirect::UnSubscribeMarketData(const char* instrument)
{
    if (!m_pfnUnSubscribeMarketData || !instrument) return -1;
    return m_pfnUnSubscribeMarketData(instrument);
}

double NtDirect::MarketData(const char* instrument, int dataType)
{
    if (!m_pfnMarketData || !instrument) return 0.0;
    return m_pfnMarketData(instrument, dataType);
}

//=============================================================================
// Account
//=============================================================================

double NtDirect::CashValue(const char* account)
{
    if (!m_pfnCashValue || !account) return 0.0;
    return m_pfnCashValue(account);
}

double NtDirect::BuyingPower(const char* account)
{
    if (!m_pfnBuyingPower || !account) return 0.0;
    return m_pfnBuyingPower(account);
}

double NtDirect::RealizedPnL(const char* account)
{
    if (!m_pfnRealizedPnL || !account) return 0.0;
    return m_pfnRealizedPnL(account);
}

//=============================================================================
// Position
//=============================================================================

int NtDirect::MarketPosition(const char* instrument, const char* account)
{
    if (!m_pfnMarketPosition || !instrument || !account) return 0;
    return m_pfnMarketPosition(instrument, account);
}

double NtDirect::AvgEntryPrice(const char* instrument, const char* account)
{
    if (!m_pfnAvgEntryPrice || !instrument || !account) return 0.0;
    return m_pfnAvgEntryPrice(instrument, account);
}

double NtDirect::AvgEntryPriceByOrderId(const char* orderId)
{
    if (!m_pfnAvgEntryPriceByOrderId || !orderId) return 0.0;
    return m_pfnAvgEntryPriceByOrderId(orderId);
}

//=============================================================================
// Orders
//=============================================================================

const char* NtDirect::NewOrderId()
{
    if (!m_pfnNewOrderId) return "";
    const char* result = m_pfnNewOrderId();
    return result ? result : "";
}

int NtDirect::Command(const char* command, const char* account, const char* instrument,
                      const char* action, int quantity, const char* orderType,
                      double limitPrice, double stopPrice, const char* timeInForce,
                      const char* oco, const char* orderId, const char* strategyId,
                      const char* strategyName)
{
    if (!m_pfnCommand) return -1;
    
    return m_pfnCommand(
        command ? command : "",
        account ? account : "",
        instrument ? instrument : "",
        action ? action : "",
        quantity,
        orderType ? orderType : "",
        limitPrice,
        stopPrice,
        timeInForce ? timeInForce : "",
        oco ? oco : "",
        orderId ? orderId : "",
        strategyId ? strategyId : "",
        strategyName ? strategyName : ""
    );
}

int NtDirect::Filled(const char* orderId)
{
    if (!m_pfnFilled || !orderId) return 0;
    return m_pfnFilled(orderId);
}

double NtDirect::AvgFillPrice(const char* orderId)
{
    if (!m_pfnAvgFillPrice || !orderId) return 0.0;
    return m_pfnAvgFillPrice(orderId);
}

const char* NtDirect::OrderStatus(const char* orderId)
{
    if (!m_pfnOrderStatus || !orderId) return "";
    const char* result = m_pfnOrderStatus(orderId);
    return result ? result : "";
}

int NtDirect::ConfirmOrders(int confirm)
{
    if (!m_pfnConfirmOrders) return -1;
    return m_pfnConfirmOrders(confirm);
}

const char* NtDirect::Orders(const char* account)
{
    if (!m_pfnOrders || !account) return "";
    const char* result = m_pfnOrders(account);
    return result ? result : "";
}

const char* NtDirect::Strategies(const char* account)
{
    if (!m_pfnStrategies || !account) return "";
    const char* result = m_pfnStrategies(account);
    return result ? result : "";
}

//=============================================================================
// Convenience Order Functions
//=============================================================================

int NtDirect::PlaceMarketOrder(const char* account, const char* instrument,
                                const char* action, int quantity, const char* orderId)
{
    return Command("PLACE", account, instrument, action, quantity,
                   "MARKET", 0.0, 0.0, "GTC", "", orderId, "", "");
}

int NtDirect::PlaceLimitOrder(const char* account, const char* instrument,
                               const char* action, int quantity, double limitPrice,
                               const char* orderId)
{
    return Command("PLACE", account, instrument, action, quantity,
                   "LIMIT", limitPrice, 0.0, "GTC", "", orderId, "", "");
}

int NtDirect::PlaceStopOrder(const char* account, const char* instrument,
                              const char* action, int quantity, double stopPrice,
                              const char* orderId)
{
    return Command("PLACE", account, instrument, action, quantity,
                   "STOP", 0.0, stopPrice, "GTC", "", orderId, "", "");
}

int NtDirect::CancelOrder(const char* orderId)
{
    return Command("CANCEL", "", "", "", 0, "", 0.0, 0.0, "", "", orderId, "", "");
}

int NtDirect::ClosePosition(const char* account, const char* instrument)
{
    return Command("CLOSEPOSITION", account, instrument, "", 0, "", 0.0, 0.0, "", "", "", "", "");
}

int NtDirect::FlattenEverything()
{
    return Command("FLATTENEVERYTHING", "", "", "", 0, "", 0.0, 0.0, "", "", "", "", "");
}
