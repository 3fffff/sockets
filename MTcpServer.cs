using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;

namespace SimpleTcp
{
    public class TcpServer : IDisposable
    {
        public Func<string, bool> ClientConnected = null;

        public Func<string, bool> ClientDisconnected = null;

        public Func<string, byte[], bool> DataReceived = null;

        public bool ConsoleLogging { get; set; }

        private int _ReceiveBufferSize;

        private string _ListenerIp;
        private IPAddress _IPAddress;
        private int _Port;

        private ConcurrentDictionary<string, ClientData> _Clients;

        private TcpListener _Listener;
        private bool _Running;

        private CancellationTokenSource _TokenSource;
        private CancellationToken _Token;

        public TcpServer(string listenerIp, int port)
        {
            if (string.IsNullOrEmpty(listenerIp)) throw new ArgumentNullException(nameof(listenerIp));
            if (port < 0) throw new ArgumentException("Port must be zero or greater.");

            _ReceiveBufferSize = 4096;

            _ListenerIp = listenerIp;
            _IPAddress = IPAddress.Parse(_ListenerIp);
            _Port = port;
            _Running = false;
            _Clients = new ConcurrentDictionary<string, ClientData>();

            _TokenSource = new CancellationTokenSource();
            _Token = _TokenSource.Token;

            ConsoleLogging = false;
        }

        // Dispose of the TCP server.
        public void Dispose()
        {
            _TokenSource.Cancel();
            _TokenSource.Dispose();

            if (_Listener != null && _Listener.Server != null)
            {
                _Listener.Server.Close();
                _Listener.Server.Dispose();
            }

            if (_Clients != null && _Clients.Count > 0)
            {
                foreach (KeyValuePair<string, ClientData> currMetadata in _Clients)
                {
                    currMetadata.Value.Dispose();
                }
            }
            GC.SuppressFinalize(this);
        }

        // Start the TCP server and begin accepting connections.
        public void Start()
        {
            if (_Running) throw new InvalidOperationException("TcpServer is already running.");

            _Listener = new TcpListener(_IPAddress, _Port);
            _Listener.Start();

            _Clients = new ConcurrentDictionary<string, ClientData>();

            Task.Run(() => AcceptConnections(), _Token);
        }

        // Retrieve a list of client IP:port connected to the server.
        public List<string> GetClients()
        {
            List<string> clients = new List<string>(_Clients.Keys);
            return clients;
        }


        // Asynchronously send data to the specified client by IP:port.
        public void Send(string ipPort, byte[] data)
        {
            if (string.IsNullOrEmpty(ipPort)) throw new ArgumentNullException(nameof(ipPort));
            if (data == null || data.Length < 1) throw new ArgumentNullException(nameof(data));

            ClientData client = GetClient(ipPort);

            lock (client.Lock)
            {
                    client.NetworkStream.Write(data, 0, data.Length);
                    client.NetworkStream.Flush();
            }
        }

        private void Log(string msg)
        {
            if (ConsoleLogging) Console.WriteLine(msg);
        }

        private async void AcceptConnections()
        {
            while (!_Token.IsCancellationRequested)
            {
                try
                {
                    TcpClient tcpClient = await _Listener.AcceptTcpClientAsync();
                    string clientIp = tcpClient.Client.RemoteEndPoint.ToString();

                    ClientData clientMetadata = new ClientData(tcpClient);

                    AddClient(clientIp, clientMetadata);
                    Log("FinalizeConnection Starting data receiver [" + clientIp + "]");
                    if (ClientConnected != null) await Task.Run(() => ClientConnected(clientIp));
                    Task unawaited2 = Task.Run(() => DataReceiver(clientMetadata), _Token);
                }
                catch (Exception ex)
                {
                    Log("AcceptConnections exception: " + ex.Message);
                }
            }
        }

        private bool IsClientConnected(ClientData client)
        {
            if (client.TcpClient.Connected)
            {
                if ((client.TcpClient.Client.Poll(0, SelectMode.SelectWrite)) && (!client.TcpClient.Client.Poll(0, SelectMode.SelectError)))
                {
                    byte[] buffer = new byte[1];
                    if (client.TcpClient.Client.Receive(buffer, SocketFlags.Peek) == 0)
                        return false;
                    else
                        return true;
                }
                else return false;
            }
            else return false;
        }

        private async Task DataReceiver(ClientData client)
        {
            try
            {
                while (true)
                {
                    try
                    {
                        if (!IsClientConnected(client))
                            break;
                        byte[] data = await DataReadAsync(client);
                        if (data == null)
                        {
                            // no message available 
                            await Task.Delay(30);
                            continue;
                        }

                        if (DataReceived != null)
                        {
                            Task<bool> unawaited = Task.Run(() => DataReceived(client.TcpClient.Client.RemoteEndPoint.ToString(), data));
                        }
                    }
                    catch (Exception)
                    {
                        break;
                    }
                }
            }
            finally
            {
                RemoveClient(client.TcpClient.Client.RemoteEndPoint.ToString());
                Log("[" + client.TcpClient.Client.RemoteEndPoint.ToString() + "] DataReceiver disconnect detected");
                if (ClientDisconnected != null) await Task.Run(() => ClientDisconnected(client.TcpClient.Client.RemoteEndPoint.ToString()));
                client.Dispose();
            }
        }

        private void AddClient(string ipPort, ClientData client)
        {
            _Clients.TryAdd(ipPort, client);
        }

        private void RemoveClient(string ipPort)
        {
            if (string.IsNullOrEmpty(ipPort)) throw new ArgumentNullException(nameof(ipPort));

            ClientData client;
            _Clients.TryRemove(ipPort, out client);
        }

        private ClientData GetClient(string ipPort)
        {
            ClientData client;
            if (!_Clients.TryGetValue(ipPort, out client))
                throw new KeyNotFoundException("Client IP " + ipPort + " not found.");

            return client;
        }

        private async Task<byte[]> DataReadAsync(ClientData client)
        {
            try
            {
                if (!client.NetworkStream.CanRead) return null;
                if (!client.NetworkStream.DataAvailable) return null;

                byte[] buffer = new byte[_ReceiveBufferSize];
                int read = 0;

                using (MemoryStream ms = new MemoryStream())
                {
                    while (true)
                    {
                         read = await client.NetworkStream.ReadAsync(buffer, 0, buffer.Length);

                         if (read > 0)
                         {
                             ms.Write(buffer, 0, read);
                             return ms.ToArray();
                         }
                    }
                 }
            }
            catch (Exception)
            {
                Log("[" + client.TcpClient.Client.RemoteEndPoint.ToString() + "] DataReadAsync server disconnected");
                return null;
            }
        }
    }
}
