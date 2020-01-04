using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using JetBrains.Annotations;
using PtyClr.LinApi;
// ReSharper disable UnusedAutoPropertyAccessor.Global
// ReSharper disable MemberCanBePrivate.Global

namespace PtyClr
{
    // ReSharper disable once UnusedMember.Global
    public sealed class Pty : IDisposable
    {
        #region Constants

        private static readonly NumberFormatInfo NfiNoThousandsSeparator = new NumberFormatInfo
            {NumberGroupSeparator = string.Empty};

        #endregion Constants

        #region Static

        static Pty() => Constants.DoAsserts();

        private static void Dispose(Queue<CommandPack> queue, [NotNull] Func<Exception> exceptionFactory)
        {
            if (queue == null)
                return;

            while (queue.Any())
                queue.Dequeue().TaskCompletionSource.TrySetException(exceptionFactory());
        }

        private static void Dispose(Queue<TaskCompletionSource<object>> queue,
            [NotNull] Func<Exception> exceptionFactory)
        {
            if (queue == null)
                return;

            while (queue.Any())
                queue.Dequeue().TrySetException(exceptionFactory());
        }

        #endregion Static

        #region Fields

        private readonly object _lock = new object();
        private readonly CancellationTokenSource _masterCts = new CancellationTokenSource();
        private readonly PtyBuild _ptyBuild;

        private Process _mediatorProcess;
        private AnonymousPipeServerStream _outputStream;
        private AnonymousPipeServerStream _inputStream;
        private AnonymousPipeServerStream _inputRecordStream;
        private AnonymousPipeServerStream _cmdInStream;
        private AnonymousPipeServerStream _cmdOutStream;

        private bool _valid;
        private bool _disposed;

        #endregion Fields

        #region Properties

        public string ShellCommand { get; private set; }

        public string Arguments { get; private set; }

        public PipeStream OutputStream => _outputStream;

        #endregion Properties

        #region Events

        public event EventHandler Corrupt;

        #endregion Events

        #region Constructors

        public Pty(PtyBuild ptyBuild) => _ptyBuild = ptyBuild;

        #endregion Constructors

        #region API methods

        public void Spawn([NotNull] string command, string arguments = null, ushort cols = 80,
            ushort rows = 25, IDictionary<string, string> environmentVariables = null, string workingDirectory = null,
            LogLevel logLevel = LogLevel.None, bool sysLog = false)
        {
            if (string.IsNullOrEmpty(command))
                throw new ArgumentNullException(nameof(command), "Argument is either null or empty.");

            lock (_lock)
            {
                if (_disposed)
                    throw new ObjectDisposedException(nameof(Pty));

                if (_outputStream != null)
                    throw new InvalidOperationException("The method can be called only once.");

                _outputStream = new AnonymousPipeServerStream(PipeDirection.In, HandleInheritability.Inheritable);
            }

            var mediatorExe = Helpers.FindMediatorExecutable(_ptyBuild);

            if (string.IsNullOrEmpty(mediatorExe))
            {
                _outputStream.Dispose();

                throw new Exception("Mediator executable not found.");
            }

            ShellCommand = command;
            Arguments = arguments;

            _inputStream = new AnonymousPipeServerStream(PipeDirection.Out, HandleInheritability.Inheritable);
            _inputRecordStream = new AnonymousPipeServerStream(PipeDirection.Out, HandleInheritability.Inheritable);
            _cmdInStream = new AnonymousPipeServerStream(PipeDirection.Out, HandleInheritability.Inheritable);
            _cmdOutStream = new AnonymousPipeServerStream(PipeDirection.In, HandleInheritability.Inheritable);

            var args =
                $"--out {_outputStream.GetClientHandleAsString()} --ins {_inputStream.GetClientHandleAsString()} --inr {_inputRecordStream.GetClientHandleAsString()} --cmd {_cmdInStream.GetClientHandleAsString()};{_cmdOutStream.GetClientHandleAsString()} --rows {rows.ToString(NfiNoThousandsSeparator)} --cols {cols.ToString(NfiNoThousandsSeparator)} --log {((int) logLevel).ToString(NfiNoThousandsSeparator)}";

            if (!string.IsNullOrEmpty(workingDirectory))
                args += $" --dir {workingDirectory.QuoteIfNeeded()}";

            if (sysLog)
                args += " --syslog";

            args += $" - {command.Trim().QuoteIfNeeded()}";

            if (!string.IsNullOrEmpty(arguments))
                args += " " + arguments.Trim();

            _mediatorProcess = new Process
            {
                StartInfo =
                {
                    FileName = mediatorExe,
                    Arguments = args,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    RedirectStandardInput = true
                },
                EnableRaisingEvents = true
            };

            _mediatorProcess.Exited += MediatorProcessExited;

            if (environmentVariables != null)
            {
                foreach (var kvp in environmentVariables)
                    _mediatorProcess.StartInfo.EnvironmentVariables[kvp.Key] = kvp.Value;
            }

            try
            {
                _mediatorProcess.Start();
            }
            catch
            {
                _mediatorProcess.Dispose();
                _outputStream.Dispose();
                _inputStream.Dispose();
                _inputRecordStream.Dispose();
                _cmdInStream.Dispose();
                _cmdOutStream.Dispose();

                throw;
            }

            _outputStream.DisposeLocalCopyOfClientHandle();
            _inputStream.DisposeLocalCopyOfClientHandle();
            _inputRecordStream.DisposeLocalCopyOfClientHandle();
            _cmdInStream.DisposeLocalCopyOfClientHandle();
            _cmdOutStream.DisposeLocalCopyOfClientHandle();

            lock (_lock)
                _valid = true;
        }

        public void WriteInput(byte[] input)
        {
            if (ValidateCall() is Exception ex)
                throw ex;

            lock (_inputLock)
            {
                try
                {
                    _inputStream.Write(input, 0, input.Length);
                }
                catch (Exception e)
                {
                    ReportCorrupt(e);

                    throw new TerminalCorruptException();
                }
            }
        }

        public Task WriteInputAsync(byte[] input)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException(ex);

            return EnqueueInputAsync(input);
        }

        /// <summary>
        /// Sends <paramref name="keyEventRecord"/> to the background process' input stream.
        /// </summary>
        /// <param name="keyEventRecord"><see cref="KEY_EVENT_RECORD"/> to send.</param>
        /// <remarks>
        /// <note type="warn">You should not use both synchronous and async versions of record-sending methods in
        /// the same implementation! If both async and sync are used at the same time (while previous isn't finished),
        /// it may cause stream corruption, and even terminal corruption.</note>
        /// In other words, you should either decide to go with sync implementation, and use
        /// <see cref="SendKeyEventRecord"/> and <see cref="SendMouseEventRecord"/> everywhere, or go with async
        /// versions and use <see cref="SendKeyEventRecordAsync"/> and <see cref="SendMouseEventRecordAsync"/> everywhere.
        /// </remarks>
        /// <seealso cref="SendKeyEventRecordAsync"/>
        public void SendKeyEventRecord(ref KEY_EVENT_RECORD keyEventRecord)
        {
            if (ValidateCall() is Exception ex)
                throw ex;

            var rec = new INPUT_RECORD { EventType = EventType.KEY_EVENT, KeyEvent = keyEventRecord };

            SendInputRecord(rec.ToByteArray());
        }

        /// <summary>
        /// Async version of <see cref="SendKeyEventRecord"/>.
        /// </summary>
        /// <inheritdoc cref="SendKeyEventRecord" select="param,remarks"/>
        /// <seealso cref="SendKeyEventRecord"/>
        public Task SendKeyEventRecordAsync(ref KEY_EVENT_RECORD keyEventRecord)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException(ex);

            var rec = new INPUT_RECORD { EventType = EventType.KEY_EVENT, KeyEvent = keyEventRecord };

            return EnqueueInputRecordAsync(rec.ToByteArray());
        }

        /// <summary>
        /// Sends <paramref name="mouseEventRecord"/> to the background process' input stream.
        /// </summary>
        /// <param name="mouseEventRecord"><see cref="MOUSE_EVENT_RECORD"/> to send.</param>
        /// <inheritdoc cref="SendKeyEventRecord" select="remarks"/>
        /// <seealso cref="SendMouseEventRecordAsync"/>
        public void SendMouseEventRecord(ref MOUSE_EVENT_RECORD mouseEventRecord)
        {
            if (ValidateCall() is Exception ex)
                throw ex;

            var rec = new INPUT_RECORD { EventType = EventType.MOUSE_EVENT, MouseEvent = mouseEventRecord };
            
            SendInputRecord(rec.ToByteArray());
        }

        /// <summary>
        /// Async version of <see cref="SendMouseEventRecord"/>.
        /// </summary>
        /// <inheritdoc cref="SendMouseEventRecord" select="param"/>
        /// <inheritdoc cref="SendKeyEventRecord" select="remarks"/>
        /// <seealso cref="SendMouseEventRecord"/>
        public Task SendMouseEventRecordAsync(ref MOUSE_EVENT_RECORD mouseEventRecord)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException(ex);

            var rec = new INPUT_RECORD { EventType = EventType.MOUSE_EVENT, MouseEvent = mouseEventRecord };

            return EnqueueInputRecordAsync(rec.ToByteArray());
        }

        public Task PingAsync(CancellationToken? cancellationToken = null)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException(ex);

            return EnqueueAsync(new[] { (byte)Command.Ping }, cancellationToken ?? CancellationToken.None);
        }

        public Task<WinSize> GetWinSizeAsync(CancellationToken? cancellationToken = null)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException<WinSize>(ex);

            return EnqueueAsync(new[] { (byte)Command.GetWinSize }, cancellationToken ?? CancellationToken.None)
                .ContinueWith(t => (WinSize)t.Result, TaskContinuationOptions.OnlyOnRanToCompletion);
        }

        public Task SetWinSizeAsync(WinSize winSize, CancellationToken? cancellationToken = null)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException(ex);

            var command = new byte[Constants.WinSizeSize + 1];

            command[0] = (byte)Command.SetWinSize;

            unsafe
            {
                command.ReadFromPtr(winSize.Bytes, destIndex: 1);
            }

            return EnqueueAsync(command, cancellationToken ?? CancellationToken.None);
        }

        public Task<Termios> GetAttributesAsync(CancellationToken? cancellationToken = null)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException<Termios>(ex);

            return EnqueueAsync(new[] { (byte)Command.GetAttributes }, cancellationToken ?? CancellationToken.None)
                .ContinueWith(t => (Termios)t.Result, TaskContinuationOptions.OnlyOnRanToCompletion);
        }

        public Task SetAttributesAsync(Termios attributes, CancellationToken? cancellationToken = null)
        {
            if (ValidateCall() is Exception ex)
                return Task.FromException(ex);

            var command = new byte[Constants.Termios_size + 1];

            command[0] = (byte)Command.SetWinSize;

            unsafe
            {
                command.ReadFromPtr(attributes.Bytes, destIndex: 1);
            }

            return EnqueueAsync(command, cancellationToken ?? CancellationToken.None);
        }

        public void Dispose()
        {
            lock (_lock)
            {
                if (_disposed)
                    return;

                _disposed = true;
            }

            Queue<CommandPack> cmdQueue;

            lock (_commandLock)
            {
                cmdQueue = _commandQueue;
                _commandQueue = null;
            }

            Dispose(cmdQueue, () => new ObjectDisposedException(nameof(Pty)));

            Queue<TaskCompletionSource<object>> tcsQueue;

            lock (_inputLock)
            {
                tcsQueue = _inputQueue;
                _inputQueue = null;
            }

            Dispose(tcsQueue, () => new ObjectDisposedException(nameof(Pty)));

            lock (_inRecLock)
            {
                tcsQueue = _inRecQueue;
                _inRecQueue = null;
            }

            Dispose(tcsQueue, () => new ObjectDisposedException(nameof(Pty)));

            try
            {
                _masterCts.Cancel(false);
            }
            catch
            {
                // ignored
            }

            try
            {
                _masterCts.Dispose();
            }
            catch
            {
                // ignored
            }

            if (_mediatorProcess != null)
            {
                try
                {
                    _mediatorProcess.Kill();
                }
                catch
                {
                    // ignored
                }

                _mediatorProcess.Dispose();
            }

            try
            {
                _outputStream?.Dispose();
            }
            catch
            {
                // ignored
            }

            try
            {
                _inputStream?.Dispose();
            }
            catch
            {
                // ignored
            }

            try
            {
                _inputRecordStream?.Dispose();
            }
            catch
            {
                // ignored
            }

            try
            {
                _cmdInStream?.Dispose();
            }
            catch
            {
                // ignored
            }

            try
            {
                _cmdOutStream?.Dispose();
            }
            catch
            {
                // ignored
            }
        }

        #endregion API Methods

        #region Input records handling

        private readonly object _inRecLock = new object();

        private Queue<TaskCompletionSource<object>> _inRecQueue;

        private void SendInputRecord(byte[] record)
        {
            lock (_inRecLock)
            {
                try
                {
                    _inputRecordStream.Write(record, 0, record.Length);
                }
                catch (Exception e)
                {
                    ReportCorrupt(e);
                    throw new TerminalCorruptException();
                }
            }
        }

        private Task EnqueueInputRecordAsync(byte[] record)
        {
            var tcs = new TaskCompletionSource<object>(record);

            lock (_inRecLock)
            {
                var shouldLaunch = false;

                if (_inRecQueue == null)
                {
                    shouldLaunch = true;

                    _inRecQueue = new Queue<TaskCompletionSource<object>>();
                }

                _inRecQueue.Enqueue(tcs);

                if (shouldLaunch)
                    // ReSharper disable once AssignmentIsFullyDiscarded
                    _ = WriteInputRecordsAsync();
            }

            return tcs.Task;
        }

        private async Task WriteInputRecordsAsync()
        {
            while (true)
            {
                Queue<TaskCompletionSource<object>> queue;

                lock (_inRecLock)
                {
                    queue = _inRecQueue;

                    if (queue == null)
                        return;

                    _inRecQueue = null;
                }

                while (queue.Any())
                {
                    lock (_lock)
                    {
                        if (_disposed)
                        {
                            Dispose(queue, () => new ObjectDisposedException(nameof(Pty)));
                            return;
                        }

                        if (!_valid)
                        {
                            Dispose(queue, () => new TerminalCorruptException());
                            return;
                        }
                    }

                    var tcs = queue.Dequeue();
                    var rec = (byte[]) tcs.Task.AsyncState;

                    try
                    {
                        await _inputRecordStream.WriteAsync(rec, 0, rec.Length, _masterCts.Token).ConfigureAwait(false);
                    }
                    catch (Exception e)
                    {
                        ReportCorrupt(e);
                        tcs.TrySetException(new TerminalCorruptException());
                        continue;
                    }

                    tcs.TrySetResult(null);
                }
            }
        }

        #endregion Input records handling

        #region Input handling

        private readonly object _inputLock = new object();

        private Queue<TaskCompletionSource<object>> _inputQueue;

        private Task EnqueueInputAsync(byte[] input)
        {
            var tcs = new TaskCompletionSource<object>(input);

            lock (_inputLock)
            {
                var shouldLaunch = false;

                if (_inputQueue == null)
                {
                    shouldLaunch = true;

                    _inputQueue = new Queue<TaskCompletionSource<object>>();
                }

                _inputQueue.Enqueue(tcs);

                if (shouldLaunch)
                    // ReSharper disable once AssignmentIsFullyDiscarded
                    _ = WriteInputAsync();
            }

            return tcs.Task;
        }

        private async Task WriteInputAsync()
        {
            while (true)
            {
                Queue<TaskCompletionSource<object>> queue;

                lock (_inputLock)
                {
                    queue = _inputQueue;

                    if (queue == null)
                        return;

                    _inputQueue = null;
                }

                while (queue.Any())
                {
                    lock (_lock)
                    {
                        if (_disposed)
                        {
                            Dispose(queue, () => new ObjectDisposedException(nameof(Pty)));
                            return;
                        }

                        if (!_valid)
                        {
                            Dispose(queue, () => new TerminalCorruptException());
                            return;
                        }
                    }

                    var tcs = queue.Dequeue();
                    var input = (byte[]) tcs.Task.AsyncState;

                    try
                    {
                        await _inputStream.WriteAsync(input, 0, input.Length, _masterCts.Token).ConfigureAwait(false);
                    }
                    catch (Exception e)
                    {
                        ReportCorrupt(e);
                        tcs.TrySetException(new TerminalCorruptException());
                        continue;
                    }

                    tcs.TrySetResult(null);
                }
            }
        }

        #endregion Input handling

        #region Command processing

        private class CommandPack
        {
            [NotNull]
            internal TaskCompletionSource<object> TaskCompletionSource { get; }

            internal CancellationToken CancellationToken { get; }

            internal CommandPack([NotNull] TaskCompletionSource<object> taskCompletionSource,
                CancellationToken cancellationToken)
            {
                TaskCompletionSource = taskCompletionSource;
                CancellationToken = cancellationToken;
            }
        }

        private const byte SuccessByte = 0;
        private const byte FailureByte = 1;

        private readonly object _commandLock = new object();

        private Queue<CommandPack> _commandQueue;

        private Task<object> EnqueueAsync(byte[] command, CancellationToken cancellationToken)
        {
            var cmd = new CommandPack(new TaskCompletionSource<object>(command), cancellationToken);

            lock (_commandLock)
            {
                var running = true;

                if (_commandQueue == null)
                {
                    _commandQueue = new Queue<CommandPack>();

                    running = false;
                }

                _commandQueue.Enqueue(cmd);

                if (!running)
                    // ReSharper disable once AssignmentIsFullyDiscarded
                    _ = ExecuteCommandsAsync();
            }

            return cmd.TaskCompletionSource.Task;
        }

        private async Task ExecuteCommandsAsync()
        {
            while (true)
            {
                Queue<CommandPack> queue;

                lock (_commandLock)
                {
                    queue = _commandQueue;

                    _commandQueue = null;
                }

                if (queue == null)
                    return;

                while (queue.Any())
                {
                    lock (_lock)
                    {
                        if (_disposed)
                        {
                            Dispose(queue, () => new ObjectDisposedException(nameof(Pty)));
                            return;
                        }

                        if (!_valid)
                        {
                            Dispose(queue, () => new TerminalCorruptException());
                            return;
                        }
                    }

                    var cmd = queue.Dequeue();

                    if (cmd.CancellationToken.IsCancellationRequested)
                    {
                        cmd.TaskCompletionSource.TrySetCanceled(cmd.CancellationToken);

                        continue;
                    }

                    await ExecuteCommandAsync(cmd).ConfigureAwait(false);
                }
            }
        }

        private async Task ExecuteCommandAsync(CommandPack command)
        {
            var cmd = (byte[])command.TaskCompletionSource.Task.AsyncState;

            try
            {
                await _cmdInStream.WriteAsync(cmd, 0, cmd.Length, _masterCts.Token).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                ReportCorrupt(ex);

                command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                return;
            }

            lock (_lock)
            {
                if (_disposed)
                    command.TaskCompletionSource.TrySetException(new ObjectDisposedException(nameof(Pty)));

                if (!_valid)
                    command.TaskCompletionSource.TrySetException(new TerminalCorruptException());
            }

            var buff = new byte[1];

            int read;

            try
            {
                read = await _cmdOutStream.ReadAsync(buff, 0, 1, _masterCts.Token).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                ReportCorrupt(ex);

                command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                return;
            }

            if (read != 1)
            {
                // Won't happen ever, but...
                ReportCorrupt();

                command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                return;
            }

            if (buff[0] == FailureByte)
            {
                string message;

                try
                {
                    message = await _cmdOutStream.ReadStringAsync(_masterCts.Token);
                }
                catch (Exception ex)
                {
                    ReportCorrupt(ex);

                    command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                    return;
                }

                command.TaskCompletionSource.TrySetException(new Exception(message));

                return;
            }

            if (buff[0] != SuccessByte)
            {
                ReportCorrupt();

                command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                return;
            }

            switch ((Command)cmd[0])
            {
                case Command.Ping:
                    command.TaskCompletionSource.TrySetResult(null);
                    return;

                case Command.GetWinSize:

                    WinSize? winSize;

                    try
                    {
                        winSize = await _cmdOutStream.ReadWinSizeAsync(_masterCts.Token);
                    }
                    catch (Exception ex)
                    {
                        ReportCorrupt(ex);

                        command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                        return;
                    }

                    command.TaskCompletionSource.TrySetResult(winSize.Value);

                    return;

                case Command.SetWinSize:
                    command.TaskCompletionSource.TrySetResult(null);
                    return;

                case Command.GetAttributes:

                    Termios? termios;

                    try
                    {
                        termios = await _cmdOutStream.ReadTermiosAsync(_masterCts.Token);
                    }
                    catch (Exception ex)
                    {
                        ReportCorrupt(ex);

                        command.TaskCompletionSource.TrySetException(new TerminalCorruptException());

                        return;
                    }

                    command.TaskCompletionSource.TrySetResult(termios.Value);

                    return;

                case Command.SetAttributes:
                    command.TaskCompletionSource.TrySetResult(null);
                    return;

                default:
                    // Won't happen ever, but still...
                    ReportCorrupt();

                    break;
            }
        }

        #endregion Command processing

        #region Private methods

        private Exception ValidateCall()
        {
            lock (_lock)
            {
                if (_disposed)
                    return new ObjectDisposedException(nameof(Pty));

                if (_outputStream == null)
                    return new InvalidOperationException($"Method {nameof(Spawn)} must be called first.");

                if (!_valid)
                    return new TerminalCorruptException();
            }

            return null;
        }

        // ReSharper disable once UnusedParameter.Local
        private void ReportCorrupt(Exception ex = null)
        {
            lock (_lock)
            {
                if (!_valid)
                    return;

                _valid = false;

                if (_disposed)
                    return;
            }

            try
            {
                _masterCts.Cancel(false);
            }
            catch
            {
                // ignored
            }

            Corrupt?.Invoke(this, EventArgs.Empty);
        }

        private void MediatorProcessExited(object sender, EventArgs e) => ReportCorrupt();

        #endregion Private methods
    }
}
