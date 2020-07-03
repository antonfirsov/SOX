using System;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace DelayedAck_Client
{
    internal class Program
    {
        private const int N = 1000000;
        
        public static async Task Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("Address please!");
                return;
            }

            IPEndPoint serverEndPoint = IPEndPoint.Parse(args[0]);
            
            using Socket client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            await client.ConnectAsync(serverEndPoint);
            
            // Warm up:
            byte[] buffer = new byte[1];
            await client.SendAsync(buffer, SocketFlags.None);
            await client.ReceiveAsync(buffer, SocketFlags.None);


            Console.WriteLine("Testing ....");
            Stopwatch stopwatch = Stopwatch.StartNew();
            for (int i = 0; i < N; i++)
            {
                await client.SendAsync(buffer, SocketFlags.None);
                await client.ReceiveAsync(buffer, SocketFlags.None);
            }
            
            stopwatch.Stop();

            TimeSpan averageDelay = stopwatch.Elapsed / (2 * N);
            Console.WriteLine("Delay: " + averageDelay);
        }
    }
}