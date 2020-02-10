using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    internal class TestServer : IDisposable
    {
        private byte[] _buffer = new byte[1024];
        private StringBuilder _bld = new StringBuilder();

        private const int ServerPort = 11000;

        private static IPAddress GetOwnIP()
        {
            string hostName = Dns.GetHostName();
            IPHostEntry ipHostInfo = Dns.GetHostEntry(hostName);
            IPAddress[] addresses = ipHostInfo.AddressList.Where(a => a.AddressFamily == AddressFamily.InterNetwork).ToArray();
            return ipHostInfo.AddressList.First(a => a.AddressFamily == AddressFamily.InterNetwork && a.ToString().Contains("172"));
        }

        public async Task RunAsync()
        {
            Console.WriteLine("********* SERVER *********");

            IPAddress ipAddress = GetOwnIP();

            IPEndPoint localEndPoint = new IPEndPoint(ipAddress, ServerPort);

            using Socket listener = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            listener.Bind(localEndPoint);
            listener.Listen(10);
            Console.WriteLine($"Server is listening on {localEndPoint}.");
            
            ArraySegment<byte> bufferSegment = new ArraySegment<byte>(_buffer);

            while (true)
            {
                Console.WriteLine("Waiting for conn ..");
                Socket handler = await listener.AcceptAsync();
                Console.WriteLine($"ESTABLISHED: {handler.RemoteEndPoint} <------ {handler.LocalEndPoint}");

                _bld.Clear();
                while (true)
                {
                    int count = await handler.ReceiveAsync(bufferSegment, SocketFlags.None);
                    string part = Encoding.ASCII.GetString(_buffer, 0, count);
                    _bld.Append(part);
                    if (part.EndsWith("."))
                    {
                        string recMsg = _bld.ToString().Substring(0, _bld.Length - 1);
                        Console.WriteLine("RECEIVED:" + recMsg);

                        if (recMsg.ToLower() == "exit") break;

                        byte[] echo = Encoding.ASCII.GetBytes(recMsg + "!!!");

                        await handler.SendAsync(new ArraySegment<byte>(echo), SocketFlags.None);
                        _bld.Clear();
                    }
                }
                
                handler.Shutdown(SocketShutdown.Both);
                handler.Close();
            }
        }

        public void Dispose()
        {
            
        }
    }
}
