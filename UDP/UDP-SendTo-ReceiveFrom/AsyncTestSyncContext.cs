using System;
using System.Threading;
using System.Threading.Tasks;

namespace UDP_SendTo_ReceiveFrom
{
    public class AsyncTestSyncContext : SynchronizationContext
	{
		readonly AsyncManualResetEvent @event = new AsyncManualResetEvent(true);
		Exception? exception;
		readonly SynchronizationContext? innerContext;
		int operationCount;

		/// <summary>
		/// Initializes a new instance of the <see cref="AsyncTestSyncContext"/> class.
		/// </summary>
		/// <param name="innerContext">The existing synchronization context (may be <c>null</c>).</param>
		public AsyncTestSyncContext(SynchronizationContext? innerContext)
		{
			this.innerContext = innerContext;
		}

		/// <inheritdoc/>
		public override void OperationCompleted()
		{
			var result = Interlocked.Decrement(ref operationCount);
			if (result == 0)
				@event.Set();
		}

		/// <inheritdoc/>
		public override void OperationStarted()
		{
			Interlocked.Increment(ref operationCount);
			@event.Reset();
		}

		/// <inheritdoc/>
		public override void Post(SendOrPostCallback d, object? state)
		{
			// The call to Post() may be the state machine signaling that an exception is
			// about to be thrown, so we make sure the operation count gets incremented
			// before the Task.Run, and then decrement the count when the operation is done.
			OperationStarted();

			try
			{
				if (innerContext == null)
				{
					ThreadPool.QueueUserWorkItem(_ =>
					{
						try
						{
							d(state);
						}
						catch (Exception ex)
						{
							exception = ex;
						}
						finally
						{
							OperationCompleted();
						}
					});
				}
				else
					innerContext.Post(_ =>
					{
						try
						{
							d(state);
						}
						catch (Exception ex)
						{
							exception = ex;
						}
						finally
						{
							OperationCompleted();
						}
					}, null);
			}
			catch { }
		}

		/// <inheritdoc/>
		public override void Send(SendOrPostCallback d, object? state)
		{
			try
			{
				if (innerContext != null)
					innerContext.Send(d, state);
				else
					d(state);
			}
			catch (Exception ex)
			{
				exception = ex;
			}
		}

		/// <summary>
		/// Returns a task which is signaled when all outstanding operations are complete.
		/// </summary>
		public async Task<Exception?> WaitForCompletionAsync()
		{
			await @event.WaitAsync();

			return exception;
		}

		class AsyncManualResetEvent
		{
			volatile TaskCompletionSource<bool> taskCompletionSource = new TaskCompletionSource<bool>();

			public AsyncManualResetEvent(bool signaled = false)
			{
				if (signaled)
					taskCompletionSource.TrySetResult(true);
			}

			public bool IsSet => taskCompletionSource.Task.IsCompleted;

			public void Reset()
			{
				if (IsSet)
					taskCompletionSource = new TaskCompletionSource<bool>();
			}

			public void Set() => taskCompletionSource.TrySetResult(true);

			public Task WaitAsync() => taskCompletionSource.Task;
		}
	}
}