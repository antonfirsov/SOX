using System;

namespace UDP_StressTest
{
    public class TestOutputHelper : ITestOutputHelper
    {
        public string Prefix { get; set; }
        public void WriteLine(string msg)
        {
            Console.WriteLine($"{Prefix} {msg}");
        }
    }
}