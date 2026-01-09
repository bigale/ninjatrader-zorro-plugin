// TcpBridge.h - TCP Bridge to NinjaTrader 8.1+ AddOn
// Replaces NtDirect.dll for NT8 8.1 compatibility

#pragma once

#ifndef TCPBRIDGE_H
#define TCPBRIDGE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

//=============================================================================
// TcpBridge class - Communicates with NinjaTrader 8.1+ via TCP
//=============================================================================

class TcpBridge
{
public:
    TcpBridge();
    ~TcpBridge();
    
    bool Connect(const char* host = "127.0.0.1", int port = 8888);
    void Disconnect();
    bool IsConnected() const { return m_connected; }
    
    // Low-level command interface (public for direct use)
    std::string SendCommand(const std::string& command);
    
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
    
    // Account
    double CashValue(const char* account);
    double BuyingPower(const char* account);
    double RealizedPnL(const char* account);
    
    // Position
    int MarketPosition(const char* instrument, const char* account);
    double AvgEntryPrice(const char* instrument, const char* account);
    
    // Orders
    const char* NewOrderId();
    const char* GetLastNtOrderId() const { return m_lastNtOrderId.c_str(); }
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
    int CancelOrder(const char* orderId);
    int ClosePosition(const char* account, const char* instrument);

private:
    SOCKET m_socket;
    bool m_connected;
    std::string m_lastResponse;
    char m_orderIdBuffer[64];
    int m_nextOrderId;
    std::string m_lastNtOrderId;  // Store NT order ID from last PLACEORDER
    
    // Communication helpers
    bool InitializeWinsock();
    void CleanupWinsock();
    
    // Parsing helpers
    std::vector<std::string> SplitResponse(const std::string& response, char delimiter);
};

#endif // TCPBRIDGE_H
