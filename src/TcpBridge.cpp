// TcpBridge.cpp - TCP Bridge to NinjaTrader 8.1+ AddOn Implementation
// Copyright (c) 2025

#include "TcpBridge.h"
#include <sstream>
#include <vector>

//=============================================================================
// Constructor / Destructor
//=============================================================================

TcpBridge::TcpBridge()
    : m_socket(INVALID_SOCKET)
    , m_connected(false)
    , m_nextOrderId(1000)
{
    InitializeWinsock();
}

TcpBridge::~TcpBridge()
{
    Disconnect();
    CleanupWinsock();
}

//=============================================================================
// Connection Management
//=============================================================================

bool TcpBridge::InitializeWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return (result == 0);
}

void TcpBridge::CleanupWinsock()
{
    WSACleanup();
}

bool TcpBridge::Connect(const char* host, int port)
{
    if (m_connected) {
        return true;  // Already connected
    }
    
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        return false;
    }
    
    // Setup address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    // Convert host to address
    inet_pton(AF_INET, host, &serverAddr.sin_addr);
    
    // Connect
    if (connect(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    m_connected = true;
    
    // Test connection with PING
    std::string response = SendCommand("PING");
    if (response != "PONG") {
        Disconnect();
        return false;
    }
    
    return true;
}

void TcpBridge::Disconnect()
{
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_connected = false;
}

//=============================================================================
// Communication Helper
//=============================================================================

std::string TcpBridge::SendCommand(const std::string& command)
{
    if (!m_connected || m_socket == INVALID_SOCKET) {
        return "ERROR:Not connected";
    }
    
    // Send command
    std::string fullCommand = command + "\n";
    int sent = send(m_socket, fullCommand.c_str(), (int)fullCommand.length(), 0);
    if (sent == SOCKET_ERROR) {
        m_connected = false;
        return "ERROR:Send failed";
    }
    
    // Receive response
    char buffer[4096];
    int received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    if (received == SOCKET_ERROR || received == 0) {
        m_connected = false;
        return "ERROR:Receive failed";
    }
    
    buffer[received] = '\0';
    m_lastResponse = std::string(buffer);
    
    // Remove trailing newline
    if (!m_lastResponse.empty() && m_lastResponse.back() == '\n') {
        m_lastResponse.pop_back();
    }
    
    return m_lastResponse;
}

std::vector<std::string> TcpBridge::SplitResponse(const std::string& response, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream ss(response);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        parts.push_back(item);
    }
    
    return parts;
}

//=============================================================================
// Connection
//=============================================================================

int TcpBridge::Connected(int showMessage)
{
    if (!m_connected) {
        // Try to connect
        if (!Connect()) {
            return -1;  // Failed to connect
        }
    }
    
    std::string response = SendCommand("CONNECTED");
    if (response.find("CONNECTED:1") != std::string::npos) {
        return 0;  // Connected
    }
    
    return -1;  // Not connected
}

int TcpBridge::TearDown()
{
    SendCommand("LOGOUT");
    Disconnect();
    return 0;
}

//=============================================================================
// Market Data
//=============================================================================

int TcpBridge::SubscribeMarketData(const char* instrument)
{
    if (!instrument) return -1;
    
    std::string cmd = std::string("SUBSCRIBE:") + instrument;
    std::string response = SendCommand(cmd);
    
    return (response.find("OK") != std::string::npos) ? 0 : -1;
}

int TcpBridge::UnSubscribeMarketData(const char* instrument)
{
    if (!instrument) return -1;
    
    std::string cmd = std::string("UNSUBSCRIBE:") + instrument;
    std::string response = SendCommand(cmd);
    
    return (response.find("OK") != std::string::npos) ? 0 : -1;
}

double TcpBridge::MarketData(const char* instrument, int dataType)
{
    if (!instrument) return 0.0;
    
    std::string cmd = std::string("GETPRICE:") + instrument;
    std::string response = SendCommand(cmd);
    
    // Parse response: PRICE:last:bid:ask:volume
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 5 || parts[0] != "PRICE") {
        return 0.0;
    }
    
    // dataType: 0=Last, 1=Bid, 2=Ask, 3=Volume
    switch (dataType) {
        case 0: return std::stod(parts[1]);  // Last
        case 1: return std::stod(parts[2]);  // Bid
        case 2: return std::stod(parts[3]);  // Ask
        case 3: return std::stod(parts[4]);  // Volume
        default: return 0.0;
    }
}

//=============================================================================
// Account
//=============================================================================

double TcpBridge::CashValue(const char* account)
{
    std::string response = SendCommand("GETACCOUNT");
    
    // Parse response: ACCOUNT:cashValue:buyingPower:realizedPnL
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 4 || parts[0] != "ACCOUNT") {
        return 0.0;
    }
    
    return std::stod(parts[1]);
}

double TcpBridge::BuyingPower(const char* account)
{
    std::string response = SendCommand("GETACCOUNT");
    
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 4 || parts[0] != "ACCOUNT") {
        return 0.0;
    }
    
    return std::stod(parts[2]);
}

double TcpBridge::RealizedPnL(const char* account)
{
    std::string response = SendCommand("GETACCOUNT");
    
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 4 || parts[0] != "ACCOUNT") {
        return 0.0;
    }
    
    return std::stod(parts[3]);
}

//=============================================================================
// Position
//=============================================================================

int TcpBridge::MarketPosition(const char* instrument, const char* account)
{
    if (!instrument) return 0;
    
    std::string cmd = std::string("GETPOSITION:") + instrument;
    std::string response = SendCommand(cmd);
    
    // Log to file for debugging
    FILE* log = fopen("C:\\Zorro_2.66\\TcpBridge_debug.log", "a");
    if (log) {
        fprintf(log, "[MarketPosition] query: %s\n", cmd.c_str());
        fprintf(log, "[MarketPosition] response: '%s'\n", response.c_str());
        fflush(log);
    }
    
    // Parse response: POSITION:quantity:avgPrice
    auto parts = SplitResponse(response, ':');
    
    if (log) {
        fprintf(log, "[MarketPosition] Parsed %zu parts:", parts.size());
        for (size_t i = 0; i < parts.size(); i++) {
            fprintf(log, " [%zu]='%s'", i, parts[i].c_str());
        }
        fprintf(log, "\n");
    }
    
    if (parts.size() < 3 || parts[0] != "POSITION") {
        if (log) {
            fprintf(log, "[MarketPosition] Parse FAILED: size=%zu, parts[0]='%s'\n", 
                parts.size(), parts.size() > 0 ? parts[0].c_str() : "NONE");
            fclose(log);
        }
        return 0;
    }
    
    int position = std::stoi(parts[1]);
    
    if (log) {
        fprintf(log, "[MarketPosition] Returning position: %d\n", position);
        fclose(log);
    }
    
    return position;
}

double TcpBridge::AvgEntryPrice(const char* instrument, const char* account)
{
    if (!instrument) return 0.0;
    
    std::string cmd = std::string("GETPOSITION:") + instrument;
    std::string response = SendCommand(cmd);
    
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 3 || parts[0] != "POSITION") {
        return 0.0;
    }
    
    return std::stod(parts[2]);
}

//=============================================================================
// Orders
//=============================================================================

const char* TcpBridge::NewOrderId()
{
    sprintf_s(m_orderIdBuffer, sizeof(m_orderIdBuffer), "ZORRO_%d", m_nextOrderId++);
    return m_orderIdBuffer;
}

int TcpBridge::Command(const char* command, const char* account, const char* instrument,
                       const char* action, int quantity, const char* orderType,
                       double limitPrice, double stopPrice, const char* timeInForce,
                       const char* oco, const char* orderId, const char* strategyId,
                       const char* strategyName)
{
    // Build command based on type
    std::ostringstream cmd;
    
    if (strcmp(command, "PLACE") == 0) {
        // PLACEORDER:BUY/SELL:INSTRUMENT:QUANTITY:ORDERTYPE:LIMITPRICE:STOPPRICE
        cmd << "PLACEORDER:" << action << ":" << instrument << ":" << quantity 
            << ":" << orderType << ":" << limitPrice << ":" << stopPrice;
        
        std::string response = SendCommand(cmd.str());
        
        // Extract NT order ID from response: "ORDER:fa41b14fff514c69b5749bba57471eb8"
        auto parts = SplitResponse(response, ':');
        if (parts.size() >= 2 && parts[0] == "ORDER") {
            m_lastNtOrderId = parts[1];  // Store the NT GUID
            
            FILE* log = fopen("C:\\Zorro_2.66\\TcpBridge_debug.log", "a");
            if (log) {
                fprintf(log, "[Command] PLACEORDER response: %s\n", response.c_str());
                fprintf(log, "[Command] Extracted NT order ID: %s\n", m_lastNtOrderId.c_str());
                fclose(log);
            }
            
            return 0;  // Success
        }
        
        return -1;  // Failed
    }
    else if (strcmp(command, "CANCEL") == 0) {
        cmd << "CANCELORDER:" << orderId;
        std::string response = SendCommand(cmd.str());
        return (response.find("OK") != std::string::npos) ? 0 : -1;
    }
    
    return -1;
}

int TcpBridge::Filled(const char* orderId)
{
    if (!orderId) return 0;
    
    std::string cmd = std::string("GETORDERSTATUS:") + orderId;
    std::string response = SendCommand(cmd);
    
    // Parse response: ORDERSTATUS:orderId:state:filled:avgFillPrice
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 4 || parts[0] != "ORDERSTATUS") {
        return 0;
    }
    
    return std::stoi(parts[3]);  // filled quantity
}

double TcpBridge::AvgFillPrice(const char* orderId)
{
    if (!orderId) return 0.0;
    
    std::string cmd = std::string("GETORDERSTATUS:") + orderId;
    std::string response = SendCommand(cmd);
    
    // Parse response: ORDERSTATUS:orderId:state:filled:avgFillPrice
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 5 || parts[0] != "ORDERSTATUS") {
        return 0.0;
    }
    
    return std::stod(parts[4]);  // avg fill price
}

const char* TcpBridge::OrderStatus(const char* orderId)
{
    if (!orderId) return "Unknown";
    
    std::string cmd = std::string("GETORDERSTATUS:") + orderId;
    std::string response = SendCommand(cmd);
    
    // Parse response: ORDERSTATUS:orderId:state:filled:avgFillPrice
    auto parts = SplitResponse(response, ':');
    if (parts.size() < 3 || parts[0] != "ORDERSTATUS") {
        return "Unknown";
    }
    
    // Store in static buffer (not thread-safe but OK for single-threaded Zorro)
    static char statusBuffer[64];
    strncpy_s(statusBuffer, sizeof(statusBuffer), parts[2].c_str(), _TRUNCATE);
    return statusBuffer;
}

int TcpBridge::ConfirmOrders(int confirm)
{
    // Not applicable for TCP bridge
    return 0;
}

const char* TcpBridge::Orders(const char* account)
{
    return "";
}

const char* TcpBridge::Strategies(const char* account)
{
    return "";
}

//=============================================================================
// Convenience Order Functions
//=============================================================================

int TcpBridge::PlaceMarketOrder(const char* account, const char* instrument,
                                 const char* action, int quantity, const char* orderId)
{
    return Command("PLACE", account, instrument, action, quantity,
                   "MARKET", 0.0, 0.0, "GTC", "", orderId, "", "");
}

int TcpBridge::PlaceLimitOrder(const char* account, const char* instrument,
                                const char* action, int quantity, double limitPrice,
                                const char* orderId)
{
    return Command("PLACE", account, instrument, action, quantity,
                   "LIMIT", limitPrice, 0.0, "GTC", "", orderId, "", "");
}

int TcpBridge::CancelOrder(const char* orderId)
{
    return Command("CANCEL", "", "", "", 0, "", 0.0, 0.0, "", "", orderId, "", "");
}

int TcpBridge::ClosePosition(const char* account, const char* instrument)
{
    // Would need to get current position and place opposite order
    // Not implemented yet
    return -1;
}
