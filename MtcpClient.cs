using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace SimpleTcp
{
    public class TcpClient : IDisposable
    {
        public Func<bool> Connected = null;
        public Func<bool> Disconnected = null;
        public Func<byte[], bool> DataReceived = null;
        public bool ConsoleLogging { get; set; }

        private bool _Disposed = false;

        private int _ReceiveBufferSize;

        private string _ServerIp;
        private IPAddress _IPAddress;
        private int _Port;

        private System.Net.Sockets.TcpClient _TcpClient;


        private readonly object _SendLock;

        private bool _Connected = false;

        private CancellationTokenSource _TokenSource;
        private CancellationToken _Token;

        public TcpClient(string serverIp, int port)
        {
            if (String.IsNullOrEmpty(serverIp)) throw new ArgumentNullException(nameof(serverIp));
            if (port < 0) throw new ArgumentException("Port must be zero or greater.");

            _ReceiveBufferSize = 4096;

            _ServerIp = serverIp;
            _IPAddress = IPAddress.Parse(_ServerIp);
            _Port = port;
            _TcpClient = new System.Net.Sockets.TcpClient();

            _SendLock = new object();

            ConsoleLogging = false;
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        public void Connect()
        {
            IAsyncResult ar = _TcpClient.BeginConnect(_ServerIp, _Port, null, null);
            WaitHandle wh = ar.AsyncWaitHandle;

            try
            {
                if (!ar.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(5), false))
                {
                    _TcpClient.Close();
                    throw new TimeoutException("Timeout connecting to " + _ServerIp + ":" + _Port);
                }

                _TcpClient.EndConnect(ar);
                _Connected = true;
            }
            catch (Exception)
            {
                throw;
            }
            finally
            {
                wh.Close();
            }

            if (Connected != null)
            {
                Task.Run(() => Connected());
            }

            _TokenSource = new CancellationTokenSource();
            _Token = _TokenSource.Token;
            Task.Run(async () => await DataReceiver(_Token), _Token);
        }

        public void Send(byte[] data)
        {
            if (data == null || data.Length < 1) throw new ArgumentNullException(nameof(data));
            if (!_Connected) throw new IOException("Not connected to the server; use Connect() first.");

            lock (_SendLock)
            {
                NetworkStream ns = _TcpClient.GetStream();
                ns.Write(data, 0, data.Length);
                ns.Flush();
            }
        }

        protected virtual void Dispose(bool disposing)
        {
            if (_Disposed)
            {
                return;
            }

            if (disposing)
            {
                if (_TcpClient != null)
                {
                    if (_TcpClient.Connected)
                    {
                        NetworkStream ns = _TcpClient.GetStream();
                        if (ns != null)
                        {
                            ns.Close();
                        }
                    }

                    _TcpClient.Close();
                }

                _TokenSource.Cancel();
                _TokenSource.Dispose();

                _Connected = false;
            }

            _Disposed = true;
        }
        private async Task DataReceiver(CancellationToken? cancelToken = null)
        {
            try
            {
                while (true)
                {
                    cancelToken?.ThrowIfCancellationRequested();

                    if (_TcpClient == null)
                    {
                        //  Log("*** DataReceiver null TCP interface detected, disconnection or close assumed");
                        break;
                    }

                    if (!_TcpClient.Connected)
                    {
                        //  Log("*** DataReceiver server disconnected");
                        break;
                    }

                    byte[] data = await DataReadAsync();
                    if (data == null)
                    {
                        await Task.Delay(30);
                        continue;
                    }

                    if (DataReceived != null)
                    {
                        Task<bool> unawaited = Task.Run(() => DataReceived(data));
                    }
                }

            }
            catch (OperationCanceledException)
            {
                throw; // normal cancellation
            }
            catch (Exception)
            {
                //Log("*** DataReceiver server disconnected");
            }
            finally
            {
                _Connected = false;
                Disconnected?.Invoke();
            }
        }

        private async Task<byte[]> DataReadAsync()
        {
            try
            {
                if (_TcpClient == null)
                {
                    //Log("*** DataReadAsync null client supplied");
                    return null;
                }

                if (!_TcpClient.Connected)
                {
                    //  Log("*** DataReadAsync supplied client is not connected");
                    return null;
                }

                byte[] buffer = new byte[_ReceiveBufferSize];
                int read = 0;

                NetworkStream networkStream = _TcpClient.GetStream();
                if (!networkStream.CanRead && !networkStream.DataAvailable)
                {
                    return null;
                }

                using (MemoryStream ms = new MemoryStream())
                {
                    while (true)
                    {
                        read = await networkStream.ReadAsync(buffer, 0, buffer.Length);

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
                // Log("*** DataReadAsync server disconnected");
                return null;
            }
        }
    }
}
