using System;
// ReSharper disable CommentTypo
// ReSharper disable InconsistentNaming
// ReSharper disable IdentifierTypo
// ReSharper disable UnusedMember.Global

namespace PtyClr.LinApi
{
    // Enum values are gotten from sys/termios.h header file.
    /// <summary>
    /// Used in <see cref="Termios.c_cflag"/> field of <see cref="Termios"/> struct. For more details see
    /// <see cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</see>.
    /// </summary>
    /// <remarks>
    /// <note type="warn">Note that baud rate values defined by this enum (<see cref="B0"/> -
    /// <see cref="B38400"/>), actually aren't <em>flags</em>, meaning that you <strong>should not</strong>
    /// specify more than one of these value. You'll recognize these easily - all their names start with
    /// <c>B</c>, and all have their match (the same name/value) in <see cref="BaudRate"/> enum.</note> 
    /// </remarks>
    /// <seealso cref="Termios"/>
    /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
    /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Control-Modes.html#Control-Modes">Control Modes</seealso>
    [Flags]
    public enum ControlMode : uint
    {
        CBAUD = 0x0100f,
        B0 = 0x00000,
        B50 = 0x00001,
        B75 = 0x00002,
        B110 = 0x00003,
        B134 = 0x00004,
        B150 = 0x00005,
        B200 = 0x00006,
        B300 = 0x00007,
        B600 = 0x00008,
        B1200 = 0x00009,
        B1800 = 0x0000a,
        B2400 = 0x0000b,
        B4800 = 0x0000c,
        B9600 = 0x0000d,
        B19200 = 0x0000e,
        B38400 = 0x0000f,

        CSIZE = 0x00030,
        CS5 = 0x00000,
        CS6 = 0x00010,
        CS7 = 0x00020,
        CS8 = 0x00030,
        CSTOPB = 0x00040,
        CREAD = 0x00080,
        PARENB = 0x00100,
        PARODD = 0x00200,
        HUPCL = 0x00400,
        CLOCAL = 0x00800,
    }
}
