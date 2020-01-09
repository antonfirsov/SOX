using System;

namespace Client
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
                using TestClient client = new TestClient();
                client.Run();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            

            Console.ReadLine();
        }
    }
}
