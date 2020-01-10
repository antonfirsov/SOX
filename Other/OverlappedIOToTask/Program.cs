using System;
using System.Buffers;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using PInvoke;


namespace PInvoke
{
    internal static class Kernel32Ex
    {
        [DllImport("kernel32.dll")]
        public static extern Kernel32.SafeObjectHandle CreateFile(
            [MarshalAs(UnmanagedType.LPStr)]string name,
            [MarshalAs(UnmanagedType.U4)]uint access,
            [MarshalAs(UnmanagedType.U4)]Kernel32.FileShare share,
            IntPtr security,
            [MarshalAs(UnmanagedType.U4)]Kernel32.CreationDisposition mode,
            [MarshalAs(UnmanagedType.U4)]Kernel32.CreateFileFlags attributes,
            IntPtr template);
    }
    public static class HandleInterop
    {
        public static Kernel32.SafeObjectHandle ToPlatformHandle(this SafeHandle handle, bool ownsHandle = true) => new Kernel32.SafeObjectHandle(handle.DangerousGetHandle(), ownsHandle);
         
    }
}
namespace ConsoleApp21
{

    class Program
    {
        static Task<bool> ReadAsync(ThreadPoolBoundHandle handle, Memory<byte> memory)
        {
            var completion = new TaskCompletionSource<bool>();
            Kernel32.SafeObjectHandle soHandle = handle.Handle.ToPlatformHandle();
            
            var buffer = memory.Pin();
            unsafe
            {
                void callback(uint error,uint count,NativeOverlapped* overlapped)
                {
                    try
                    {
                        completion.SetResult(true);
                    }
                    finally
                    {
                        handle.FreeNativeOverlapped(overlapped);
                        buffer.Dispose();
                    }
                }
                var nol = handle.AllocateNativeOverlapped(callback, new object[] { completion, buffer }, null);
                Kernel32.ReadFile(soHandle, buffer.Pointer, memory.Length, null, (Kernel32.OVERLAPPED*)nol);
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.WaitForFullGCComplete();
                GC.Collect();
                return completion.Task;
            }
        }
        static void Main(string[] args)
        {
            string path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "test.bin");
            
            File.WriteAllText(path, "lol!foo");

            var ubHandle = Kernel32Ex.CreateFile(
                path,
                Kernel32.FileAccess.FILE_GENERIC_READ | Kernel32.FileAccess.FILE_GENERIC_WRITE,
                Kernel32.FileShare.FILE_SHARE_READ | Kernel32.FileShare.FILE_SHARE_WRITE,
                IntPtr.Zero,
                Kernel32.CreationDisposition.OPEN_EXISTING,
                Kernel32.CreateFileFlags.FILE_FLAG_OVERLAPPED,
                IntPtr.Zero);
            Win32ErrorCode error = Kernel32.GetLastError();
            Console.WriteLine(error);
            
            var fileHandle = ThreadPoolBoundHandle.BindHandle(ubHandle);
            Memory<byte> buffer = new byte[11];
            var t = ReadAsync(fileHandle, buffer);
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.WaitForFullGCComplete();
            GC.Collect();
            t.GetAwaiter().GetResult();
            for (int i = 0; i < buffer.Length;)
            {
                for (int b = 0; b < 8 && i < buffer.Length; b++, i++)
                {
                    char c = (char)buffer.Span[i];
                    Console.Write(c);
                    
                }
                Console.WriteLine();
            }

            Console.WriteLine("Yay! End!");
            Console.ReadLine();
        }
    }
}