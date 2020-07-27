using System;
using System.Net;
using System.Net.Sockets;

namespace UDP_Sendar
{
    class Program
    {
        static void Main(string[] args)
        {
            using Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            IPEndPoint localEP = new IPEndPoint(IPAddress.Any, 0);
            if (args.Length > 0)
            {
                localEP = IPEndPoint.Parse(args[0]);
            }
            socket.Bind(localEP);

            Console.WriteLine($"LocalEndpoint: {socket.LocalEndPoint}");
            Console.WriteLine("Type dest EP, 'rcv', 'exit'");
            byte[] buffer = new byte[16];
            while (true)
            {
                Console.Write(">");
                string epString = Console.ReadLine().ToLower();
                if (epString == "exit") return;

                if (epString == "rcv")
                {
                    EndPoint bEp = new IPEndPoint(IPAddress.Any, 0);
                    int received = socket.ReceiveFrom(buffer, ref bEp);
                    IPEndPoint ep = (IPEndPoint)bEp;

                    if (received >= 16)
                    {
                        Guid guid = new Guid(buffer);
                        Console.WriteLine($"Received {guid} ({received} bytes) from {ep}.");
                    }
                    else
                    {
                        Console.WriteLine($"Received {received} byte from {ep}.");
                    }

                    continue;
                }

                try
                {
                    IPEndPoint ep = IPEndPoint.Parse(epString);
                    Guid guid = Guid.NewGuid();
                    guid.TryWriteBytes(buffer);
                    int sent = socket.SendTo(buffer, ep);
                    Console.WriteLine($"Sent {sent} bytes of {guid} to {ep}.");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"{ex.GetType()} -- {ex.Message}");
                }
            }
        }
    }
}
