using System;
using System.Threading.Tasks;

namespace Server
{
    class Program
    {
        static async Task Main(string[] args)
        {
            System.Reflection.Assembly assembly = typeof(int).Assembly;
            Console.WriteLine(assembly.ImageRuntimeVersion);
            Console.WriteLine(assembly.FullName);

            try
            {
                using var server = new TestServer();
                server.RunAsync();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            

            Console.ReadLine();
        }
    }
}
