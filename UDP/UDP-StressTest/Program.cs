using System;
using System.Threading.Tasks;

namespace UDP_StressTest
{
    class Program
    {
        static async Task Main(string[] args)
        {
            TestRepro repro = TestRepro.CreateAsync(1, 10);
            await repro.RunAsync();
        }
    }
}