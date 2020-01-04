using System.Runtime.InteropServices;

// ReSharper disable FieldCanBeMadeReadOnly.Global
// ReSharper disable MemberCanBePrivate.Global

// ReSharper disable IdentifierTypo

namespace PtyClr.LinApi
{
    [StructLayout(LayoutKind.Explicit)]
    public struct Termios
    {
        /// <summary>
        /// Defines input mode. See <see cref="InputMode"/> for more details.
        /// </summary>
        /// <seealso cref="InputMode"/>
        [FieldOffset(0)]
        public InputMode c_iflag;

        /// <summary>
        /// Defines output mode. See <see cref="OutputMode"/> for more details.
        /// </summary>
        /// <seealso cref="OutputMode"/>
        [FieldOffset(4)]
        public OutputMode c_oflag;

        /// <summary>
        /// Defines control mode. See <see cref="ControlMode"/> for more details.
        /// </summary>
        /// <seealso cref="ControlMode"/>
        [FieldOffset(8)]
        public ControlMode c_cflag;

        /// <summary>
        /// Defines local mode. See <see cref="LocalMode"/> for more details.
        /// </summary>
        /// <seealso cref="LocalMode"/>
        [FieldOffset(12)]
        public LocalMode c_lflag;

        /// <summary>
        /// Line discipline. Not used in POSIX compliant systems.
        /// </summary>
        [FieldOffset(16)]
        public byte c_line;

        /// <summary>
        /// To understand members of this array see constants in <see cref="Cc"/> class, which are representing
        /// named indices of this array.
        /// </summary>
        /// <seealso cref="Cc"/>
        [FieldOffset(17)]
        public unsafe fixed byte c_cc[Cc.NCCS];

        /// <summary>
        /// Input baud rate. See <see cref="BaudRate"/> for more details.
        /// </summary>
        /// <seealso cref="BaudRate"/>
        [FieldOffset(36)]
        public BaudRate c_ispeed;

        /// <summary>
        /// Output baud rate. See <see cref="BaudRate"/> for more details.
        /// </summary>
        /// <seealso cref="BaudRate"/>
        [FieldOffset(40)]
        public BaudRate c_ospeed;

        [FieldOffset(0)]
        internal unsafe fixed byte Bytes[Constants.Termios_size];
    }
}
