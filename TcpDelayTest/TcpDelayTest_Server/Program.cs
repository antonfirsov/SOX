using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace TcpDelayTest_Server
{
    class Program
    {
        
        static async Task Main(string[] args)
        {
            using Socket listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            IPEndPoint endpoint = GetIpEndpoint(args.Length > 0 ? args[0] : null);
            
            listener.Bind(endpoint);
            listener.Listen(10);
            Console.WriteLine($"Listening on {endpoint} ...");
            using Socket handler = await listener.AcceptAsync();
            Console.WriteLine("Connected.");

            byte[] buffer = new byte[2048];
            while (true)
            {
                int received = await handler.ReceiveAsync(buffer, SocketFlags.None);
                string str = GetMessageString(buffer, received);
                Console.WriteLine(str);
            }
        }

        private static string GetMessageString(byte[] buffer, int length)
        {
            return string.Create(buffer.Length, buffer, (c, b) =>
            {
                for (int i = 0; i < length; i++)
                {
                    c[i] = (char) ('0' + b[i]);
                }
            });
        }
        
        private static IPEndPoint GetIpEndpoint(string endpointStr)
        {
            IPAddress ipAddress;
            int port = 11000;
            if (endpointStr != null)
            {
                string[] parts = endpointStr.Split(':');
                ipAddress = IPAddress.Parse(parts[0]);
                if (parts.Length > 1)
                {
                    port = int.Parse(parts[1]);
                }
            }
            else
            {
                ipAddress = GetOwInP();
            }
            
            return new IPEndPoint(ipAddress, port);
        }
        
        private static IPAddress GetOwInP()
        {
            string hostName = Dns.GetHostName();
            IPHostEntry ipHostInfo = Dns.GetHostEntry(hostName);
            IPAddress[] addresses = ipHostInfo.AddressList.Where(a => a.AddressFamily == AddressFamily.InterNetwork).ToArray();
            return ipHostInfo.AddressList.First(a => a.AddressFamily == AddressFamily.InterNetwork && a.ToString().Contains("192"));
        }
    }
}