using System;

namespace DuplicateSocket_HandlerServer
{
    internal class Program
    {
        public static void Main(string[] args)
        {
            try
            {
                using var server = new HandlerServer();
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