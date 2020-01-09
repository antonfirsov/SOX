using System;
using System.Threading.Tasks;

namespace DuplicateSocket_HandlerServer
{
    internal class Program
    {
        public static async Task Main(string[] args)
        {
            try
            {
                using var server = new HandlerServer();
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