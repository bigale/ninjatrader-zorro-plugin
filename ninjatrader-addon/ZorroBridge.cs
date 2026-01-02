// ZorroBridge.cs - NinjaTrader 8.1 AddOn for Zorro Integration
// Place this file in: Documents\NinjaTrader 8\bin\Custom\AddOns\
//
// This AddOn creates a TCP server that allows Zorro to communicate with NinjaTrader 8.1+
// Compile: Tools -> Compile -> Compile All (or F5) in NinjaTrader

#region Using declarations
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using NinjaTrader.Cbi;
using NinjaTrader.NinjaScript;
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
        private Dictionary<string, Instrument> subscribedInstruments = new Dictionary<string, Instrument>();
        private List<Order> activeOrders = new List<Order>();
        
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

            string zorroSymbol = parts[1];
            string nt8Symbol = ConvertToNT8Symbol(zorroSymbol);
            
            if (nt8Symbol != zorroSymbol)
                Log(LogLevel.DEBUG, $"Converted: {zorroSymbol} -> {nt8Symbol}");
            
            Instrument instrument = Instrument.GetInstrument(nt8Symbol);
            if (instrument == null)
            {
                Log(LogLevel.ERROR, $"Instrument '{nt8Symbol}' not found");
                return $"ERROR:Instrument '{nt8Symbol}' not found in NinjaTrader";
            }

            Log(LogLevel.TRACE, $"Found: {instrument.FullName}");
            
            if (instrument.MarketData == null)
            {
                Log(LogLevel.WARN, $"MarketData is null for {nt8Symbol} - connect to data feed");
            }

            subscribedInstruments[zorroSymbol] = instrument;
            Log(LogLevel.INFO, $"Subscribed to {nt8Symbol}");
            
            return $"OK:Subscribed to {nt8Symbol}";
        }
        
        private string ConvertMonthCode(char code)
        {
            // CME month codes to month numbers
            switch (char.ToUpper(code))
            {
                case 'F': return "01"; // January
                case 'G': return "02"; // February
                case 'H': return "03"; // March
                case 'J': return "04"; // April
                case 'K': return "05"; // May
                case 'M': return "06"; // June
                case 'N': return "07"; // July
                case 'Q': return "08"; // August
                case 'U': return "09"; // September
                case 'V': return "10"; // October
                case 'X': return "11"; // November
                case 'Z': return "12"; // December
                default: return null;
            }
        }
        
        private string ConvertToNT8Symbol(string zorroSymbol)
        {
            // Convert various Zorro symbol formats to NinjaTrader format
            // Handles: "MES 0326", "MESH26" -> "MES 03-26"
            
            if (zorroSymbol.Contains("-"))
                return zorroSymbol; // Already in NT8 format
            
            if (zorroSymbol.Contains(" ") && !zorroSymbol.Contains("-"))
            {
                // Format: "MES 0326" with space
                string[] symbolParts = zorroSymbol.Split(' ');
                if (symbolParts.Length == 2 && symbolParts[1].Length == 4)
                {
                    string month = symbolParts[1].Substring(0, 2);
                    string year = symbolParts[1].Substring(2, 2);
                    return $"{symbolParts[0]} {month}-{year}";
                }
            }
            else if (!zorroSymbol.Contains(" ") && zorroSymbol.Length >= 6)
            {
                // Format: "MESH26" (symbol + month letter + year)
                string symbol = zorroSymbol.Substring(0, zorroSymbol.Length - 3);
                char monthCode = zorroSymbol[zorroSymbol.Length - 3];
                string year = zorroSymbol.Substring(zorroSymbol.Length - 2);
                
                string month = ConvertMonthCode(monthCode);
                if (month != null)
                {
                    return $"{symbol} {month}-{year}";
                }
            }
            
            return zorroSymbol; // Return as-is if no conversion needed
        }

        private string HandleUnsubscribe(string[] parts)
        {
            if (parts.Length < 2)
                return "ERROR:Instrument name required";

            string instrumentName = parts[1];
            subscribedInstruments.Remove(instrumentName);
            
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

            return $"ACCOUNT:{cashValue}:{buyingPower}:{realizedPnL}";
        }

        private string HandleGetPosition(string[] parts)
        {
            if (currentAccount == null)
                return "ERROR:Not logged in";

            if (parts.Length < 2)
                return "ERROR:Instrument name required";

            string zorroSymbol = parts[1];
            string nt8Symbol = ConvertToNT8Symbol(zorroSymbol);
            
            Instrument instrument = Instrument.GetInstrument(nt8Symbol);
            
            if (instrument == null)
                return "ERROR:Instrument not found";

            // Find position by iterating through positions
            int position = 0;
            double avgPrice = 0;
            
            foreach (Position pos in currentAccount.Positions)
            {
                if (pos.Instrument == instrument)
                {
                    position = pos.Quantity;
                    avgPrice = pos.AveragePrice;
                    break;
                }
            }

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
                string zorroSymbol = parts[2];
                int quantity = int.Parse(parts[3]);
                string orderType = parts[4].ToUpper();
                double limitPrice = parts.Length > 5 ? double.Parse(parts[5]) : 0;
                double stopPrice = parts.Length > 6 ? double.Parse(parts[6]) : 0;

                Log(LogLevel.DEBUG, $"Order: {action} {quantity} {zorroSymbol} @ {orderType}");
                if (limitPrice > 0)
                    Log(LogLevel.DEBUG, $"Limit: {limitPrice}");
                if (stopPrice > 0)
                    Log(LogLevel.DEBUG, $"Stop: {stopPrice}");

                string nt8Symbol = ConvertToNT8Symbol(zorroSymbol);
                if (nt8Symbol != zorroSymbol)
                    Log(LogLevel.TRACE, $"Converted: {zorroSymbol} -> {nt8Symbol}");

                Instrument instrument = Instrument.GetInstrument(nt8Symbol);
                if (instrument == null)
                {
                    Log(LogLevel.ERROR, $"Instrument '{nt8Symbol}' not found");
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
                
                Log(LogLevel.INFO, $"ORDER PLACED: {action} {quantity} {zorroSymbol} @ {orderType} (ID:{order.OrderId})");
                
                activeOrders.Add(order);
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
            Order order = activeOrders.FirstOrDefault(o => o.OrderId == orderId);

            if (order == null)
                return "ERROR:Order not found";

            currentAccount.Cancel(new[] { order });
            Log(LogLevel.INFO, $"ORDER CANCELLED: {orderId}");
            return $"OK:Order {orderId} cancelled";
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
    }
}
