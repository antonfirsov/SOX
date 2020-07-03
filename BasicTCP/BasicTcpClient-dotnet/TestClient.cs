using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Client
{
    internal class TestClient : IDisposable
    {
        private byte[] _buffer = new byte[1024];
        private StringBuilder _bld = new StringBuilder();
        private const int ServerPort = 11011;


        private static IPAddress GetRemoteIP() => IPAddress.Parse("172.17.99.105");

        //private static IPAddress GetRemoteIP() => IPAddress.Parse("fe80::ec50:fb40:898f:3795");

        
        public void Run(IPEndPoint remoteEndpoint)
        {
            Console.WriteLine("********* CLIENT *********");
;
            using Socket sender = new Socket(remoteEndpoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

            Console.WriteLine("Press ENTER to connect ...");
            Console.ReadLine();

            sender.Connect(remoteEndpoint);

            Console.WriteLine($"Connected: {sender.LocalEndPoint} ---> {sender.RemoteEndPoint}");

            while (true)
            {
                Console.Write(">");
                string msg = Console.ReadLine();
                
                byte[] msgBytes = Encoding.ASCII.GetBytes(msg);
                int bytesSent = sender.Send(msgBytes);
                Console.WriteLine($"Sent {bytesSent} bytes.");
                
                if (msg.ToLower() == "exit.") break;

                if (msg.EndsWith("."))
                {
                    int count = sender.Receive(_buffer);
                    string echo = Encoding.ASCII.GetString(_buffer, 0, count);
                    Console.WriteLine("ECHO:" + echo);
                }
            }

            sender.Shutdown(SocketShutdown.Both);
            sender.Close();
        }

        public void Dispose()
        {
            
        }

    }
}
