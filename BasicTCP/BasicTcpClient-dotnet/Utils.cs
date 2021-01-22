using System;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace Client
{
    internal static class Utils
    {
        const int SOL_SOCKET = 0xffff;
        const int SO_KEEPALIVE = 0x0008;

        private static unsafe void QueryKeepalive(Socket socket)
        {
            int enabled = -1;
            void* pEnabled = &enabled;
            socket.GetRawSocketOption(SOL_SOCKET, SO_KEEPALIVE, new Span<byte>(pEnabled, sizeof(int)));

            Console.WriteLine("Enabled: "+enabled);
        }

        public static unsafe void EnableKeepaliveWindows(this Socket socket, uint time = 2000, uint interval = 500)
        {
            QueryKeepalive(socket);

            tcp_keepalive keepalive = new tcp_keepalive
            {
                onoff = 1,
                keepalivetime = time,
                keepaliveinterval = interval
            };
            void* pKeepalive = &keepalive;

            byte[] bytes = new Span<byte>(pKeepalive, sizeof(tcp_keepalive)).ToArray();
            byte[] outVals = new byte[bytes.Length];
            int retVal = socket.IOControl(IOControlCode.KeepAliveValues, bytes, outVals);
            if (retVal != 0)
            {
                throw new Exception("IOControl(IOControlCode.KeepAliveValues) != 0");
            }

            QueryKeepalive(socket);
        }

        struct tcp_keepalive
        {
            public UInt32 onoff;
            public UInt32 keepalivetime;
            public UInt32 keepaliveinterval;
        };
    }
}
