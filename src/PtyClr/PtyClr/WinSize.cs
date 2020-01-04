using System.Runtime.InteropServices;
// ReSharper disable FieldCanBeMadeReadOnly.Global
// ReSharper disable MemberCanBePrivate.Global

namespace PtyClr
{
    [StructLayout(LayoutKind.Explicit)]
    public struct WinSize
    {
        [FieldOffset(0)]
        public ushort Width;

        [FieldOffset(2)]
        public ushort Height;

        [FieldOffset(0)]
        internal unsafe fixed byte Bytes[4];
    }
}
