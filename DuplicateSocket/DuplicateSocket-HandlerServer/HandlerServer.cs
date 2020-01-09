using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Threading;

namespace DuplicateSocket_HandlerServer
{
    internal class HandlerServer : IDisposable
    {
        private byte[] _buffer = new byte[1024];
        private StringBuilder _bld = new StringBuilder();

        private const int ServerPort = 11000;
        
        private static readonly string LocalAppDataFolder =
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

        private static readonly string SocketInfoFile = Path.Combine(LocalAppDataFolder, "_SocketInfo.json");

        private static readonly BinaryFormatter BinaryFormatter = new BinaryFormatter();
        
        private static SocketInformation DeserializeSocketInformation()
        {
            using FileStream fs = File.OpenRead(SocketInfoFile);
            SocketInformation socketInfo = (SocketInformation) BinaryFormatter.Deserialize(fs);
            File.Delete(SocketInfoFile);
            return socketInfo;
        }

        public void Run()
        {
            Console.WriteLine("********* HANDLER *********");
            if (File.Exists(SocketInfoFile)) File.Delete(SocketInfoFile);

            while (true)
            {
                if (File.Exists(SocketInfoFile))
                {
                    SocketInformation socketInfo = DeserializeSocketInformation();
                    Console.WriteLine($"Found Socket info: {socketInfo.Options}");
                }
                
                Thread.Sleep(100);
            }
        }

        public void Dispose()
        {
            
        }
    }
}
