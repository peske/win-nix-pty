using System;
// ReSharper disable CommentTypo
// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global
// ReSharper disable IdentifierTypo

namespace PtyClr.LinApi
{
    // Enum values are gotten from sys/termios.h header file.
    /// <summary>
    /// Used in <see cref="Termios.c_lflag"/> field of <see cref="Termios"/> struct. For more details see
    /// <see cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</see>.
    /// </summary>
    /// <seealso cref="Termios"/>
    /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
    /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Local-Modes.html#Local-Modes">Local Modes</seealso>
    [Flags]
    public enum LocalMode : uint
    {
        ISIG = 0x0001,
        ICANON = 0x0002,
        ECHO = 0x0004,
        ECHOE = 0x0008,
        ECHOK = 0x0010,
        ECHONL = 0x0020,
        NOFLSH = 0x0040,
        TOSTOP = 0x0080,
        IEXTEN = 0x0100,
        FLUSHO = 0x0200,
        ECHOKE = 0x0400,
        ECHOCTL = 0x0800
    }
}
