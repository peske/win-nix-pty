using System;
// ReSharper disable CommentTypo
// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global
// ReSharper disable IdentifierTypo

namespace PtyClr.LinApi
{
    // Enum values are gotten from sys/termios.h header file.
    /// <summary>
    /// Used in <see cref="Termios.c_oflag"/> field of <see cref="Termios"/> struct. For more details see
    /// <see cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</see>.
    /// </summary>
    /// <seealso cref="Termios"/>
    /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
    /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Output-Modes.html#Output-Modes">Output Modes</seealso>
    [Flags]
    public enum OutputMode : uint
    {
        OPOST = 0x00001,
        OLCUC = 0x00002,
        OCRNL = 0x00004,
        ONLCR = 0x00008,
        ONOCR = 0x00010,
        ONLRET = 0x00020,
        OFILL = 0x00040,
        CRDLY = 0x00180,
        CR0 = 0x00000,
        CR1 = 0x00080,
        CR2 = 0x00100,
        CR3 = 0x00180,
        NLDLY = 0x00200,
        NL0 = 0x00000,
        NL1 = 0x00200,
        BSDLY = 0x00400,
        BS0 = 0x00000,
        BS1 = 0x00400,
        TABDLY = 0x01800,
        TAB0 = 0x00000,
        TAB1 = 0x00800,
        TAB2 = 0x01000,
        TAB3 = 0x01800,
        XTABS = 0x01800,
        VTDLY = 0x02000,
        VT0 = 0x00000,
        VT1 = 0x02000,
        FFDLY = 0x04000,
        FF0 = 0x00000,
        FF1 = 0x04000,
        OFDEL = 0x08000,
    }
}
