using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace SendReceiveCancellation_TCP
{
    class Program
    {
        static async Task Main(string[] args)
        {
            //await DisposeDuringPendingReceive_TCP(true);
            //await DisposeDuringPendingReceive_TCP(false);
            await DisposeDuringPendingReceiveFrom_UDP(true);
        }

        private static async Task DisposeDuringPendingReceiveFrom_UDP(bool receiveMessageFrom)
        {
            using var socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            socket.Bind(new IPEndPoint(IPAddress.Any, 0));

            EndPoint testEndpoint = new IPEndPoint(IPAddress.Parse("1.2.3.4"), 1234);

            void DoReceiveFrom()
            {
                try
                {
                    if (receiveMessageFrom)
                    {
                        var flags = SocketFlags.None;
                        socket.ReceiveMessageFrom(new byte[128], 0, 128, ref flags, ref testEndpoint, out _);
                    }
                    else
                    {
                        socket.ReceiveFrom(new byte[128], ref testEndpoint);
                    }   
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"ABORTED! {ex.GetType().Name} : {ex.Message}");
                }
            }

            Console.WriteLine("Starting operation...");
            Task receiveTask = Task.Run(DoReceiveFrom);
            await Task.Delay(200);

            Console.WriteLine("Disposing...");
            socket.Dispose();

            var timeoutTask = Task.Delay(5000);
            Console.WriteLine("Waiting for operation...");
            if (Task.WhenAny(receiveTask, timeoutTask) == timeoutTask)
            {
                Console.WriteLine("TIMEOUT!!!");
            }
        }

        private static async Task DisposeDuringPendingReceive_TCP(bool disposeOrClose)
        {
            using Socket listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            using Socket client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            listener.Bind(new IPEndPoint(IPAddress.Loopback, 0));
            listener.Listen(1);

            var connectTask = client.ConnectAsync(listener.LocalEndPoint);

            using Socket server = listener.Accept();
            await connectTask;

            void DoReceive()
            {
                try
                {
                    client.Receive(new byte[128]);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"ABORTED! {ex.GetType().Name} : {ex.Message}");
                }
            }

            Console.WriteLine("Starting....");
            Task receiveTask = Task.Run(DoReceive);
            await Task.Delay(200);
            if (disposeOrClose) client.Dispose();
            else client.Close();

            var timeoutTask = Task.Delay(5000);
            if (Task.WhenAny(receiveTask, timeoutTask) == timeoutTask)
            {
                Console.WriteLine("TIMEOUT!!!");
            }
        }
    }
}