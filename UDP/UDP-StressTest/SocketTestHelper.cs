// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace UDP_StressTest
{
    // Abstract base class for various different socket "modes" (sync, async, etc)
    // See SendReceive.cs for usage
    public abstract class SocketHelperBase
    {
        protected SocketHelperBase(ITestOutputHelper output)
        {
            Output = output;
        }

        protected ITestOutputHelper Output { get; }
        
        public abstract Task<Socket> AcceptAsync(Socket s);
        public abstract Task<Socket> AcceptAsync(Socket s, Socket acceptSocket);
        public abstract Task ConnectAsync(Socket s, EndPoint endPoint);
        public abstract Task MultiConnectAsync(Socket s, IPAddress[] addresses, int port);
        public abstract Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer);
        public abstract Task<SocketReceiveFromResult> ReceiveFromAsync(
            Socket s, ArraySegment<byte> buffer, EndPoint endPoint);
        public abstract Task<int> ReceiveAsync(Socket s, IList<ArraySegment<byte>> bufferList);
        public abstract Task<int> SendAsync(Socket s, ArraySegment<byte> buffer);
        public abstract Task<int> SendAsync(Socket s, IList<ArraySegment<byte>> bufferList);
        public abstract Task<int> SendToAsync(Socket s, ArraySegment<byte> buffer, EndPoint endpoint);
        public virtual bool GuaranteedSendOrdering => true;
        public virtual bool ValidatesArrayArguments => true;
        public virtual bool UsesSync => false;
        public virtual bool UsesApm => false;
        public virtual bool DisposeDuringOperationResultsInDisposedException => false;
        public virtual bool ConnectAfterDisconnectResultsInInvalidOperationException => false;
        public virtual bool SupportsMultiConnect => true;
        public virtual bool SupportsAcceptIntoExistingSocket => true;
        public virtual void Listen(Socket s, int backlog)  { s.Listen(backlog); }
    }

    public class SocketHelperArraySync : SocketHelperBase
    {
        public override Task<Socket> AcceptAsync(Socket s) =>
            Task.Run(() => s.Accept());
        public override Task<Socket> AcceptAsync(Socket s, Socket acceptSocket) => throw new NotSupportedException();
        public override Task ConnectAsync(Socket s, EndPoint endPoint) =>
            Task.Run(() => s.Connect(endPoint));
        public override Task MultiConnectAsync(Socket s, IPAddress[] addresses, int port) =>
            Task.Run(() => s.Connect(addresses, port));
        public override Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer) =>
            Task.Run(() => s.Receive(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None));
        public override Task<int> ReceiveAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            Task.Run(() => s.Receive(bufferList, SocketFlags.None));
        public override Task<SocketReceiveFromResult> ReceiveFromAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            Task.Run(() =>
            {
                int received = s.ReceiveFrom(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None, ref endPoint);
                return new SocketReceiveFromResult
                {
                    ReceivedBytes = received,
                    RemoteEndPoint = endPoint
                };
            });
        public override Task<int> SendAsync(Socket s, ArraySegment<byte> buffer) =>
            Task.Run(() => s.Send(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None));
        public override Task<int> SendAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            Task.Run(() => s.Send(bufferList, SocketFlags.None));
        public override Task<int> SendToAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            Task.Run(() => s.SendTo(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None, endPoint));

        public override bool GuaranteedSendOrdering => false;
        public override bool UsesSync => true;
        public override bool ConnectAfterDisconnectResultsInInvalidOperationException => true;
        public override bool SupportsAcceptIntoExistingSocket => false;

        public SocketHelperArraySync(ITestOutputHelper output) : base(output)
        {
        }
    }


    public sealed class SocketHelperApm : SocketHelperBase
    {
        public override bool DisposeDuringOperationResultsInDisposedException => true;
        public override Task<Socket> AcceptAsync(Socket s) =>
            Task.Factory.FromAsync(s.BeginAccept, s.EndAccept, null);
        public override Task<Socket> AcceptAsync(Socket s, Socket acceptSocket) =>
            Task.Factory.FromAsync(s.BeginAccept, s.EndAccept, acceptSocket, 0, null);
        public override Task ConnectAsync(Socket s, EndPoint endPoint) =>
            Task.Factory.FromAsync(s.BeginConnect, s.EndConnect, endPoint, null);
        public override Task MultiConnectAsync(Socket s, IPAddress[] addresses, int port) =>
            Task.Factory.FromAsync(s.BeginConnect, s.EndConnect, addresses, port, null);
        public override Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer) =>
            Task.Factory.FromAsync((callback, state) =>
                s.BeginReceive(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None, callback, state),
                s.EndReceive, null);
        public override Task<int> ReceiveAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            Task.Factory.FromAsync(s.BeginReceive, s.EndReceive, bufferList, SocketFlags.None, null);
        public override Task<SocketReceiveFromResult> ReceiveFromAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint)
        {
            var tcs = new TaskCompletionSource<SocketReceiveFromResult>();
            s.BeginReceiveFrom(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None, ref endPoint, iar =>
            {
                try
                {
                    int receivedBytes = s.EndReceiveFrom(iar, ref endPoint);
                    tcs.TrySetResult(new SocketReceiveFromResult
                    {
                        ReceivedBytes = receivedBytes,
                        RemoteEndPoint = endPoint
                    });
                }
                catch (Exception e) { tcs.TrySetException(e); }
            }, null);
            return tcs.Task;
        }
        public override Task<int> SendAsync(Socket s, ArraySegment<byte> buffer) =>
            Task.Factory.FromAsync((callback, state) =>
                s.BeginSend(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None, callback, state),
                s.EndSend, null);
        public override Task<int> SendAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            Task.Factory.FromAsync(s.BeginSend, s.EndSend, bufferList, SocketFlags.None, null);
        public override Task<int> SendToAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            Task.Factory.FromAsync(
                (callback, state) => s.BeginSendTo(buffer.Array, buffer.Offset, buffer.Count, SocketFlags.None, endPoint, callback, state),
                s.EndSendTo, null);

        public override bool UsesApm => true;

        public SocketHelperApm(ITestOutputHelper output) : base(output)
        {
        }
    }

    public class SocketHelperTask : SocketHelperBase
    {
        public override Task<Socket> AcceptAsync(Socket s) =>
            s.AcceptAsync();
        public override Task<Socket> AcceptAsync(Socket s, Socket acceptSocket) =>
            s.AcceptAsync(acceptSocket);
        public override Task ConnectAsync(Socket s, EndPoint endPoint) =>
            s.ConnectAsync(endPoint);
        public override Task MultiConnectAsync(Socket s, IPAddress[] addresses, int port) =>
            s.ConnectAsync(addresses, port);
        public override Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer) =>
            s.ReceiveAsync(buffer, SocketFlags.None);
        public override Task<int> ReceiveAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            s.ReceiveAsync(bufferList, SocketFlags.None);
        public override Task<SocketReceiveFromResult> ReceiveFromAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            s.ReceiveFromAsync(buffer, SocketFlags.None, endPoint);
        public override Task<int> SendAsync(Socket s, ArraySegment<byte> buffer) =>
            s.SendAsync(buffer, SocketFlags.None);
        public override Task<int> SendAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            s.SendAsync(bufferList, SocketFlags.None);
        public override Task<int> SendToAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            s.SendToAsync(buffer, SocketFlags.None, endPoint);

        public SocketHelperTask(ITestOutputHelper output) : base(output)
        {
        }
    }

    public sealed class SocketHelperEap : SocketHelperBase
    {
        public override bool ValidatesArrayArguments => false;

        public override Task<Socket> AcceptAsync(Socket s) =>
            InvokeAsync("AcceptAsync", s, e => e.AcceptSocket, e => s.AcceptAsync(e));
        public override Task<Socket> AcceptAsync(Socket s, Socket acceptSocket) =>
            InvokeAsync("AcceptAsync", s, e => e.AcceptSocket, e =>
            {
                e.AcceptSocket = acceptSocket;
                return s.AcceptAsync(e);
            });
        public override Task ConnectAsync(Socket s, EndPoint endPoint) =>
            InvokeAsync("ConnectAsync", s, e => true, e =>
            {
                e.RemoteEndPoint = endPoint;
                return s.ConnectAsync(e);
            });
        public override Task MultiConnectAsync(Socket s, IPAddress[] addresses, int port) => throw new NotSupportedException();
        public override Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer) =>
            InvokeAsync("ReceiveAsync", s, e => e.BytesTransferred, e =>
            {
                e.SetBuffer(buffer.Array, buffer.Offset, buffer.Count);
                return s.ReceiveAsync(e);
            });
        public override Task<int> ReceiveAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            InvokeAsync("ReceiveAsync", s, e => e.BytesTransferred, e =>
            {
                e.BufferList = bufferList;
                return s.ReceiveAsync(e);
            });
        public override Task<SocketReceiveFromResult> ReceiveFromAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            InvokeAsync("ReceiveFromAsync", s, e => new SocketReceiveFromResult { ReceivedBytes = e.BytesTransferred, RemoteEndPoint = e.RemoteEndPoint }, e =>
            {
                e.SetBuffer(buffer.Array, buffer.Offset, buffer.Count);
                e.RemoteEndPoint = endPoint;
                return s.ReceiveFromAsync(e);
            });
        public override Task<int> SendAsync(Socket s, ArraySegment<byte> buffer) =>
            InvokeAsync("SendAsync", s, e => e.BytesTransferred, e =>
            {
                e.SetBuffer(buffer.Array, buffer.Offset, buffer.Count);
                return s.SendAsync(e);
            });
        public override Task<int> SendAsync(Socket s, IList<ArraySegment<byte>> bufferList) =>
            InvokeAsync("SendAsync",s, e => e.BytesTransferred, e =>
            {
                e.BufferList = bufferList;
                return s.SendAsync(e);
            });
        public override Task<int> SendToAsync(Socket s, ArraySegment<byte> buffer, EndPoint endPoint) =>
            InvokeAsync("SendToAsync", s, e => e.BytesTransferred, e =>
            {
                e.SetBuffer(buffer.Array, buffer.Offset, buffer.Count);
                e.RemoteEndPoint = endPoint;
                return s.SendToAsync(e);
            });

        private Task<TResult> InvokeAsync<TResult>(
            string operationName,
            Socket s,
            Func<SocketAsyncEventArgs, TResult> getResult,
            Func<SocketAsyncEventArgs, bool> invoke)
        {
            var tcs = new TaskCompletionSource<TResult>();
            var saea = new SocketAsyncEventArgs();
            EventHandler<SocketAsyncEventArgs> handler = (_, e) =>
            {
                if (e.SocketError == SocketError.Success)
                    tcs.SetResult(getResult(e));
                else
                    tcs.SetException(new SocketException((int)e.SocketError));
                saea.Dispose();
            };
            saea.Completed += handler;
            if (!invoke(saea))
            {
                Output.WriteLine($"- ! {operationName}: completed synchronously!");
                handler(s, saea);
            }
            else
            {
                Output.WriteLine($"- ? {operationName}: async invocation");
            }
            return tcs.Task;
        }

        public override bool SupportsMultiConnect => false;

        public SocketHelperEap(ITestOutputHelper output) : base(output)
        {
        }
    }

    public class SocketHelperSpanSync : SocketHelperArraySync
    {
        public override bool ValidatesArrayArguments => false;
        public override Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer) =>
            Task.Run(() => s.Receive((Span<byte>)buffer, SocketFlags.None));
        public override Task<int> SendAsync(Socket s, ArraySegment<byte> buffer) =>
            Task.Run(() => s.Send((ReadOnlySpan<byte>)buffer, SocketFlags.None));
        public override bool UsesSync => true;

        public SocketHelperSpanSync(ITestOutputHelper output) : base(output)
        {
        }
    }


    public sealed class SocketHelperMemoryArrayTask : SocketHelperTask
    {
        public override bool ValidatesArrayArguments => false;
        public override Task<int> ReceiveAsync(Socket s, ArraySegment<byte> buffer) =>
            s.ReceiveAsync((Memory<byte>)buffer, SocketFlags.None).AsTask();
        public override Task<int> SendAsync(Socket s, ArraySegment<byte> buffer) =>
            s.SendAsync((ReadOnlyMemory<byte>)buffer, SocketFlags.None).AsTask();

        public SocketHelperMemoryArrayTask(ITestOutputHelper output) : base(output)
        {
        }
    }

}
