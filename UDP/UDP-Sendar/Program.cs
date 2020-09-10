using System;
using System.Net;
using System.Net.Sockets;

namespace UDP_Sendar
{
    class Program
    {
        static void Main(string[] args)
        {
            DualModeRepro();
            // DoSendarStuff(args);
        }

        private static void DualModeRepro()
        {
            Console.WriteLine("UDP Duplicate Bind Test");

            var socketAny = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            
            Console.WriteLine($"ExclusiveAddressUse: {socketAny.ExclusiveAddressUse} ReuseAddress:{socketAny.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress)}");

            socketAny.Bind(new IPEndPoint(IPAddress.Any, 0));
            IPEndPoint anyEP = socketAny.LocalEndPoint as IPEndPoint;

            Console.WriteLine($"Socket successfully bound on {anyEP}");

            var socketIP6Any = new Socket(AddressFamily.InterNetworkV6, SocketType.Dgram, ProtocolType.Udp);

            Console.WriteLine($"ExclusiveAddressUse: {socketIP6Any.ExclusiveAddressUse} ReuseAddress:{socketIP6Any.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress)}");

            socketIP6Any.DualMode = true;
            socketIP6Any.Bind(new IPEndPoint(IPAddress.IPv6Any, anyEP.Port));
            IPEndPoint ip6AnyEP = socketIP6Any.LocalEndPoint as IPEndPoint;

            Console.WriteLine($"Socket successfully bound on {ip6AnyEP}, is dual mode {socketIP6Any.DualMode}");

            Console.ReadLine();
        }

        private static void DoSendarStuff(string[] args)
        {
            Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
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
