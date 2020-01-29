using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace UDP_SendTo_ReceiveFrom
{
    static class Utils
    {
        public static bool SequenceEquals<T>(this ArraySegment<T> left, ArraySegment<T> right)
        {
            if (left == null || right == null) throw new ArgumentNullException();
            
            if (left.Count != right.Count) return false;

            for (int i = 0; i < left.Count; i++)
            {
                if (!left[i].Equals(right[i])) return false;
            }

            return true;
        }

        public static void MustBeSequenceEqualTo<T>(this ArraySegment<T> left, ArraySegment<T> right)
        {
            if (!left.SequenceEquals(right)) throw new Exception("Segments do not match!");
        }
    }
    
    class Program
    {
        static async Task Main(string[] args)
        {
            await SendReceiveSingle(AddressFamily.InterNetwork);
            await SendReceiveSingle(AddressFamily.InterNetworkV6);
        }

        private const int Port0 = 25010;
        private const int Port1 = 25011;
        private const int Port2 = 25012;
        private const int Port3 = 25013;

        class SocketNode : IDisposable
        {
            private IPAddress _address;
            
            public SocketNode(AddressFamily addressFamily, int port)
            {
                AddressFamily = addressFamily;
                Port = port;
                _address = addressFamily == AddressFamily.InterNetwork
                    ? IPAddress.Loopback
                    : IPAddress.IPv6Loopback;
                Socket = new Socket(addressFamily, SocketType.Dgram, ProtocolType.Udp);
                Data = Guid.NewGuid().ToByteArray();
                EndPoint = new IPEndPoint(_address, port);
            }

            public AddressFamily AddressFamily { get; }
            public int Port { get; }
            public Socket Socket { get; }
            public ArraySegment<byte> Data { get; }
            
            public EndPoint EndPoint { get; }

            public void Bind() => Socket.Bind(EndPoint);

            public void Dispose() => Socket?.Dispose();

            public Task<int> SendToAsync(EndPoint remote)
            {
                return Socket.SendToAsync(Data, SocketFlags.None, remote);
            }

            public Task<SocketReceiveFromResult> ReceiveFromAsync(EndPoint remote)
            {
                return Socket.ReceiveFromAsync(Data, SocketFlags.None, remote);
            }
        }
        
        private static async Task SendReceiveSingle(AddressFamily addressFamily)
        {
            using var sndA = new SocketNode(addressFamily, Port0);
            using var rcvA = new SocketNode(addressFamily, Port1);
            
            sndA.Bind();
            rcvA.Bind();

            await sndA.SendToAsync(rcvA.EndPoint);

            var result = await rcvA.ReceiveFromAsync(sndA.EndPoint);

            rcvA.Data.MustBeSequenceEqualTo(sndA.Data);

            Console.WriteLine("Cool!");
        }
    }
}