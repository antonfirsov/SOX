using System;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace DuplicateSocket_ListenerServer
{
    internal class ListenerServer : IDisposable
    {
        private byte[] _buffer = new byte[1024];
        private StringBuilder _bld = new StringBuilder();

        private const int ServerPort = 11000;

        private const string HalderProcessName = "DuplicateSocket_HandlerServer";

        private static int GetHandlerProcessId() => Process.GetProcessesByName(HalderProcessName).Single().Id;

        public void Run()
        {
            Console.WriteLine("********* SERVER *********");

            IPHostEntry ipHostInfo = Dns.GetHostEntry("localhost");
            IPAddress ipAddress = ipHostInfo.AddressList.First(a => a.AddressFamily == AddressFamily.InterNetwork);
            IPEndPoint localEndPoint = new IPEndPoint(ipAddress, ServerPort);

            using Socket listener = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            listener.Bind(localEndPoint);
            listener.Listen(10);

            while (true)
            {
                Console.WriteLine("Waiting for conn ..");
                Socket handler = listener.Accept();
                int handlerId = GetHandlerProcessId();
                SocketInformation socketInfo = handler.DuplicateAndClose(handlerId);
                

                Console.WriteLine($"ESTABLISHED: {handler.RemoteEndPoint} <------ {handler.LocalEndPoint}");

                _bld.Clear();
                while (true)
                {
                    int count = handler.Receive(_buffer);
                    string part = Encoding.ASCII.GetString(_buffer, 0, count);
                    _bld.Append(part);
                    if (part.EndsWith("."))
                    {
                        string recMsg = _bld.ToString().Substring(0, _bld.Length - 1);
                        Console.WriteLine("RECEIVED:" + recMsg);

                        if (recMsg.ToLower() == "exit") break;

                        byte[] echo = Encoding.ASCII.GetBytes(recMsg + "!!!");
                        handler.Send(echo);
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
