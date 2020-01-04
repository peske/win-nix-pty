using System;
using System.Runtime.InteropServices;

// ReSharper disable InconsistentNaming
// ReSharper disable UnusedMember.Global
// ReSharper disable MemberCanBePrivate.Global
// ReSharper disable FieldCanBeMadeReadOnly.Global
// ReSharper disable IdentifierTypo

// ReSharper disable once CheckNamespace
namespace PtyClr
{
    internal partial class WinApi
    {
        // http://pinvoke.net/default.aspx/user32/GetKeyboardLayout.html
        [DllImport("user32.dll", SetLastError = true)]
        internal static extern IntPtr GetKeyboardLayout(uint idThread);

        // http://pinvoke.net/default.aspx/user32/GetKeyboardLayout.html
        [DllImport("user32.dll", SetLastError = true)]
        internal static extern uint MapVirtualKeyExW(uint uCode, MapType uMapType, IntPtr dwhkl);

        // http://pinvoke.net/default.aspx/kernel32/WriteConsole.html
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool WriteConsole(
            IntPtr hConsoleOutput,
            string lpBuffer,
            uint nNumberOfCharsToWrite,
            out uint lpNumberOfCharsWritten,
            IntPtr lpReserved
        );

        /* Writes data directly to the console input buffer. */
        [DllImport("kernel32.dll", EntryPoint = "WriteConsoleInputW", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern bool WriteConsoleInput(
            IntPtr hConsoleInput,
            INPUT_RECORD[] lpBuffer,
            uint nLength,
            out uint lpNumberOfEventsWritten);
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-mapvirtualkeyexw
    internal enum MapType : uint
    {
        /// <summary>
        /// The uCode parameter is a virtual-key code and is translated into a scan code. If it is a virtual-key code
        /// that does not distinguish between left- and right-hand keys, the left-hand scan code is returned. If there
        /// is no translation, the function returns 0.
        /// </summary>
        MAPVK_VK_TO_VSC = 0,

        /// <summary>
        /// The uCode parameter is a scan code and is translated into a virtual-key code that does not distinguish
        /// between left- and right-hand keys. If there is no translation, the function returns 0.
        /// </summary>
        MAPVK_VSC_TO_VK = 1, 

        /// <summary>
        /// The uCode parameter is a virtual-key code and is translated into an unshifted character value in the low
        /// order word of the return value. Dead keys (diacritics) are indicated by setting the top bit of the return
        /// value. If there is no translation, the function returns 0.
        /// </summary>
        MAPVK_VK_TO_CHAR = 2,

        /// <summary>
        /// The uCode parameter is a scan code and is translated into a virtual-key code that distinguishes between
        /// left- and right-hand keys. If there is no translation, the function returns 0. 
        /// </summary>
        MAPVK_VSC_TO_VK_EX = 3, 

        /// <summary>
        /// The uCode parameter is a virtual-key code and is translated into a scan code. If it is a virtual-key code
        /// that does not distinguish between left- and right-hand keys, the left-hand scan code is returned. If the
        /// scan code is an extended scan code, the high byte of the uCode value can contain either 0xe0 or 0xe1 to
        /// specify the extended scan code. If there is no translation, the function returns 0.
        /// </summary>
        MAPVK_VK_TO_VSC_EX = 4
    }

    // From wincontypes.h
    internal enum EventType : ushort
    {
        KEY_EVENT = 0x0001,
        MOUSE_EVENT = 0x0002,
        WINDOW_BUFFER_SIZE_EVENT = 0x0004
    }

    [StructLayout(LayoutKind.Explicit, CharSet = CharSet.Unicode)]
    internal struct INPUT_RECORD
    {
        [FieldOffset(0)]
        internal EventType EventType;

        // These are a union
        [FieldOffset(2)]
        internal KEY_EVENT_RECORD KeyEvent;

        [FieldOffset(2)]
        internal MOUSE_EVENT_RECORD MouseEvent;

        [FieldOffset(2)]
        internal WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;

        /*
        MENU_EVENT_RECORD MenuEvent;
        FOCUS_EVENT_RECORD FocusEvent; */
        // MSDN claims that these are used internally and shouldn't be used
        // https://docs.microsoft.com/en-us/windows/console/input-record-str

        // Added for serialization
        [FieldOffset(0)]
        internal unsafe fixed byte Bytes[Constants.INPUT_RECORD_size];
    }

    // Struct checked by FD.
    [StructLayout(LayoutKind.Sequential)]
    public struct COORD
    {
        public short X;
        public short Y;
    }

    [Flags]
    public enum MouseButtonState : uint
    {
        /// <summary>
        /// The leftmost mouse button.
        /// </summary>
        FROM_LEFT_1ST_BUTTON_PRESSED = 0x0001,

        /// <summary>
        /// The rightmost mouse button.
        /// </summary>
        RIGHTMOST_BUTTON_PRESSED = 0x0002,

        /// <summary>
        /// The second button from the left.
        /// </summary>
        FROM_LEFT_2ND_BUTTON_PRESSED = 0x0004,

        /// <summary>
        /// The third button from the left.
        /// </summary>
        FROM_LEFT_3RD_BUTTON_PRESSED = 0x0008,

        /// <summary>
        /// The fourth button from the left.
        /// </summary>
        FROM_LEFT_4TH_BUTTON_PRESSED = 0x0010
    }

    [Flags]
    public enum ControlKeyState : uint
    {
        /// <summary>
        /// The right ALT key is pressed.
        /// </summary>
        RIGHT_ALT_PRESSED = 0x0001,

        /// <summary>
        /// The left ALT key is pressed.
        /// </summary>
        LEFT_ALT_PRESSED = 0x0002,

        /// <summary>
        /// The right CTRL key is pressed.
        /// </summary>
        RIGHT_CTRL_PRESSED = 0x0004,

        /// <summary>
        /// The left CTRL key is pressed.
        /// </summary>
        LEFT_CTRL_PRESSED = 0x0008,

        /// <summary>
        /// The SHIFT key is pressed.
        /// </summary>
        SHIFT_PRESSED = 0x0010,

        /// <summary>
        /// The NUM LOCK light is on.
        /// </summary>
        NUMLOCK_ON = 0x0020,

        /// <summary>
        /// The SCROLL LOCK light is on.
        /// </summary>
        SCROLLLOCK_ON = 0x0040,

        /// <summary>
        /// The CAPS LOCK light is on.
        /// </summary>
        CAPSLOCK_ON = 0x0080,

        /// <summary>
        /// The key is enhanced.
        /// </summary>
        ENHANCED_KEY = 0x0100
    }

    [Flags]
    public enum MouseEventFlags : uint
    {
        /// <summary>
        /// A change in mouse position occurred.
        /// </summary>
        MOUSE_MOVED = 0x0001,

        /// <summary>
        /// The second click (button press) of a double-click occurred. The first click is returned
        /// as a regular button-press event.
        /// </summary>
        DOUBLE_CLICK = 0x0002,

        /// <summary>
        /// The horizontal mouse wheel was moved.
        /// </summary>
        /// <remarks>
        /// If the high word of the <see cref="MOUSE_EVENT_RECORD.dwButtonState"/> member contains a positive
        /// value, the wheel was rotated to the right. Otherwise, the wheel was rotated to the left.
        /// </remarks>
        MOUSE_HWHEELED = 0x0008,

        /// <summary>
        /// The vertical mouse wheel was moved.
        /// </summary>
        /// <remarks>
        /// If the high word of the dwButtonState member contains a positive value, the wheel was rotated forward,
        /// away from the user. Otherwise, the wheel was rotated backward, toward the user.
        /// </remarks>
        MOUSE_WHEELED = 0x0004
    }

    // Struct checked by FD.
    [StructLayout(LayoutKind.Explicit)]
    public struct MOUSE_EVENT_RECORD
    {
        [FieldOffset(0)]
        public COORD dwMousePosition;

        [FieldOffset(4)]
        public MouseButtonState dwButtonState;

        [FieldOffset(8)]
        public ControlKeyState dwControlKeyState;

        [FieldOffset(12)]
        public MouseEventFlags dwEventFlags;
    }

	
	// Struct checked by FD.
	[StructLayout(LayoutKind.Explicit, CharSet = CharSet.Unicode)]
    public struct KEY_EVENT_RECORD
    {
        [FieldOffset(0)]
        public int bKeyDown;

        [FieldOffset(4)]
        public ushort wRepeatCount;

        [FieldOffset(6)]
        public VirtualKey wVirtualKeyCode;

        [FieldOffset(8)]
        public ushort wVirtualScanCode;

        [FieldOffset(10)]
        public char UnicodeChar;

        [FieldOffset(10)]
        public byte AsciiChar;

        [FieldOffset(12)]
        public ControlKeyState dwControlKeyState;
    }

    // Struct checked by FD.
    [StructLayout(LayoutKind.Explicit)]
    public struct WINDOW_BUFFER_SIZE_RECORD
    {
        [FieldOffset(0)]
        public COORD dwSize;
    }
}