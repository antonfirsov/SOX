using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace Server
{
    class Program
    {
        static async Task Main(string[] args)
        {
            System.Reflection.Assembly assembly = typeof(int).Assembly;
            Console.WriteLine(assembly.ImageRuntimeVersion);
            Console.WriteLine(assembly.FullName);

            IPEndPoint endPoint = GetIpEndpoint(args.Length > 0 ? args[0] : null);
            
            try
            {
                using var server = new TestServer();
                await server.RunAsync(endPoint);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            

            Console.ReadLine();
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
