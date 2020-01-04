using System;
using System.Runtime.InteropServices;
// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global

// ReSharper disable once CheckNamespace
namespace PtyClr
{
    internal enum StdHandle
    {
        STD_INPUT_HANDLE = -10,
        STD_OUTPUT_HANDLE = -11,
        STD_ERROR_HANDLE = -12
    }

    internal partial class WinApi
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern IntPtr GetStdHandle(StdHandle nStdHandle);
    }
}