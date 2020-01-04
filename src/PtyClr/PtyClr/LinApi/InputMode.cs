using System;
// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global
// ReSharper disable IdentifierTypo
// ReSharper disable CommentTypo

namespace PtyClr.LinApi
{
    // Enum values are gotten from sys/termios.h header file.
    /// <summary>
    /// Used in <see cref="Termios.c_iflag"/> field of <see cref="Termios"/> struct. For more details see
    /// <see cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</see>.
    /// </summary>
    /// <seealso cref="Termios"/>
    /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
    /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#Input-Modes">Input Modes</seealso>
    [Flags]
    public enum InputMode : uint
    {
        /// <summary>
        /// If this bit is set, break conditions are ignored. For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IGNBRK">Macro: tcflag_t IGNBRK</see>.
        /// </summary>
        /// <seealso cref="BRKINT"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IGNBRK">Macro: tcflag_t IGNBRK</seealso>
        IGNBRK = 0x00001,

        /// <summary>
        /// If this bit is set and <see cref="IGNBRK"/> is not set, a break condition clears the terminal input
        /// and output queues and raises a <c>SIGINT</c> signal for the foreground process group associated with
        /// the terminal. For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-BRKINT">Macro: tcflag_t BRKINT</see>.
        /// </summary>
        /// <seealso cref="IGNBRK"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-BRKINT">Macro: tcflag_t BRKINT</seealso>
        BRKINT = 0x00002,

        /// <summary>
        /// If this bit is set, any byte with a framing or parity error is ignored. This is only useful if
        /// <see cref="INPCK"/> is also set. 
        /// </summary>
        /// <seealso cref="INPCK"/>
        /// <seealso cref="PARMRK"/> 
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IGNPAR">Macro: tcflag_t IGNPAR</seealso>
        IGNPAR = 0x00004,

        /// <summary>
        /// If this bit is set, then filling up the terminal input buffer sends a <c>BEL</c> character (code <c>007</c>)
        /// to the terminal to ring the bell. This is a BSD extension.
        /// </summary>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IMAXBEL">Macro: tcflag_t IMAXBEL</seealso>
        IMAXBEL = 0x00008,

        /// <summary>
        /// If this bit is set, input parity checking is enabled. If it is not set, no checking at all is done for
        /// parity errors on input; the characters are simply passed through to the application. For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-INPCK">Macro: tcflag_t INPCK</see>.
        /// </summary>
        /// <seealso cref="IGNPAR"/>
        /// <seealso cref="PARMRK"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-INPCK">Macro: tcflag_t INPCK</seealso>
        INPCK = 0x00010,

        /// <summary>
        /// If this bit is set, valid input bytes are stripped to seven bits; otherwise, all eight bits are available
        /// for programs to read.
        /// </summary>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-ISTRIP">Macro: tcflag_t ISTRIP</seealso>
        ISTRIP = 0x00020,

        /// <summary>
        /// If this bit is set, newline characters (<c>'\n'</c>) received as input are passed to the application as
        /// carriage return characters (<c>'\r'</c>). 
        /// </summary>
        /// <seealso cref="IGNCR"/>
        /// <seealso cref="ICRNL"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-INLCR">Macro: tcflag_t INLCR</seealso>
        INLCR = 0x00040,

        /// <summary>
        /// If this bit is set, carriage return characters (<c>'\r'</c>) are discarded on input. Discarding carriage
        /// return may be useful on terminals that send both carriage return and linefeed when you type the <c>RET</c> key. 
        /// </summary>
        /// <seealso cref="INLCR"/>
        /// <seealso cref="ICRNL"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IGNCR">Macro: tcflag_t IGNCR</seealso>
        IGNCR = 0x00080,

        /// <summary>
        /// If this bit is set and <see cref="IGNCR"/> is not set, carriage return characters (<c>'\r'</c>) received
        /// as input are passed to the application as newline characters (<c>'\n'</c>). 
        /// </summary>
        /// <seealso cref="INLCR"/>
        /// <seealso cref="IGNCR"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-ICRNL">Macro: tcflag_t ICRNL</seealso>
        ICRNL = 0x00100,

        /// <summary>
        /// If this bit is set, start/stop control on output is enabled. For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IXON">Macro: tcflag_t IXON</see>.
        /// </summary>
        /// <seealso cref="IXOFF"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IXON">Macro: tcflag_t IXON</seealso>
        IXON = 0x00400,

        /// <summary>
        /// If this bit is set, start/stop control on input is enabled. For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IXOFF">Macro: tcflag_t IXOFF</see>.
        /// </summary>
        /// <seealso cref="IXON"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IXOFF">Macro: tcflag_t IXOFF</seealso>
        IXOFF = 0x01000,

        /// <summary>
        /// (not in POSIX) Map uppercase characters to lowercase on input.
        /// </summary>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        IUCLC = 0x04000,

        /// <summary>
        /// If this bit is set, any input character restarts output when output has been suspended with the STOP
        /// character. Otherwise, only the START character restarts output. For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IXANY">Macro: tcflag_t IXANY</see>.
        /// </summary>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-IXANY">Macro: tcflag_t IXANY</seealso>
        IXANY = 0x08000,

        /// <summary>
        /// If this bit is set, input bytes with parity or framing errors are marked when passed to the program. This bit is meaningful
        /// only when <see cref="INPCK"/> is set and <see cref="IGNPAR"/> is not set.  For more details see
        /// <see cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-PARMRK">Macro: tcflag_t PARMRK</see>.
        /// </summary>
        /// <seealso cref="INPCK"/>
        /// <seealso cref="IGNPAR"/>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        /// <seealso cref="!:https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html#index-PARMRK">Macro: tcflag_t PARMRK</seealso>
        PARMRK = 0x10000,

        /// <summary>
        /// (since Linux 2.6.4, not in POSIX) Input is UTF8; this allows character-erase to be correctly performed
        /// in cooked mode.
        /// </summary>
        /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
        IUTF8 = 0x20000
    }
}
