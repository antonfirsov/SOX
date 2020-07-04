using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
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

            while (true)
            {
                try
                {
                    Console.WriteLine($"Listening on {endpoint} ...");
                    using Socket handler = await listener.AcceptAsync();
                    Console.WriteLine("Connected.");
                    //DisableDelayedAck(handler);
                    

                    byte[] buffer = new byte[2048];
                    while (true)
                    {
                        DisableDelayedAck12(handler);

                        int received = await handler.ReceiveAsync(buffer, SocketFlags.None);
                        string str = GetMessageString(buffer, received);
                        Console.WriteLine(str);
                    }
                }
                catch (SocketException ex)
                {
                    Console.WriteLine(ex.Message);
                    Console.WriteLine(ex.StackTrace);
                }
            }
        }
        
        private const int SIO_TCP_SET_ACK_FREQUENCY = unchecked((int)0x98000017);
        private static readonly byte[] Dummy = new byte[128];

        private static void DisableDelayedAck(Socket socket)
        {
            socket.IOControl(SIO_TCP_SET_ACK_FREQUENCY, BitConverter.GetBytes(1), Dummy);
        }

        private static void DisableDelayedAck12(Socket socket)
        {
            try
            {
                socket.SetRawSocketOption((int) SocketOptionLevel.Tcp, 12, BitConverter.GetBytes(1));
            }
            catch (Exception ex)
            {
                Console.WriteLine("SetRawSocketOption failed: " + ex.Message);
                Console.WriteLine(ex.StackTrace);
            }
            
        }
        
        private static void DisableDelayedAck13(Socket socket)
        {
            socket.SetSocketOption(SocketOptionLevel.Tcp, (SocketOptionName)13, true);
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
    
    internal static partial class Winsock
    {
        // Used with SIOGETEXTENSIONFUNCTIONPOINTER - we're assuming that will never block.
        [DllImport("Ws2_32.dll", SetLastError = true)]
        internal static extern SocketError WSAIoctl(
            SafeSocketHandle socketHandle,
            [In] int ioControlCode,
            [In, Out] ref Guid guid,
            [In] int guidSize,
            [Out] out IntPtr funcPtr,
            [In]  int funcPtrSize,
            [Out] out int bytesTransferred,
            [In] IntPtr shouldBeNull,
            [In] IntPtr shouldBeNull2);

        [DllImport("Ws2_32.dll", SetLastError = true, EntryPoint = "WSAIoctl")]
        internal static extern SocketError WSAIoctl_Blocking(
            SafeSocketHandle socketHandle,
            [In] int ioControlCode,
            [In] byte[] inBuffer,
            [In] int inBufferSize,
            [Out] byte[] outBuffer,
            [In] int outBufferSize,
            [Out] out int bytesTransferred,
            [In] IntPtr overlapped,
            [In] IntPtr completionRoutine);
    }
}