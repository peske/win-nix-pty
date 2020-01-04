using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using JetBrains.Annotations;
using PtyClr.LinApi;

namespace PtyClr
{
    internal static class Helpers
    {
        private const string CygwinBuildExeName = "pty-cyg.exe";
        // ReSharper disable once IdentifierTypo
        private const string Msys2BuildExeName = "pty-msys2.exe";

        private static readonly Regex WhiteSpaceRx = new Regex(@"\s", RegexOptions.Compiled);

        #region Byte array conversions

        private static unsafe void WriteToPtr([NotNull] this byte[] src, byte* dest, int srcIndex = 0,
            int destIndex = 0, int count = -1)
        {
            if (count < 0)
                count = src.Length - srcIndex;

            for (var i = 0; i < count; ++i)
                dest[destIndex + i] = src[srcIndex + i];
        }

        internal static unsafe void ReadFromPtr([NotNull] this byte[] dest, byte* src, int srcIndex = 0,
            int destIndex = 0, int count = -1)
        {
            if (count < 0)
                count = dest.Length - destIndex;

            for (var i = 0; i < count; ++i)
                dest[destIndex + i] = src[srcIndex + i];
        }

        internal static unsafe byte[] ToByteArray(this INPUT_RECORD rec)
        {
            var buff = new byte[Constants.INPUT_RECORD_size];
            buff.ReadFromPtr(rec.Bytes);
            return buff;
        }

        #endregion Byte array conversions

        #region Stream helpers

        private static async Task<byte[]> ReadExactAsync([NotNull] this PipeStream stream, int count,
            CancellationToken cancellationToken)
        {
            byte[] buff = new byte[count];
            var offset = 0;

            while (count > 0)
            {
                int read;

                try
                {
                    read = await stream.ReadAsync(buff, offset, count, cancellationToken).ConfigureAwait(false);
                }
                catch
                {
                    return null;
                }

                if (read < 1)
                    return null;

                count -= read;
                offset += read;
            }

            return buff;
        }

        internal static async Task<string> ReadStringAsync([NotNull] this PipeStream stream,
            CancellationToken cancellationToken)
        {
            var buff = new byte[1];
            var bytes = new List<byte>();

            while (true)
            {
                var read = await stream.ReadAsync(buff, 0, 1, cancellationToken).ConfigureAwait(false);

                if (read != 1)
                    throw new Exception("Failed to read a byte from the stream.");

                if (buff[0] == 0)
                    break;

                bytes.Add(buff[0]);
            }

            return Encoding.UTF8.GetString(bytes.ToArray());
        }

        internal static async Task<WinSize?> ReadWinSizeAsync([NotNull] this PipeStream stream,
            CancellationToken cancellationToken)
        {
            var buff = await stream.ReadExactAsync(Constants.WinSizeSize, cancellationToken);

            if (buff == null)
                return null;

            var winSize = new WinSize();

            unsafe
            {
                buff.WriteToPtr(winSize.Bytes);
            }

            return winSize;
        }

        // ReSharper disable once IdentifierTypo
        internal static async Task<Termios?> ReadTermiosAsync([NotNull] this PipeStream stream,
            CancellationToken cancellationToken)
        {
            var buff = await stream.ReadExactAsync(Constants.Termios_size, cancellationToken);

            if (buff == null)
                return null;

            var termios = new Termios();

            unsafe
            {
                buff.WriteToPtr(termios.Bytes);
            }

            return termios;
        }

        #endregion Stream helpers

        internal static string QuoteIfNeeded([NotNull] this string input)
        {
            input = input.Trim();

            if (!WhiteSpaceRx.IsMatch(input))
                return input;

            var firstChar = input.First();

            if ((firstChar == '\'' || firstChar == '"') && firstChar == input.Last())
                // Already quoted
                return input;

            if (!input.Contains('"'))
                return string.Concat("\"", input, "\"");

            if (!input.Contains('\''))
                return string.Concat("'", input, "'");

            return string.Concat("\"", input.Replace("\"", "\"\""), "\"");
        }

        internal static string FindMediatorExecutable(PtyBuild build)
        {
            var dirName =
                Path.GetDirectoryName(Assembly.GetEntryAssembly()?.Location ?? Directory.GetCurrentDirectory());

            if (dirName == null)
                return null;

            return FindFile(new DirectoryInfo(dirName),
                build == PtyBuild.Cygwin ? CygwinBuildExeName : Msys2BuildExeName);
        }

        private static string FindFile(DirectoryInfo dir, string fileName)
        {
            var exe = Path.Combine(dir.FullName, fileName);

            if (File.Exists(exe))
                return exe;

            foreach (var subDir in dir.GetDirectories())
            {
                exe = FindFile(subDir, fileName);

                if (exe != null)
                    return exe;
            }

            return null;
        }
    }
}
