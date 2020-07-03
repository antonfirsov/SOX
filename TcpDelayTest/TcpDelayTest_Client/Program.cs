using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace TcpDelayTest_Client
{
    class Program
    {
        static async Task Main(string[] args)
        {
            IPEndPoint ep = GetIpEndpoint(args[0]);
            
            using Socket client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            Console.WriteLine("Connecting ...");
            await client.ConnectAsync(ep);
            Console.WriteLine("Connected.");

            const int delayMs = 10;
            byte[] buffer = new byte[8];
            while (true)
            {
                int burstLength = int.Parse(Console.ReadLine() ?? "10");

                for (int i = 0; i < burstLength; i++)
                {
                    FillPayload(buffer, i);
                    int sent = await client.SendAsync(buffer, SocketFlags.None);
                    if (sent == 0) throw new Exception("wtf");
                    await Task.Delay(delayMs);
                }
            }
        }

        private static void FillPayload(byte[] buffer, int i)
        {
            for (int j = 0; j < buffer.Length; j+=2)
            {
                buffer[j] = (byte) i;
            }
        }
        
        private static IPEndPoint GetIpEndpoint(string endpointStr)
        {
            int port = 11000;
            string[] parts = endpointStr.Split(':');
            var ipAddress = IPAddress.Parse(parts[0]);
            if (parts.Length > 1)
            {
                port = int.Parse(parts[1]);
            }

            return new IPEndPoint(ipAddress, port);
        }
    }
}