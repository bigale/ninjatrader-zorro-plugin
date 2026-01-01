// ZorroATI.cs - NinjaTrader 8.1 AddOn for Zorro Integration
// Place this file in: C:\Users\bigal\Documents\NinjaTrader 8\bin\Custom\AddOns\
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
    public class ZorroATI : AddOnBase
    {
        private TcpListener tcpListener;
        private Thread listenerThread;
        private bool isRunning = false;
        private const int PORT = 8888;
        
        private Account currentAccount;
        private Dictionary<string, Instrument> subscribedInstruments = new Dictionary<string, Instrument>();
        private List<Order> activeOrders = new List<Order>();

        protected override void OnStateChange()
        {
            if (State == State.SetDefaults)
            {
                Name = "Zorro ATI Bridge";
                Description = "TCP bridge for Zorro trading platform - NT8 8.1+ compatible";
            }
            else if (State == State.Configure)
            {
                // Start the TCP server as soon as the AddOn is configured
                // This happens at NinjaTrader startup, no need to wait for Realtime
                LogMessage("Zorro ATI Bridge starting...");
                StartTcpServer();
            }
            else if (State == State.Terminated)
            {
                LogMessage("Zorro ATI Bridge stopping...");
                StopTcpServer();
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
                
                LogMessage($"Zorro ATI Bridge listening on port {PORT}");
            }
            catch (Exception ex)
            {
                LogMessage($"Error starting TCP server: {ex.Message}");
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
                    Thread clientThread = new Thread(HandleClient);
                    clientThread.IsBackground = true;
                    clientThread.Start(client);
                }
                catch (SocketException)
                {
                    // Server stopped
                    break;
                }
                catch (Exception ex)
                {
                    LogMessage($"Error accepting client: {ex.Message}");
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
                        string response = ProcessCommand(request);
                        
                        byte[] responseBytes = Encoding.UTF8.GetBytes(response + "\n");
                        stream.Write(responseBytes, 0, responseBytes.Length);
                        stream.Flush();
                    }
                }
            }
            catch (Exception ex)
            {
                LogMessage($"Error handling client: {ex.Message}");
            }
            finally
            {
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
                return $"ERROR:Account '{accountName}' not found";

            LogMessage($"Logged in to account: {accountName}");
            return $"OK:Logged in to {accountName}";
        }

        private string HandleLogout()
        {
            if (currentAccount != null)
            {
                LogMessage($"Logged out from account: {currentAccount.Name}");
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
            {
                Print($"[Zorro ATI] Converted symbol: {zorroSymbol} -> {nt8Symbol}");
            }
            
            Instrument instrument = Instrument.GetInstrument(nt8Symbol);
            if (instrument == null)
            {
                Print($"[Zorro ATI] ERROR: Instrument.GetInstrument returned null for '{nt8Symbol}'");
                Print($"[Zorro ATI] NinjaTrader does not recognize this instrument");
                Print($"[Zorro ATI] Make sure the instrument exists and is spelled correctly in NT8");
                return $"ERROR:Instrument '{nt8Symbol}' not found in NinjaTrader";
            }

            Print($"[Zorro ATI] Instrument found: {instrument.FullName}");
            
            // Check if market data is available
            if (instrument.MarketData == null)
            {
                Print($"[Zorro ATI] WARNING: MarketData is null for {nt8Symbol}");
                Print($"[Zorro ATI] You may need to connect to a data feed in NinjaTrader");
            }
            else
            {
                Print($"[Zorro ATI] MarketData available for {nt8Symbol}");
            }

            subscribedInstruments[zorroSymbol] = instrument;
            Print($"[Zorro ATI] Subscribed to {nt8Symbol}");
            
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
            
            Print($"[Zorro ATI] HandleGetPrice called for: {zorroSymbol}");
            
            if (!subscribedInstruments.ContainsKey(zorroSymbol))
            {
                Print($"[Zorro ATI] ERROR: Not subscribed to {zorroSymbol}");
                Print($"[Zorro ATI] Subscribed instruments count: {subscribedInstruments.Count}");
                foreach (var key in subscribedInstruments.Keys)
                {
                    Print($"[Zorro ATI] - Subscribed: {key}");
                }
                return "ERROR:Not subscribed to instrument";
            }

            Instrument instrument = subscribedInstruments[zorroSymbol];
            
            if (instrument == null)
            {
                Print($"[Zorro ATI] ERROR: Instrument is null for {zorroSymbol}");
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
                else
                    Print($"[Zorro ATI] WARNING: Last is null");
                    
                if (instrument.MarketData.Bid != null)
                    bid = instrument.MarketData.Bid.Price;
                else
                    Print($"[Zorro ATI] WARNING: Bid is null");
                    
                if (instrument.MarketData.Ask != null)
                    ask = instrument.MarketData.Ask.Price;
                else
                    Print($"[Zorro ATI] WARNING: Ask is null");
                    
                if (instrument.MarketData.DailyVolume != null)
                    volume = instrument.MarketData.DailyVolume.Volume;
                else
                    Print($"[Zorro ATI] WARNING: DailyVolume is null");

                Print($"[Zorro ATI] Returning price - Last:{last} Bid:{bid} Ask:{ask} Vol:{volume}");

                return $"PRICE:{last}:{bid}:{ask}:{volume}";
            }
            catch (Exception ex)
            {
                Print($"[Zorro ATI] ERROR getting price data: {ex.Message}");
                Print($"[Zorro ATI] Stack trace: {ex.StackTrace}");
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
            // PLACEORDER:BUY/SELL:INSTRUMENT:QUANTITY:MARKET/LIMIT:PRICE
            if (currentAccount == null)
                return "ERROR:Not logged in";

            if (parts.Length < 5)
                return "ERROR:Invalid order format";

            try
            {
                string action = parts[1].ToUpper();
                string zorroSymbol = parts[2];
                int quantity = int.Parse(parts[3]);
                string orderType = parts[4].ToUpper();
                double limitPrice = parts.Length > 5 ? double.Parse(parts[5]) : 0;

                string nt8Symbol = ConvertToNT8Symbol(zorroSymbol);

                Instrument instrument = Instrument.GetInstrument(nt8Symbol);
                if (instrument == null)
                    return "ERROR:Instrument not found";

                // CreateOrder with correct parameter count for NT8 8.1
                Order order = currentAccount.CreateOrder(
                    instrument,
                    action == "BUY" ? OrderAction.Buy : OrderAction.Sell,
                    orderType == "LIMIT" ? OrderType.Limit : OrderType.Market,
                    OrderEntry.Manual,
                    TimeInForce.Day,
                    quantity,
                    limitPrice,
                    0,  // stop price
                    "",  // OCO
                    "Zorro",  // order name
                    DateTime.MaxValue,  // GTD time
                    null  // on order update
                );

                currentAccount.Submit(new[] { order });
                activeOrders.Add(order);

                return $"ORDER:{order.OrderId}";
            }
            catch (Exception ex)
            {
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
            return $"OK:Order {orderId} cancelled";
        }

        private void LogMessage(string message)
        {
            // NT8 8.1 uses Print instead of Output.Process
            Print($"[Zorro ATI] {message}");
        }
    }
}
