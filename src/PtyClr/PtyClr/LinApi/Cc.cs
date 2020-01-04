// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global
// ReSharper disable IdentifierTypo
// ReSharper disable CommentTypo
namespace PtyClr.LinApi
{
    // Enum values are gotten from sys/termios.h header file.
    /// <summary>
    /// Constants exposed by this class can be used as <em>named indices</em> in <see cref="Termios.c_cc"/> array.
    /// <see cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</see>.
    /// </summary>
    /// <seealso cref="Termios"/>
    /// <seealso cref="!:http://man7.org/linux/man-pages/man3/termios.3.html">termios(3)</seealso>
    public static class Cc
    {
        public const int VDISCARD = 1;
        public const int VEOL = 2;
        public const int VEOL2 = 3;
        public const int VEOF = 4;
        public const int VERASE = 5;
        public const int VINTR = 6;
        public const int VKILL = 7;
        public const int VLNEXT = 8;
        public const int VMIN = 9;
        public const int VQUIT = 10;
        public const int VREPRINT = 11;
        public const int VSTART = 12;
        public const int VSTOP = 13;
        public const int VSUSP = 14;
        public const int VSWTC = 15;
        public const int VTIME = 16;
        public const int VWERASE = 17;

        internal const int NCCS = 18;
    }
}
