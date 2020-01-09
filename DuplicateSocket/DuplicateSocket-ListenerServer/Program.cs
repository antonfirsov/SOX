using System;
using System.Threading.Tasks;

namespace DuplicateSocket_ListenerServer
{
    internal class Program
    {
        public static async Task Main(string[] args)
        {
            try
            {
                using var server = new ListenerServer();
                await server.RunAsync();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            

            Console.ReadLine();
        }
    }
}