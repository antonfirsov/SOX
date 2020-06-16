using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace DelayedAck_Server
{
    class Program
    {
        public const int N = 100000;
        
        static async Task Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("Address please!");
                return;
            }
            
            IPEndPoint serverEndPoint = IPEndPoint.Parse(args[0]);
            
            using Socket listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            listener.Bind(serverEndPoint);
            listener.Listen(10);

            Console.WriteLine("Awaiting connection ...");
            using Socket server = await listener.AcceptAsync();
            Console.WriteLine("OK. Testing ...");

            byte[] buffer = new byte[1];
            while(true)
            {
                int bytesReceived = await server.ReceiveAsync(buffer, SocketFlags.None);
                if (bytesReceived != 1) throw new Exception("Meh");
                await server.SendAsync(buffer, SocketFlags.None);
            }
        }
    }
}