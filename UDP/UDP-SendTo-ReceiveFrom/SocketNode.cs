using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace UDP_SendTo_ReceiveFrom
{
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
}