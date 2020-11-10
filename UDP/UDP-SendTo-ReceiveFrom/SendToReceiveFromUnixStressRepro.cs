using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Numerics;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace UDP_SendTo_ReceiveFrom
{
    public static class SendToReceiveFromUnixStressRepro
    {
        public static async Task Run()
        {
            Task test = Task.Run(SendToRecvFrom_Datagram_UDP);
            Task burn = BurnCpuAsync(60, 4);

            Task finished = await Task.WhenAny(test, burn);
            Console.WriteLine($"LEFT:\n{_leftOutput}\nRIGHT:\n{_rightOutput}\n");
            if (finished == burn)
            {
                Console.WriteLine("TIMEOUT!");
            }
            else
            {
                await test;
                Console.WriteLine("SUCCESS!");
            }
        }

        private static StringBuilder _leftOutput = new StringBuilder();
        private static StringBuilder _rightOutput = new StringBuilder();

        private static async ValueTask MeasureAsync(string operationName, StringBuilder output, Func<Task> action)
        {
            Stopwatch sw = Stopwatch.StartNew();
            output.Append($"{operationName} ... ");
            await action();
            output.AppendLine($"completed in {sw.ElapsedMilliseconds} ms");
        }

        public static async Task SendToRecvFrom_Datagram_UDP()
        {
            using var measure = new Measure(nameof(SendToRecvFrom_Datagram_UDP));

            IPAddress leftAddress = IPAddress.Loopback;
            IPAddress rightAddress = IPAddress.Loopback;

            const int DatagramSize = 256;
            const int DatagramsToSend = 100;
            const int AckTimeout = 2000;
            const int TestTimeout = 10000;

            using var left = new Socket(leftAddress.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            using var right = new Socket(rightAddress.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            left.Bind(new IPEndPoint(leftAddress, 0));
            right.Bind(new IPEndPoint(rightAddress, 0));

            var leftEndpoint = (IPEndPoint)left.LocalEndPoint;
            var rightEndpoint = (IPEndPoint)right.LocalEndPoint;
            
            var receiverAck = new SemaphoreSlim(0);
            var senderAck = new SemaphoreSlim(0);

            Console.WriteLine($"{DateTime.Now}: Sending data from {rightEndpoint} to {leftEndpoint}");

            static Task<int> ReceiveFromAsync(Socket socket, byte[] buffer, EndPoint ep) 
                => Task.Run(() => socket.ReceiveFrom(buffer, ref ep));

            static Task<int> SendToAsync(Socket socket, byte[] buffer, EndPoint ep)
                => Task.Run(() => socket.SendTo(buffer, ep));

            var receivedChecksums = new uint?[DatagramsToSend];
            Task leftThread = Task.Run(async () =>
            {
                EndPoint remote = leftEndpoint.Create(leftEndpoint.Serialize());
                var recvBuffer = new byte[DatagramSize];
                for (int i = 0; i < DatagramsToSend; i++)
                {
                    var result = await ReceiveFromAsync(
                        left, recvBuffer, remote);
                    
                    int datagramId = recvBuffer[0];
                    receivedChecksums[datagramId] = Fletcher32.Checksum(recvBuffer, 0, result);

                    receiverAck.Release();

                    bool gotAck = false;
                    await MeasureAsync($"senderAck.WaitAsync [{i}]", _leftOutput, async () =>  gotAck = await senderAck.WaitAsync(TestTimeout));

                    if (!gotAck)
                    {
                        Fail($"{DateTime.Now}: Timeout waiting {TestTimeout} for senderAck in iteration {i}");
                    }
                }
            });

            var sentChecksums = new uint[DatagramsToSend];
            using (right)
            {
                var random = new Random();
                var sendBuffer = new byte[DatagramSize];
                for (int i = 0; i < DatagramsToSend; i++)
                {
                    random.NextBytes(sendBuffer);
                    sendBuffer[0] = (byte)i;

                    int sent = await SendToAsync(right, sendBuffer, leftEndpoint);

                    bool gotAck = false;
                    await MeasureAsync($"receiverAck.WaitAsync [{i}]", _rightOutput, async () => gotAck = await receiverAck.WaitAsync(AckTimeout));

                    if (!gotAck)
                    {
                        string msg = $"{DateTime.Now}: Timeout waiting {AckTimeout} for receiverAck in iteration {i} after sending {sent}. Receiver is in {leftThread.Status}";
                        if (leftThread.Status == TaskStatus.Faulted)
                        {
                            try
                            {
                                await leftThread;
                            }
                            catch (Exception ex)
                            {
                                msg += $"{ex.Message}{Environment.NewLine}{ex.StackTrace}";
                            }
                        }
                        Fail(msg);
                    }

                    senderAck.Release();
                    sentChecksums[i] = Fletcher32.Checksum(sendBuffer, 0, sent);
                }
            }

            await leftThread;
        }

        private static void Fail(string message)
        {
            throw new Exception(message);
        }

        public static void BurnCpu(int seconds, int parallelism)
        {
            TimeSpan dt = TimeSpan.FromSeconds(seconds);
            DateTime end = DateTime.Now + dt;

            while (DateTime.Now < end)
            {
                Parallel.For(0, parallelism, i => _ = FindPrimeNumber(500 + i));
            }
        }

        public static Task BurnCpuAsync(int seconds, int parallelism) => Task.Run(() => BurnCpu(seconds, parallelism));

        public static long FindPrimeNumber(int n)
        {
            int count = 0;
            long a = 2;
            while (count < n)
            {
                long b = 2;
                int prime = 1;// to check if found a prime
                while (b * b <= a)
                {
                    if (a % b == 0)
                    {
                        prime = 0;
                        break;
                    }
                    b++;
                }
                if (prime > 0)
                {
                    count++;
                }
                a++;
            }
            return (--a);
        }

        internal struct Fletcher32
        {
            private ushort _sum1, _sum2;
            private byte? _leftover;

            public uint Sum
            {
                get
                {
                    ushort s1 = _sum1, s2 = _sum2;
                    if (_leftover != null)
                    {
                        Add((byte)_leftover, 0, ref s1, ref s2);
                    }

                    return (uint)s1 << 16 | (uint)s2;
                }
            }

            private static void Add(byte lo, byte hi, ref ushort sum1, ref ushort sum2)
            {
                ushort word = (ushort)((ushort)lo << 8 | (ushort)hi);
                sum1 = (ushort)((sum1 + word) % 65535);
                sum2 = (ushort)((sum2 + sum1) % 65535);
            }

            public void Add(byte[] bytes, int offset, int count)
            {
                int numBytes = count;
                if (numBytes == 0)
                {
                    return;
                }

                int i = offset;
                if (_leftover != null)
                {
                    Add((byte)_leftover, bytes[i], ref _sum1, ref _sum2);
                    i++;
                    numBytes--;
                }

                int words = numBytes / 2;
                for (int w = 0; w < words; w++, i += 2)
                {
                    Add(bytes[i], bytes[i + 1], ref _sum1, ref _sum2);
                }

                _leftover = numBytes % 2 != 0 ? ((byte?)bytes[i]) : null;
            }

            public static uint Checksum(byte[] bytes, int offset, int count)
            {
                var fletcher32 = new Fletcher32();
                fletcher32.Add(bytes, offset, count);
                return fletcher32.Sum;
            }
        }
    }
}