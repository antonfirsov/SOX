using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace SendReceiveCancellation_TCP
{
    class Program
    {
        static Task Main(string[] args)
        {
            return Tcp();
        }

        private static async Task Tcp()
        {
            using Socket listener = new Socket(SocketType.Stream, ProtocolType.Tcp);
            listener.SendBufferSize = 0;
            IPEndPoint ep = new IPEndPoint(IPAddress.Loopback, 0);
            listener.Bind(ep);
            listener.Listen(1);
            ep = (IPEndPoint)listener.LocalEndPoint;

            using Socket receiver = new Socket(SocketType.Stream, ProtocolType.Tcp);
            receiver.ReceiveBufferSize = 1000;

            var connectTask = receiver.ConnectAsync(ep);
            var acceptTask = listener.AcceptAsync();

            using Socket sender = await acceptTask;
            await connectTask;

            sender.SendBufferSize = 0;          

            byte[] sendData = new byte[3000];
            byte[] receiveData = new byte[5000];

            var sendTask = sender.SendAsync(sendData, SocketFlags.None);
            var receiveTask = receiver.ReceiveAsync(receiveData, SocketFlags.None);

            int received = await receiveTask;
            Console.WriteLine($"Received: {received}");
            int sent = await sendTask;
            Console.WriteLine($"Sent: {sent}");
        }
    }
}
