using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace UDP_SendTo_ReceiveFrom
{

    public class Program
    {
        public static async Task Main(string[] args)
        {
            SynchronizationContext.SetSynchronizationContext(new AsyncTestSyncContext(SynchronizationContext.Current));

            await SendToReceiveFromUnixStressRepro.Run();

            await Task.CompletedTask;
            //await SendReceiveSingle(AddressFamily.InterNetwork);
            //await SendReceiveSingle(AddressFamily.InterNetworkV6);
        }


        private const int Port0 = 25010;
        private const int Port1 = 25011;
        private const int Port2 = 25012;
        private const int Port3 = 25013;

        private static async Task SendReceiveSingle(AddressFamily addressFamily)
        {
            // using var a = new SocketNode("a", addressFamily, Port0, false);
            // using var b = new SocketNode("b", addressFamily, Port1, false);
            
            using var a = new Socket(addressFamily, SocketType.Dgram, ProtocolType.Udp);
            a.Bind(new IPEndPoint(IPAddress.Loopback, Port0));
            
            using var b = new Socket(addressFamily, SocketType.Dgram, ProtocolType.Udp);
            b.Bind(new IPEndPoint(IPAddress.Loopback, Port1));

            await a.SendToAsync(new byte[] {1,2,3}, SocketFlags.None, new IPEndPoint(IPAddress.Loopback, Port1));

            byte[] buffer = new byte[128];

            var lol = new IPEndPoint(IPAddress.Loopback, Port0);
            
            var bResult = await b.ReceiveFromAsync(buffer, SocketFlags.None, lol);

            Console.WriteLine(lol.ToString());
            Console.WriteLine("b received shit from: " + bResult.RemoteEndPoint);
            Console.WriteLine("b received shit from: " + buffer[0]);

            Console.WriteLine(ReferenceEquals(lol, bResult.RemoteEndPoint));

            Console.WriteLine("Cool!");
        }
    }
}