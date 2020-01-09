using System;

namespace DuplicateSocket_ListenerServer
{
    internal class Program
    {
        public static void Main(string[] args)
        {
            try
            {
                using var server = new ListenerServer();
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