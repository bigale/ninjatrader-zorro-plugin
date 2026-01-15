// ZorroBridge.cs - NinjaTrader 8.1 AddOn for Zorro Integration
// Place this file in: Documents\NinjaTrader 8\bin\Custom\AddOns\
//
// This AddOn creates a TCP server that allows Zorro to communicate with NinjaTrader 8.1+
// Compile: Tools -> Compile -> Compile All (or F5) in NinjaTrader

#region Using declarations
using System;
using System.Collections.Generic;
using System.Collections.Concurrent;  // NEW: For ConcurrentDictionary
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using NinjaTrader.Cbi;
using NinjaTrader.NinjaScript;
using NinjaTrader.Data;  // For BarsRequest
using NinjaTrader.NinjaScript.BarsTypes;  // For BarsPeriod and BarsPeriodType
#endregion

namespace NinjaTrader.NinjaScript.AddOns
{
    // Logging levels
    public enum LogLevel
    {
        TRACE = 0,   // Every call, every detail
        DEBUG = 1,   // Important operations and their results
        INFO = 2,    // Heartbeat and significant events only
        WARN = 3,    // Warnings and errors
        ERROR = 4    // Errors only
    }
    
    public class ZorroBridge : AddOnBase
    {
        private TcpListener tcpListener;
        private Thread listenerThread;
        private bool isRunning = false;
        private const int PORT = 8888;
        
        private Account currentAccount;
        // **FIXED: Use thread-safe ConcurrentDictionary instead of Dictionary**
        // Multiple client threads can access these concurrently
        private ConcurrentDictionary<string, Instrument> subscribedInstruments = new ConcurrentDictionary<string, Instrument>();
        private ConcurrentDictionary<string, Order> activeOrders = new ConcurrentDictionary<string, Order>();
        
        // Logging configuration
        private LogLevel currentLogLevel = LogLevel.INFO;  // Default to INFO
        private DateTime lastHeartbeat = DateTime.MinValue;
        private int priceRequestCount = 0;
        private int orderCount = 0;
        
        protected override void OnStateChange()
        {
            if (State == State.SetDefaults)
            {
                Name = "Zorro Bridge";
                Description = "TCP bridge for Zorro trading platform - NT8 8.1+ compatible";
            }
            else if (State == State.Configure)
            {
                Log(LogLevel.INFO, "Zorro Bridge starting...");
                StartTcpServer();
            }
            else if (State == State.Terminated)
            {
                Log(LogLevel.INFO, "Zorro Bridge stopping...");
                StopTcpServer();
            }
        }
        
        // Centralized logging method
        private void Log(LogLevel level, string message)
        {
            if (level < currentLogLevel)
                return;
                
            string prefix = "";
            switch (level)
            {
                case LogLevel.TRACE: prefix = "[TRACE]"; break;
                case LogLevel.DEBUG: prefix = "[DEBUG]"; break;
                case LogLevel.INFO:  prefix = "[INFO]"; break;
                case LogLevel.WARN:  prefix = "[WARN]"; break;
                case LogLevel.ERROR: prefix = "[ERROR]"; break;
            }
            
            Print($"[Zorro Bridge] {prefix} {message}");
        }
        
        // Heartbeat logging - only logs periodically
        private void Heartbeat()
        {
            TimeSpan elapsed = DateTime.Now - lastHeartbeat;
            if (elapsed.TotalSeconds >= 10)
            {
                Log(LogLevel.INFO, $"Status OK | Prices:{priceRequestCount} Orders:{orderCount} Instruments:{subscribedInstruments.Count}");
                lastHeartbeat = DateTime.Now;
                priceRequestCount = 0;
            }
        }

        private void StartTcpServer()
        {
            try
            {
                tcpListener = new TcpListener(IPAddress.Loopback, PORT);
                tcpListener.Start();
                isRunning = true;
                
                listenerThread = new Thread(ListenForClients);
                listenerThread.IsBackground = true;
                listenerThread.Start();
                
                Log(LogLevel.INFO, $"Listening on port {PORT}");
                lastHeartbeat = DateTime.Now;
            }
            catch (Exception ex)
            {
                Log(LogLevel.ERROR, $"Failed to start TCP server: {ex.Message}");
            }
        }

        private void StopTcpServer()
        {
            isRunning = false;
            
            if (tcpListener != null)
            {
                tcpListener.Stop();
            }
            
            if (listenerThread != null && listenerThread.IsAlive)
            {
                listenerThread.Join(1000);
            }
        }

        private void ListenForClients()
        {
            while (isRunning)
            {
                try
                {
                    TcpClient client = tcpListener.AcceptTcpClient();
                    Log(LogLevel.DEBUG, "Client connected");
                    Thread clientThread = new Thread(HandleClient);
                    clientThread.IsBackground = true;
                    clientThread.Start(client);
                }
                catch (SocketException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    Log(LogLevel.ERROR, $"Error accepting client: {ex.Message}");
                }
            }
        }

        private void HandleClient(object obj)
        {
            TcpClient client = (TcpClient)obj;
            
            try
            {
                using (NetworkStream stream = client.GetStream())
                {
                    byte[] buffer = new byte[4096];
                    
                    while (client.Connected && isRunning)
                    {
                        int bytesRead = stream.Read(buffer, 0, buffer.Length);
                        if (bytesRead == 0) break;
                        
                        string request = Encoding.UTF8.GetString(buffer, 0, bytesRead).Trim();
                        Log(LogLevel.TRACE, $"<< {request}");
                        
                        string response = ProcessCommand(request);
                        
                        Log(LogLevel.TRACE, $">> {response}");
                        
                        byte[] responseBytes = Encoding.UTF8.GetBytes(response + "\n");
                        stream.Write(responseBytes, 0, responseBytes.Length);
                        stream.Flush();
                    }
                }
            }
            catch (Exception ex)
            {
                Log(LogLevel.ERROR, $"Error handling client: {ex.Message}");
            }
            finally
            {
                Log(LogLevel.DEBUG, "Client disconnected");
                client.Close();
            }
        }

        private string ProcessCommand(string command)
        {
            try
            {
                string[] parts = command.Split(':');
                string cmd = parts[0].ToUpper();

                switch (cmd)
                {
                    case "PING":
                        return "PONG";

                    case "VERSION":
                        return "VERSION:1.0";

                    case "LOGIN":
                        return HandleLogin(parts);

                    case "LOGOUT":
                        return HandleLogout();

                    case "CONNECTED":
                        return currentAccount != null ? "CONNECTED:1" : "CONNECTED:0";

                    case "SUBSCRIBE":
                        return HandleSubscribe(parts);

                    case "UNSUBSCRIBE":
                        return HandleUnsubscribe(parts);

                    case "GETPRICE":
                        return HandleGetPrice(parts);

                    case "GETACCOUNT":
                        return HandleGetAccount();

                    case "GETPOSITION":
                        return HandleGetPosition(parts);

                    case "PLACEORDER":
                        return HandlePlaceOrder(parts);

                    case "CANCELORDER":
                        return HandleCancelOrder(parts);
                    
                    case "SETLOGLEVEL":
                        return HandleSetLogLevel(parts);
                    
                    case "GETORDERSTATUS":
                        return HandleGetOrderStatus(parts);
                    
                    case "GETHISTORY":
                        return HandleGetHistory(parts);
                    
                    case "GETINSTRUMENTS":
                        return HandleGetInstruments();

                    default:
                        return $"ERROR:Unknown command: {cmd}";
                }
            }
            catch (Exception ex)
            {
                return $"ERROR:{ex.Message}";
            }
        }

        private string HandleLogin(string[] parts)
        {
            if (parts.Length < 2)
                return "ERROR:Account name required";

            string accountName = parts[1];
            currentAccount = Account.All.FirstOrDefault(a => a.Name == accountName);

            if (currentAccount == null)
            {
                Log(LogLevel.ERROR, $"Account '{accountName}' not found");
                return $"ERROR:Account '{accountName}' not found";
            }

            Log(LogLevel.INFO, $"Logged in to account: {accountName}");
            return $"OK:Logged in to {accountName}";
        }

        private string HandleLogout()
        {
            if (currentAccount != null)
            {
                Log(LogLevel.INFO, $"Logged out from: {currentAccount.Name}");
                currentAccount = null;
            }
            subscribedInstruments.Clear();
            return "OK:Logged out";
        }

        private string HandleSubscribe(string[] parts)
        {
            if (parts.Length < 2)
                return "ERROR:Instrument name required";

            string instrumentName = parts[1];
            
            Log(LogLevel.DEBUG, $"Subscribe request: {instrumentName}");
            
            Instrument instrument = Instrument.GetInstrument(instrumentName);
            if (instrument == null)
            {
                Log(LogLevel.ERROR, $"Instrument '{instrumentName}' not found");
                return $"ERROR:Instrument '{instrumentName}' not found in NinjaTrader";
            }

            Log(LogLevel.TRACE, $"Found: {instrument.FullName}");
            
            if (instrument.MarketData == null)
            {
                Log(LogLevel.WARN, $"MarketData is null for {instrumentName} - connect to data feed");
            }

            subscribedInstruments[instrumentName] = instrument;
            
            // Get contract specifications
            double tickSize = instrument.MasterInstrument.TickSize;
            double pointValue = instrument.MasterInstrument.PointValue;
            
            Log(LogLevel.INFO, $"Subscribed to {instrumentName}");
            Log(LogLevel.DEBUG, $"Contract specs: TickSize={tickSize} PointValue={pointValue}");
            
            // Return format: OK:Subscribed:{instrument}:{tickSize}:{pointValue}
            return $"OK:Subscribed:{instrumentName}:{tickSize}:{pointValue}";
        }

        private string HandleUnsubscribe(string[] parts)
        {
            if (parts.Length < 2)
                return "ERROR:Instrument name required";

            string instrumentName = parts[1];
            // **FIXED: ConcurrentDictionary uses TryRemove instead of Remove**
            Instrument removedInstrument;
            subscribedInstruments.TryRemove(instrumentName, out removedInstrument);
            
            return $"OK:Unsubscribed from {instrumentName}";
        }

        private string HandleGetPrice(string[] parts)
        {
            if (parts.Length < 2)
                return "ERROR:Instrument name required";

            string zorroSymbol = parts[1];
            
            Log(LogLevel.TRACE, $"GetPrice: {zorroSymbol}");
            
            if (!subscribedInstruments.ContainsKey(zorroSymbol))
            {
                Log(LogLevel.ERROR, $"Not subscribed to {zorroSymbol}");
                return "ERROR:Not subscribed to instrument";
            }

            Instrument instrument = subscribedInstruments[zorroSymbol];
            
            if (instrument == null)
            {
                Log(LogLevel.ERROR, $"Instrument is null for {zorroSymbol}");
                return "ERROR:Instrument is null";
            }
            
            try
            {
                double last = 0;
                double bid = 0;
                double ask = 0;
                long volume = 0;
                
                if (instrument.MarketData.Last != null)
                    last = instrument.MarketData.Last.Price;
                    
                if (instrument.MarketData.Bid != null)
                    bid = instrument.MarketData.Bid.Price;
                    
                if (instrument.MarketData.Ask != null)
                    ask = instrument.MarketData.Ask.Price;
                    
                if (instrument.MarketData.DailyVolume != null)
                    volume = instrument.MarketData.DailyVolume.Volume;

                priceRequestCount++;
                Heartbeat();  // Check if we should log heartbeat
                
                Log(LogLevel.DEBUG, $"Price {zorroSymbol}: L:{last} B:{bid} A:{ask}");
                Log(LogLevel.TRACE, $"Volume: {volume}");

                return $"PRICE:{last}:{bid}:{ask}:{volume}";
            }
            catch (Exception ex)
            {
                Log(LogLevel.ERROR, $"Error getting price: {ex.Message}");
                return $"ERROR:{ex.Message}";
            }
        }

        private string HandleGetAccount()
        {
            if (currentAccount == null)
                return "ERROR:Not logged in";

            double cashValue = currentAccount.Get(AccountItem.CashValue, Currency.UsDollar);
            double buyingPower = currentAccount.Get(AccountItem.BuyingPower, Currency.UsDollar);
            double realizedPnL = currentAccount.Get(AccountItem.RealizedProfitLoss, Currency.UsDollar);

            // Calculate unrealized P&L from open positions
            double unrealizedPnL = 0;
            
            foreach (Position pos in currentAccount.Positions)
            {
                if (pos.MarketPosition != MarketPosition.Flat)
                {
                    try
                    {
                        // Get current market price
                        double currentPrice = 0;
                        if (pos.Instrument.MarketData != null && pos.Instrument.MarketData.Last != null)
                        {
                            currentPrice = pos.Instrument.MarketData.Last.Price;
                        }
                        
                        if (currentPrice > 0)
                        {
                            double entryPrice = pos.AveragePrice;
                            int quantity = pos.Quantity;
                            double pointValue = pos.Instrument.MasterInstrument.PointValue;
                            
                            // Calculate P&L based on position direction
                            if (pos.MarketPosition == MarketPosition.Long)
                            {
                                // Long: profit when price goes up
                                unrealizedPnL += (currentPrice - entryPrice) * quantity * pointValue;
                            }
                            else if (pos.MarketPosition == MarketPosition.Short)
                            {
                                // Short: profit when price goes down
                                unrealizedPnL += (entryPrice - currentPrice) * quantity * pointValue;
                            }
                            

                            Log(LogLevel.TRACE, $"UnrealizedPnL: {pos.Instrument.FullName} {pos.MarketPosition} " +
                                $"Entry:{entryPrice} Current:{currentPrice} Qty:{quantity} PV:{pointValue} " +
                                $"Contribution:{(pos.MarketPosition == MarketPosition.Long ? (currentPrice - entryPrice) : (entryPrice - currentPrice)) * quantity * pointValue}");
                        }
                    }
                    catch (Exception ex)
                    {
                        Log(LogLevel.WARN, $"Error calculating unrealized P&L for {pos.Instrument.FullName}: {ex.Message}");
                    }
                }
            }

            Log(LogLevel.DEBUG, $"Account: Cash={cashValue} BuyPwr={buyingPower} RealPnL={realizedPnL} UnrealPnL={unrealizedPnL}");

            // Return format: ACCOUNT:cashValue:buyingPower:realizedPnL:unrealizedPnL
            return $"ACCOUNT:{cashValue}:{buyingPower}:{realizedPnL}:{unrealizedPnL}";
        }

        private string HandleGetPosition(string[] parts)
        {
            Log(LogLevel.DEBUG, "==== HandleGetPosition START ====");
            Log(LogLevel.TRACE, $"Parts: {string.Join(":", parts)}");
            
            if (currentAccount == null)
            {
                Log(LogLevel.ERROR, "Not logged in");
                return "ERROR:Not logged in";
            }

            if (parts.Length < 2)
            {
                Log(LogLevel.ERROR, "Instrument name missing");
                return "ERROR:Instrument name required";
            }

            string instrumentName = parts[1];
            Log(LogLevel.DEBUG, $"Querying position for: {instrumentName}");
            
            Instrument instrument = Instrument.GetInstrument(instrumentName);
            
            if (instrument == null)
            {
                Log(LogLevel.ERROR, $"Instrument '{instrumentName}' not found");
                return "ERROR:Instrument not found";
            }

            // Find position by iterating through positions
            int position = 0;
            double avgPrice = 0;
            
            Log(LogLevel.TRACE, $"Searching {currentAccount.Positions.Count()} positions for {instrument.FullName}");
            
            foreach (Position pos in currentAccount.Positions)
            {
                Log(LogLevel.TRACE, $"  Position: {pos.Instrument.FullName} Qty:{pos.Quantity} MarketPos:{pos.MarketPosition} AvgPrice:{pos.AveragePrice}");
                
                if (pos.Instrument == instrument)
                {
                    // NinjaTrader Position.Quantity is ALWAYS positive
                    // We need to check MarketPosition to determine sign
                    if (pos.MarketPosition == MarketPosition.Long)
                        position = pos.Quantity;  // Positive for long
                    else if (pos.MarketPosition == MarketPosition.Short)
                        position = -pos.Quantity;  // Negative for short
                    else
                        position = 0;  // Flat
                    
                    avgPrice = pos.AveragePrice;
                    Log(LogLevel.DEBUG, $"MATCHED: Instrument={instrument.FullName} MarketPos={pos.MarketPosition} Qty={pos.Quantity} SignedPos={position} AvgPrice={avgPrice}");
                    break;
                }
            }

            if (position == 0)
                Log(LogLevel.DEBUG, "No position found (flat)");
            
            Log(LogLevel.INFO, $"POSITION QUERY: {instrumentName} = {position} contracts @ {avgPrice}");
            Log(LogLevel.DEBUG, "==== HandleGetPosition END ====");
            
            return $"POSITION:{position}:{avgPrice}";
        }

        private string HandlePlaceOrder(string[] parts)
        {
            // PLACEORDER:BUY/SELL:INSTRUMENT:QUANTITY:ORDERTYPE:LIMITPRICE:STOPPRICE
            Log(LogLevel.DEBUG, "==== PlaceOrder START ====");
            Log(LogLevel.TRACE, $"Raw: {string.Join(":", parts)}");
            
            if (currentAccount == null)
            {
                Log(LogLevel.ERROR, "Not logged in");
                return "ERROR:Not logged in";
            }
            
            if (parts.Length < 5)
            {
                Log(LogLevel.ERROR, $"Invalid format (expected >=5 parts, got {parts.Length})");
                return "ERROR:Invalid order format";
            }

            try
            {
                string action = parts[1].ToUpper();
                string instrumentName = parts[2];
                int quantity = int.Parse(parts[3]);
                string orderType = parts[4].ToUpper();
                double limitPrice = parts.Length > 5 ? double.Parse(parts[5]) : 0;
                double stopPrice = parts.Length > 6 ? double.Parse(parts[6]) : 0;

                Log(LogLevel.DEBUG, $"Order: {action} {quantity} {instrumentName} @ {orderType}");
                if (limitPrice > 0)
                    Log(LogLevel.DEBUG, $"Limit: {limitPrice}");
                if (stopPrice > 0)
                    Log(LogLevel.DEBUG, $"Stop: {stopPrice}");

                Instrument instrument = Instrument.GetInstrument(instrumentName);
                if (instrument == null)
                {
                    Log(LogLevel.ERROR, $"Instrument '{instrumentName}' not found");
                    return "ERROR:Instrument not found";
                }
                
                Log(LogLevel.TRACE, $"Instrument: {instrument.FullName}");

                OrderAction orderAction = action == "BUY" ? OrderAction.Buy : OrderAction.Sell;
                
                // Determine NinjaTrader order type
                OrderType nt8OrderType;
                switch (orderType)
                {
                    case "LIMIT":
                        nt8OrderType = OrderType.Limit;
                        break;
                    case "STOP":
                        nt8OrderType = OrderType.StopMarket;
                        break;
                    case "STOPLIMIT":
                        nt8OrderType = OrderType.StopLimit;
                        break;
                    case "MARKET":
                    default:
                        nt8OrderType = OrderType.Market;
                        break;
                }

                Log(LogLevel.TRACE, $"NT8 OrderType: {nt8OrderType}");

                Order order = currentAccount.CreateOrder(
                    instrument,
                    orderAction,
                    nt8OrderType,
                    OrderEntry.Manual,
                    TimeInForce.Day,
                    quantity,
                    limitPrice,
                    stopPrice,
                    "",
                    "Zorro",
                    DateTime.MaxValue,
                    null
                );

                Log(LogLevel.TRACE, $"Created order: {order.OrderId}");                
                currentAccount.Submit(new[] { order });
                
                Log(LogLevel.INFO, $"ORDER PLACED: {action} {quantity} {instrumentName} @ {orderType} (ID:{order.OrderId})");
                
                activeOrders[order.OrderId] = order;  // Store in dictionary
                orderCount++;
                
                Log(LogLevel.DEBUG, "==== PlaceOrder SUCCESS ====");
                return $"ORDER:{order.OrderId}";
            }
            catch (Exception ex)
            {
                Log(LogLevel.ERROR, $"PlaceOrder failed: {ex.Message}");
                Log(LogLevel.TRACE, $"Stack: {ex.StackTrace}");
                return $"ERROR:{ex.Message}";
            }
        }

        private string HandleCancelOrder(string[] parts)
        {
            if (currentAccount == null)
                return "ERROR:Not logged in";

            if (parts.Length < 2)
                return "ERROR:Order ID required";

            string orderId = parts[1];
            
            if (!activeOrders.ContainsKey(orderId))
                return "ERROR:Order not found";
            
            Order order = activeOrders[orderId];

            currentAccount.Cancel(new[] { order });
            Log(LogLevel.INFO, $"ORDER CANCELLED: {orderId}");
            return $"OK:Order {orderId} cancelled";
        }
        
        private string HandleGetOrderStatus(string[] parts)
        {
            // GETORDERSTATUS:orderId
            // Returns: ORDERSTATUS:orderId:state:filled:avgFillPrice
            
            if (parts.Length < 2)
                return "ERROR:Order ID required";
            
            string orderId = parts[1];
            
            if (!activeOrders.ContainsKey(orderId))
                return "ERROR:Order not found";
            
            Order order = activeOrders[orderId];
            
            // Get order state and fill information
            string state = order.OrderState.ToString();
            int filled = order.Filled;
            double avgFillPrice = order.AverageFillPrice;
            
            Log(LogLevel.TRACE, $"Order {orderId}: State={state} Filled={filled} AvgPrice={avgFillPrice}");
            
            return $"ORDERSTATUS:{orderId}:{state}:{filled}:{avgFillPrice}";
        }
        
        private string HandleSetLogLevel(string[] parts)
        {
            // SETLOGLEVEL:TRACE/DEBUG/INFO/WARN/ERROR
            if (parts.Length < 2)
                return "ERROR:Log level required (TRACE/DEBUG/INFO/WARN/ERROR)";
            
            string levelStr = parts[1].ToUpper();
            
            try
            {
                LogLevel newLevel = (LogLevel)Enum.Parse(typeof(LogLevel), levelStr);
                LogLevel oldLevel = currentLogLevel;
                currentLogLevel = newLevel;
                
                Log(LogLevel.INFO, $"Log level changed: {oldLevel} -> {newLevel}");
                return $"OK:Log level set to {newLevel}";
            }
            catch
            {
                return $"ERROR:Invalid log level '{levelStr}'. Use: TRACE/DEBUG/INFO/WARN/ERROR";
            }
        }
        
        private string HandleGetHistory(string[] parts)
        {
            // GETHISTORY:symbol:startDate:endDate:barMinutes:maxBars
            // Returns: HISTORY:{numBars}|time,open,high,low,close,volume|...
            
            Log(LogLevel.DEBUG, "==== HandleGetHistory START ====");
            Log(LogLevel.TRACE, $"Parts: {string.Join(":", parts)}");
            
            if (currentAccount == null)
            {
                Log(LogLevel.ERROR, "Not logged in");
                return "ERROR:Not logged in";
            }
            
            if (parts.Length < 6)
            {
                Log(LogLevel.ERROR, $"Invalid format (expected 6 parts, got {parts.Length})");
                return "ERROR:Invalid history request format";
            }
            
            try
            {
                string instrumentName = parts[1];
                double startOleDate = double.Parse(parts[2]);
                double endOleDate = double.Parse(parts[3]);
                int barMinutes = int.Parse(parts[4]);
                int maxBars = int.Parse(parts[5]);
                
                Log(LogLevel.DEBUG, $"History request: {instrumentName} {barMinutes}min bars, max {maxBars}");
                
                // Convert OLE dates to DateTime (OLE epoch is Dec 30, 1899)
                DateTime startDate = DateTime.FromOADate(startOleDate);
                DateTime endDate = DateTime.FromOADate(endOleDate);
                
                Log(LogLevel.DEBUG, $"Date range: {startDate} to {endDate}");
                
                // Get instrument
                Instrument instrument = Instrument.GetInstrument(instrumentName);
                if (instrument == null)
                {
                    Log(LogLevel.ERROR, $"Instrument '{instrumentName}' not found");
                    return "ERROR:Instrument not found";
                }
                
                Log(LogLevel.TRACE, $"Instrument: {instrument.FullName}");
                
                // Build bars response synchronously
                StringBuilder barsData = new StringBuilder();
                int barCount = 0;
                bool requestComplete = false;
                string errorMsg = null;
                
                // Create bars request
                BarsRequest barsRequest = new BarsRequest(instrument, startDate, endDate);
                barsRequest.BarsPeriod = new BarsPeriod
                {
                    BarsPeriodType = BarsPeriodType.Minute,
                    Value = barMinutes
                };
                
                // Request bars with callback
                barsRequest.Request((request, errorCode, errorMessage) =>
                {
                    if (errorCode != ErrorCode.NoError)
                    {
                        errorMsg = $"Bars request failed: {errorCode} - {errorMessage}";
                        requestComplete = true;
                        return;
                    }
                    
                    if (request.Bars == null || request.Bars.Count == 0)
                    {
                        Log(LogLevel.WARN, "No bars returned");
                        requestComplete = true;
                        return;
                    }
                    
                    // Return ALL bars available (don't limit artificially)
                    // Zorro will handle chunking if needed
                    Log(LogLevel.DEBUG, $"NT8 returned {request.Bars.Count} bars");
                    
                    for (int i = 0; i < request.Bars.Count; i++)
                    {
                        // Convert time to OLE date
                        double oleTime = request.Bars.GetTime(i).ToOADate();
                        
                        // Format: time,open,high,low,close,volume
                        barsData.Append($"{oleTime},{request.Bars.GetOpen(i)},{request.Bars.GetHigh(i)},{request.Bars.GetLow(i)},{request.Bars.GetClose(i)},{request.Bars.GetVolume(i)}|");
                        barCount++;
                    }
                    
                    requestComplete = true;
                });
                
                // Wait for request to complete (max 30 seconds)
                int timeout = 0;
                while (!requestComplete && timeout < 300)
                {
                    System.Threading.Thread.Sleep(100);
                    timeout++;
                }
                
                if (!requestComplete)
                {
                    Log(LogLevel.ERROR, "Bars request timeout");
                    return "ERROR:Bars request timeout";
                }
                
                if (errorMsg != null)
                {
                    Log(LogLevel.ERROR, errorMsg);
                    return $"ERROR:{errorMsg}";
                }
                
                if (barCount == 0)
                {
                    Log(LogLevel.WARN, "No bars available for requested period");
                    return "HISTORY:0";
                }
                
                // Remove trailing |
                if (barsData.Length > 0)
                    barsData.Length--;
                
                Log(LogLevel.INFO, $"Returning {barCount} bars for {instrumentName}");
                Log(LogLevel.DEBUG, "==== HandleGetHistory SUCCESS ====");                
                return $"HISTORY:{barCount}|{barsData}";
            }
            catch (Exception ex)
            {
                Log(LogLevel.ERROR, $"HandleGetHistory failed: {ex.Message}");
                Log(LogLevel.TRACE, $"Stack: {ex.StackTrace}");
                return $"ERROR:{ex.Message}";
            }
        }
        
        private string HandleGetInstruments()
        {
            // GETINSTRUMENTS
            // Returns: INSTRUMENTS:{count}|symbol,tickSize,pointValue,lastPrice,exchange|...
            
            Log(LogLevel.DEBUG, "==== HandleGetInstruments START ====");            
            try
            {
                StringBuilder instrumentsData = new StringBuilder();
                int count = 0;
                
                // Get all instruments from NinjaTrader
                foreach (Instrument instrument in Instrument.All)
                {
                    try
                    {
                        // Skip instruments without market data
                        if (instrument.MarketData == null)
                            continue;
                        
                        // Get last price to verify instrument is tradeable/has data
                        double lastPrice = 0;
                        if (instrument.MarketData.Last != null)
                            lastPrice = instrument.MarketData.Last.Price;
                        
                        // Skip if no price data (not actively trading)
                        if (lastPrice <= 0)
                            continue;
                        
                        // Get contract specifications
                        double tickSize = instrument.MasterInstrument.TickSize;
                        double pointValue = instrument.MasterInstrument.PointValue;
                        string symbol = instrument.FullName;
                        string instrumentType = instrument.MasterInstrument.InstrumentType.ToString();
                        
                        // Format: symbol,tickSize,pointValue,lastPrice,instrumentType
                        instrumentsData.Append($"{symbol},{tickSize},{pointValue},{lastPrice},{instrumentType}|");
                        count++;
                        
                        Log(LogLevel.TRACE, $"Instrument: {symbol} Price:{lastPrice} Tick:{tickSize} Value:{pointValue} Type:{instrumentType}");
                    }
                    catch (Exception ex)
                    {
                        Log(LogLevel.TRACE, $"Skipping instrument {instrument.FullName}: {ex.Message}");
                        continue;
                    }
                }
                
                // Remove trailing |
                if (instrumentsData.Length > 0)
                    instrumentsData.Length--;
                
                Log(LogLevel.INFO, $"Returning {count} instruments with market data");
                Log(LogLevel.DEBUG, "==== HandleGetInstruments SUCCESS ====");                
                return $"INSTRUMENTS:{count}|{instrumentsData}";
            }
            catch (Exception ex)
            {
                Log(LogLevel.ERROR, $"HandleGetInstruments failed: {ex.Message}");
                Log(LogLevel.TRACE, $"Stack: {ex.StackTrace}");
                return $"ERROR:{ex.Message}";
            }
        }
    }
}
