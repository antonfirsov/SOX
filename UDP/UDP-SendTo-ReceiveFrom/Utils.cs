using System;
using System.Diagnostics;

namespace UDP_SendTo_ReceiveFrom
{
    static class Utils
    {
        public static bool SequenceEquals<T>(this ArraySegment<T> left, ArraySegment<T> right)
        {
            if (left == null || right == null) throw new ArgumentNullException();
            
            if (left.Count != right.Count) return false;

            for (int i = 0; i < left.Count; i++)
            {
                if (!left[i].Equals(right[i])) return false;
            }

            return true;
        }

        public static void MustBeSequenceEqualTo<T>(this ArraySegment<T> left, ArraySegment<T> right)
        {
            if (!left.SequenceEquals(right)) throw new Exception("Segments do not match!");
        }
    }

    internal struct Measure : IDisposable
    {
        private readonly string _what;
        private readonly Stopwatch _sw;

        public Measure(string what)
        {
            _what = what;
            Console.WriteLine($"{what} ...");
            _sw = Stopwatch.StartNew();
        }

        public void Dispose()
        {
            _sw.Stop();
            Console.WriteLine($"{_what} completed in {_sw.ElapsedMilliseconds} ms");
        }
    }
}