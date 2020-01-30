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
            //await SendReceiveSingle(AddressFamily.InterNetworkV6);
        }

        private const int Port0 = 25010;
        private const int Port1 = 25011;
        private const int Port2 = 25012;
        private const int Port3 = 25013;

        class SocketNode : IDisposable
        {
            private IPAddress _address;
            
            public SocketNode(string name, AddressFamily addressFamily, int port, bool bind = true)
            {
                Name = name;
                AddressFamily = addressFamily;
                Port = port;
                _address = addressFamily == AddressFamily.InterNetwork
                    ? IPAddress.Loopback
                    : IPAddress.IPv6Loopback;
                Socket = new Socket(addressFamily, SocketType.Dgram, ProtocolType.Udp);
                Data = Guid.NewGuid().ToByteArray();
                EndPoint = MakeEndpoint(port);
                
                if (bind) Bind();
            }

            public IPEndPoint MakeEndpoint(int port) => new IPEndPoint(_address, port);

            public string Name { get; }
            public AddressFamily AddressFamily { get; }
            public int Port { get; }
            public Socket Socket { get; }
            public ArraySegment<byte> Data { get; }
            
            public EndPoint EndPoint { get; }

            public void Bind() => Socket.Bind(EndPoint);

            public void Dispose() => Socket?.Dispose();

            public Task<int> SendToAsync(EndPoint remote)
            {
                Console.WriteLine($"{this}  ==S2==> {remote} ...");
                return Socket.SendToAsync(Data, SocketFlags.None, remote);
            }

            public Task<SocketReceiveFromResult> ReceiveFromAsync(EndPoint remote)
            {
                Console.WriteLine($"{this} <==RF==  {remote} ...");
                return Socket.ReceiveFromAsync(Data, SocketFlags.None, remote);
            }

            public EndPoint CreateEndPointSnapshot() => EndPoint.Create(EndPoint.Serialize());

            public override string ToString() => $"sock[{Name}]@{EndPoint}]";
        }
        
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