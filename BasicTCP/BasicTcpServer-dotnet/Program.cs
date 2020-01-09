using System;

namespace Server
{
    class Program
    {
        static void Main(string[] args)
        {
            System.Reflection.Assembly assembly = typeof(int).Assembly;
            Console.WriteLine(assembly.ImageRuntimeVersion);
            Console.WriteLine(assembly.FullName);

            try
            {
                using var server = new TestServer();
                server.Run();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            

            Console.ReadLine();
        }
    }
}
