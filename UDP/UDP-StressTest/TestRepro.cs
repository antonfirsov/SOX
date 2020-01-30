using System;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace UDP_StressTest
{
    public static class Utils
    {
        public static int BindToAnonymousPort(this Socket socket, IPAddress address)
        {
            socket.Bind(new IPEndPoint(address, 0));
            return ((IPEndPoint) socket.LocalEndPoint).Port;
        }
    }

    public class TestRepro
    {
        private const int AckTimeout = 10000;
        private const int TestTimeout = 30000;

        private readonly ITestOutputHelper _output;
        private readonly SocketHelperBase _helper;
        private readonly int _datagramSize;
        private readonly int _datagramsToSend;

        private static readonly IPAddress Address = IPAddress.Loopback;

        public static TestRepro CreateSync(int datagramSize, int datagramsToSend)
        {
            var output = new TestOutputHelper();
            output.Prefix = "[Sync]";
            return new TestRepro(output, new SocketHelperSpanSync(output), datagramSize, datagramsToSend);
        }

        public static TestRepro CreateAsync(int datagramSize, int datagramsToSend)
        {
            var output = new TestOutputHelper();
            output.Prefix = "[Async]";
            return new TestRepro(output, new SocketHelperEap(output), datagramSize, datagramsToSend);
        }

        public TestRepro(ITestOutputHelper output, SocketHelperBase helper, int datagramSize, int datagramsToSend)
        {
            _output = output;
            _helper = helper;
            _datagramSize = datagramSize;
            _datagramsToSend = datagramsToSend;
        }

        private async Task Measure(string msg, Func<Task> action)
        {
            _output.WriteLine($"{msg} ...");
            Stopwatch sw = Stopwatch.StartNew();
            await action();
            sw.Stop();
            _output.WriteLine($"{msg} completed in {sw.ElapsedMilliseconds} ms");
        }

        public async Task RunAsync()
        {
            var left = new Socket(Address.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            left.BindToAnonymousPort(Address);

            var right = new Socket(Address.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            right.BindToAnonymousPort(Address);

            var leftEndpoint = (IPEndPoint) left.LocalEndPoint;
            var rightEndpoint = (IPEndPoint) right.LocalEndPoint;

            var receiverAck = new SemaphoreSlim(0);
            var senderAck = new SemaphoreSlim(0);

            _output.WriteLine($"{DateTime.Now}: Sending data from {rightEndpoint} to {leftEndpoint}");

            Task leftThread = Task.Run(async () =>
            {
                using (left)
                {
                    EndPoint remote = leftEndpoint.Create(leftEndpoint.Serialize());
                    var recvBuffer = new byte[_datagramSize];
                    for (int i = 0; i < _datagramsToSend; i++)
                    {
                        await Measure("Receiving", async () =>
                        {
                            await _helper.ReceiveFromAsync(
                                left, new ArraySegment<byte>(recvBuffer), remote);
                        });
                        
                        receiverAck.Release();
                        await senderAck.WaitAsync(TestTimeout);
                    }
                }
            });

            using (right)
            {
                var random = new Random();
                var sendBuffer = new byte[_datagramSize];
                for (int i = 0; i < _datagramsToSend; i++)
                {
                    random.NextBytes(sendBuffer);
                    sendBuffer[0] = (byte) i;

                    int sent = await _helper.SendToAsync(right, new ArraySegment<byte>(sendBuffer), leftEndpoint);

                    await Measure("Waiting for receiver ack", async () =>
                    {
                        await receiverAck.WaitAsync(AckTimeout);
                    });

                    senderAck.Release();
                }
            }

            await leftThread;
        }
    }
}