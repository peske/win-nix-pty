using System.Diagnostics;
using PtyClr.LinApi;
// ReSharper disable IdentifierTypo
// ReSharper disable InconsistentNaming

namespace PtyClr
{
    internal static class Constants
    {
        internal const int WinSizeSize = 4;

        // From command_processor.cpp: #define termios_size 44
        internal const int Termios_size = 44;

        // From io_processor.cpp: #define KEY_EVENT_RECORD_size 16
        private const int KEY_EVENT_RECORD_size = 16;

        // From io_processor.cpp: #define COORD_size 4
        private const int COORD_size = 4;

        // From io_processor.cpp: #define WINDOW_BUFFER_SIZE_RECORD_size 4
        private const int WINDOW_BUFFER_SIZE_RECORD_size = 4;

        // From io_processor.cpp: #define MOUSE_EVENT_RECORD_size 16
        private const int MOUSE_EVENT_RECORD_size = 16;

        // From io_processor.cpp: #define INPUT_RECORD_size 20
        internal const int INPUT_RECORD_size = 20;

        internal static unsafe void DoAsserts()
        {
            Debug.Assert(sizeof(WinSize) == WinSizeSize);

            Debug.Assert(sizeof(Termios) == Termios_size);
            Debug.Assert(sizeof(InputMode) == 4);
            Debug.Assert(sizeof(OutputMode) == 4);
            Debug.Assert(sizeof(ControlMode) == 4);
            Debug.Assert(sizeof(LocalMode) == 4);
            Debug.Assert(sizeof(BaudRate) == 4);

            Debug.Assert(sizeof(KEY_EVENT_RECORD) == KEY_EVENT_RECORD_size);
            Debug.Assert(sizeof(COORD) == COORD_size);
            Debug.Assert(sizeof(WINDOW_BUFFER_SIZE_RECORD) == WINDOW_BUFFER_SIZE_RECORD_size);
            Debug.Assert(sizeof(MOUSE_EVENT_RECORD) == MOUSE_EVENT_RECORD_size);
            Debug.Assert(sizeof(INPUT_RECORD) == INPUT_RECORD_size);
        }
    }
}
