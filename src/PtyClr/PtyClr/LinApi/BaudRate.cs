// ReSharper disable UnusedMember.Global
// ReSharper disable CommentTypo
namespace PtyClr.LinApi
{
    // Enum values are gotten from sys/termios.h header file.
    /// <summary>
    /// Used in <see cref="Termios.c_ispeed"/> and <see cref="Termios.c_ospeed"/> fields of <see cref="Termios"/>
    /// struct. For more details see <see cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</see>.
    /// </summary>
    /// <seealso cref="Termios"/>
    /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
    public enum BaudRate : uint
    {
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
        B38400 = 0x0000f
    }
}
