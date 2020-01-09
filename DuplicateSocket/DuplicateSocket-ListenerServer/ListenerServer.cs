using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Threading.Tasks;

namespace DuplicateSocket_ListenerServer
{
    internal class ListenerServer : IDisposable
    {
        private byte[] _buffer = new byte[1024];
        private StringBuilder _bld = new StringBuilder();

        private const int ServerPort = 11000;

        private const string HalderProcessName = "DuplicateSocket-HandlerServer";

        private static int GetHandlerProcessId() => Process.GetProcessesByName(HalderProcessName).Single().Id;

        private static readonly string LocalAppDataFolder =
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

        private static readonly string SocketInfoFile = Path.Combine(LocalAppDataFolder, "_SocketInfo.json");
        
        private static readonly BinaryFormatter BinaryFormatter = new BinaryFormatter();

        private static void SerializeSocketInfo(SocketInformation socketInfo)
        {
            using (MemoryStream ms = new MemoryStream())
            {
                BinaryFormatter.Serialize(ms, socketInfo);
                byte[] stuff = ms.ToArray();
                File.WriteAllBytes(SocketInfoFile, stuff);
                
                Console.WriteLine($"Saved SocketInformation to: {SocketInfoFile}");
            }
        }
        
        public async Task RunAsync()
        {
            Console.WriteLine("********* LISTENER *********");

            IPHostEntry ipHostInfo = Dns.GetHostEntry("localhost");
            IPAddress ipAddress = ipHostInfo.AddressList.First(a => a.AddressFamily == AddressFamily.InterNetwork);
            IPEndPoint localEndPoint = new IPEndPoint(ipAddress, ServerPort);

            using Socket listener = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            listener.Bind(localEndPoint);
            listener.Listen(10);

            Console.WriteLine("Waiting for conn ..");
            Socket handler = await listener.AcceptAsync();
            Console.WriteLine($"ESTABLISHED: {handler.RemoteEndPoint} <------ {handler.LocalEndPoint}");
            int handlerId = GetHandlerProcessId();
            SocketInformation socketInfo = handler.DuplicateAndClose(handlerId);
            
            SerializeSocketInfo(socketInfo);
        }

        public void Dispose()
        {
            
        }
    }
}
