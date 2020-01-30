using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace UDP_StressTest
{
    class Program
    {
        static async Task Main(string[] args)
        {
            int numberOfSyncWorkers = 0;
            
            if (args.Length > 0)
            {
                numberOfSyncWorkers = int.Parse(args[0]);
            }
            Console.WriteLine($"Sync Workers: {numberOfSyncWorkers}");

            TestRepro a = TestRepro.CreateAsync(256, 10, "ASYNC");

            await Task.WhenAll(a.RunAsync(), CreateSyncWorkers(numberOfSyncWorkers));
        }


        private static Task CreateSyncWorkers(int count)
        {
            List<Task> workers = new List<Task>();

            for (int i = 0; i < count; i++)
            {
                TestRepro s = TestRepro.CreateSync(256, 1, $"sync{i}");
                workers.Add(s.RunAsync());
            }

            return Task.WhenAll(workers);
        }
    }
}