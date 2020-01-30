using System;
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
            return ((IPEndPoint)socket.LocalEndPoint).Port;
        }
    }
    
    public class TestRepro 
    {
        private ITestOutputHelper _output;
        private SocketHelperBase _helper;

        public TestRepro(ITestOutputHelper output, SocketHelperBase helper)
        {
            _output = output;
            _helper = helper;
        }

        public async Task SendToRecvFrom_Datagram_UDP(IPAddress loopbackAddress, int datagramSize, int datagramsToSend)
        {
            IPAddress leftAddress = loopbackAddress, rightAddress = loopbackAddress;

            const int AckTimeout = 10000;
            const int TestTimeout = 30000;

            var left = new Socket(leftAddress.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            left.BindToAnonymousPort(leftAddress);

            var right = new Socket(rightAddress.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            right.BindToAnonymousPort(rightAddress);

            var leftEndpoint = (IPEndPoint)left.LocalEndPoint;
            var rightEndpoint = (IPEndPoint)right.LocalEndPoint;

            var receiverAck = new SemaphoreSlim(0);
            var senderAck = new SemaphoreSlim(0);

            _output.WriteLine($"{DateTime.Now}: Sending data from {rightEndpoint} to {leftEndpoint}");

            Task leftThread = Task.Run(async () =>
            {
                using (left)
                {
                    EndPoint remote = leftEndpoint.Create(leftEndpoint.Serialize());
                    var recvBuffer = new byte[datagramSize];
                    for (int i = 0; i < datagramsToSend; i++)
                    {
                        SocketReceiveFromResult result = await _helper.ReceiveFromAsync(
                            left, new ArraySegment<byte>(recvBuffer), remote);

                        receiverAck.Release();
                        bool gotAck = await senderAck.WaitAsync(TestTimeout);
                        Console.WriteLine("gotAck: "+gotAck);
                    }
                }
            });

            using (right)
            {
                var random = new Random();
                var sendBuffer = new byte[datagramSize];
                for (int i = 0; i < datagramsToSend; i++)
                {
                    random.NextBytes(sendBuffer);
                    sendBuffer[0] = (byte)i;

                    int sent = await _helper.SendToAsync(right, new ArraySegment<byte>(sendBuffer), leftEndpoint);

                    bool gotAck = await receiverAck.WaitAsync(AckTimeout);
                    Console.WriteLine("GotAck: "+gotAck);
                    
                    senderAck.Release();
                }
            }

            await leftThread;
        }
    }
}