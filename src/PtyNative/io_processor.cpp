/*
 Created by Fat Dragon on 12/14/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#include "io_processor.h"

#include "command_processor.h"
#include "file_helpers.h"
#include "helpers.h"
#include "logging.h"
#include "stand_alone_io.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#define READ_LOOP_TIMEOUT 20000
#define PTY_BUFFER_SIZE 4096
#define INPUT_RECORDS_PER_CYCLE 100
#define IO_ERRCOUNT_IGNORE 2
#define HEART_BEAT_CYCLES 500

// Static asserts, to assure that the structs aren't changed in the future
#ifndef FROM_CLION_CMAKE
#define KEY_EVENT_RECORD_size 16
static_assert(sizeof(KEY_EVENT_RECORD) == KEY_EVENT_RECORD_size);
static_assert(offsetof(KEY_EVENT_RECORD, bKeyDown) == 0);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->bKeyDown) == 4);
static_assert(offsetof(KEY_EVENT_RECORD, wRepeatCount) == 4);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->wRepeatCount) == 2);
static_assert(offsetof(KEY_EVENT_RECORD, wVirtualKeyCode) == 6);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->wVirtualKeyCode) == 2);
static_assert(offsetof(KEY_EVENT_RECORD, wVirtualScanCode) == 8);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->wVirtualScanCode) == 2);
static_assert(offsetof(KEY_EVENT_RECORD, uChar) == 10);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->uChar) == 2);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->uChar.UnicodeChar) == 2);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->uChar.AsciiChar) == 1);
static_assert(offsetof(KEY_EVENT_RECORD, dwControlKeyState) == 12);
static_assert(sizeof(((KEY_EVENT_RECORD*)nullptr)->dwControlKeyState) == 4);

#define COORD_size 4
static_assert(sizeof(COORD) == COORD_size);
static_assert(offsetof(COORD, X) == 0);
static_assert(sizeof(((COORD*)nullptr)->X) == 2);
static_assert(offsetof(COORD, Y) == 2);
static_assert(sizeof(((COORD*)nullptr)->Y) == 2);

#define WINDOW_BUFFER_SIZE_RECORD_size 4
static_assert(sizeof(WINDOW_BUFFER_SIZE_RECORD) == WINDOW_BUFFER_SIZE_RECORD_size);
static_assert(offsetof(WINDOW_BUFFER_SIZE_RECORD, dwSize) == 0);
static_assert(sizeof(((WINDOW_BUFFER_SIZE_RECORD*)nullptr)->dwSize) == COORD_size);

#define MOUSE_EVENT_RECORD_size 16
static_assert(sizeof(MOUSE_EVENT_RECORD) == MOUSE_EVENT_RECORD_size);
static_assert(offsetof(MOUSE_EVENT_RECORD, dwMousePosition) == 0);
static_assert(sizeof(((MOUSE_EVENT_RECORD*)nullptr)->dwMousePosition) == COORD_size);
static_assert(offsetof(MOUSE_EVENT_RECORD, dwButtonState) == 4);
static_assert(sizeof(((MOUSE_EVENT_RECORD*)nullptr)->dwButtonState) == 4);
static_assert(offsetof(MOUSE_EVENT_RECORD, dwControlKeyState) == 8);
static_assert(sizeof(((MOUSE_EVENT_RECORD*)nullptr)->dwControlKeyState) == 4);
static_assert(offsetof(MOUSE_EVENT_RECORD, dwEventFlags) == 12);
static_assert(sizeof(((MOUSE_EVENT_RECORD*)nullptr)->dwEventFlags) == 4);

#define INPUT_RECORD_size 20
static_assert(sizeof(INPUT_RECORD) == INPUT_RECORD_size);
static_assert(offsetof(INPUT_RECORD, EventType) == 0);
static_assert(sizeof(((INPUT_RECORD*)nullptr)->EventType) == 2);
static_assert(offsetof(INPUT_RECORD, Event) == 4);
static_assert(sizeof(((INPUT_RECORD*)nullptr)->Event) == KEY_EVENT_RECORD_size);
static_assert(sizeof(((INPUT_RECORD*)nullptr)->Event.KeyEvent) == KEY_EVENT_RECORD_size);
static_assert(sizeof(((INPUT_RECORD*)nullptr)->Event.MouseEvent) == MOUSE_EVENT_RECORD_size);
static_assert(sizeof(((INPUT_RECORD*)nullptr)->Event.WindowBufferSizeEvent) == WINDOW_BUFFER_SIZE_RECORD_size);
#endif

static bool _something_happened{false};
static int _nothing_happened_count{0};

static bool process_active(int slave_pid) {
    if (kill(slave_pid, 0) != 0) {
        log_lin_error(LOG_WARN, "[process_active] 'kill' failed.");
        return false;
    }
    return true;
}

static bool slave_process_running(int slave_pid) {
    int status{0};
    const auto wait_rc = waitpid(slave_pid, &status, WNOHANG);
    if (wait_rc != slave_pid)
        return true;
    if (_min_log_level <= LOG_INFO) {
        if (WIFEXITED(status)) {
            const auto exit_code = WEXITSTATUS(status);
            logf(LOG_WARN, "[slave_process_running] Slave process (PID=%i) terminated. Exit code: %i (%s)",
                 slave_pid, exit_code, strerror(exit_code));
        }
        else if (WIFSIGNALED(status)) {
            const auto sig = WTERMSIG(status);
            logf(LOG_WARN, "[slave_process_running] Slave process (PID=%i) terminated by signal (%i): %s",
                 slave_pid, sig, strsignal(sig));
        }
        else
            logf(LOG_WARN, "[slave_process_running] Slave process (PID=%i) terminated. Status: %i", slave_pid,
                 status);
    }
    return false;
}

static bool write_output(HANDLE h_out, char* buff, int length) {
    return h_out == nullptr
           ? write_output_to_console(buff, length) // Stand-alone mode
           : write_bytes(h_out, buff, length); // Managed mode
}

// Keeping output buffer at root level so that we can try again in the next cycle if the processing fails.
static char output_buffer[PTY_BUFFER_SIZE];
static int output_buffer_count{0};

static bool process_output(HANDLE h_out, int pty_fd, bool& exhausted) {
    exhausted = false;
    const auto write_old = output_buffer_count > 0;
    if (!write_old) {
        fd_set fds{};
        FD_ZERO(&fds);
        FD_SET(pty_fd, &fds);
        timeval timeout{.tv_sec=0, .tv_usec=READ_LOOP_TIMEOUT};
        const auto result = select(pty_fd + 1, &fds, nullptr, nullptr, &timeout);
        if (result < 0) {
            log_lin_error(LOG_ERROR, "[process_output] 'select' call failed.");
            return false;
        }
        if (result > 0 && FD_ISSET(pty_fd, &fds)) {
            _something_happened = true;
            log(LOG_TRACE, "[process_output] There's something to read from PTY or it's closed.");
            int len = read(pty_fd, output_buffer, PTY_BUFFER_SIZE);
            if (len < 0) {
                log_lin_error(LOG_ERROR, "[process_output] 'read' call failed.");
                return false;
            }
            output_buffer_count = len;
            exhausted = len < PTY_BUFFER_SIZE;
            if (len > 0)
                logf(LOG_TRACE, "[process_output] 'read' returned %i bytes.", output_buffer_count);
        } else
            exhausted = true;
    }
    if (output_buffer_count > 0) {
        _something_happened = true;
        logf(LOG_TRACE, "[process_output] Trying to write %i bytes.", output_buffer_count);
        if (!write_output(h_out, output_buffer, output_buffer_count)) {
            logf(LOG_WARN, "[process_output] Failed to write %i bytes to output.", output_buffer_count);
            return false;
        }
        logf(LOG_TRACE, "[process_output] %i bytes successfully written to output.", output_buffer_count);
    }
    output_buffer_count = 0;
    return write_old ? process_output(h_out, pty_fd, exhausted) : true;
}

// Used in managed mode.
static bool read_input_records_from_pipe(HANDLE pipe, INPUT_RECORD* records, int count, int& records_read) {
    records_read = 0;
    auto pos = records;
    while (records_read < count) {
        DWORD bytes_read{0};
        if (!try_read_bytes_fixed(pipe, reinterpret_cast<char *>(pos), sizeof(INPUT_RECORD), &bytes_read))
            return false;
        if (bytes_read < sizeof(INPUT_RECORD))
            return true;
        ++records_read;
        ++pos;
    }
    return true;
}

static bool read_input_records(HANDLE h_out, HANDLE h_in_rec, INPUT_RECORD* records, int count, int& records_read) {
    if (h_out != nullptr) {
        // Managed mode
        if (h_in_rec == nullptr) {
            // Record-by-record reading not used.
            records_read = 0;
            return true;
        }
        return read_input_records_from_pipe(h_in_rec, records, count, records_read);
    }
    return read_input_records_from_console(records, count, records_read);
}

static bool process_input_record(int pty_fd, HANDLE h_out, INPUT_RECORD& record) {
    switch (record.EventType) {
        case WINDOW_BUFFER_SIZE_EVENT: {
            logf(LOG_DEBUG, "[process_input_record] WINDOW_BUFFER_SIZE_EVENT received: %i cols x %i rows.",
                 record.Event.WindowBufferSizeEvent.dwSize.X, record.Event.WindowBufferSizeEvent.dwSize.Y);
            winsize win_size{};
            win_size.ws_col = (unsigned short) record.Event.WindowBufferSizeEvent.dwSize.X;
            win_size.ws_row = (unsigned short) record.Event.WindowBufferSizeEvent.dwSize.Y;
            if (h_out == nullptr)
                // We are in standalone mode, so we should better query actual window size than rely on the input
                try_override_win_size(win_size);
            if (ioctl(pty_fd, TIOCSWINSZ, &win_size) == 0)
                return true;
            log_lin_error(LOG_ERROR, "[process_input_record] 'ioctl' call failed.");
            return false;
        }
        case KEY_EVENT: {
            if (!record.Event.KeyEvent.bKeyDown) {
                logf(LOG_DEBUG, "[process_input_record] KEY_EVENT received (%i), but bKeyDown is FALSE, so ignoring.",
                     record.Event.KeyEvent.uChar.UnicodeChar);
                return true;
            }
            // Special handling for Ctrl+Space
            if (record.Event.KeyEvent.wVirtualKeyCode == VK_SPACE &&
                (record.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))) {
                static const char zero_byte{0};
                const auto written = write(pty_fd, &zero_byte, 1);
                if (written != 1) {
                    log_lin_error(LOG_ERROR, "[process_input_record] 'write' call failed. Failed to write zero byte");
                    return false;
                }
                return true;
            }
            if (record.Event.KeyEvent.uChar.UnicodeChar == 0) {
                log(LOG_DEBUG,
                    "[process_input_record] KEY_EVENT received, but will be ignored since UnicodeChar == 0.");
                return true;
            }
            // Processing the key
            char *char_string{nullptr};
            bool success = wchar_to_char_string(CP_UTF8, &record.Event.KeyEvent.uChar.UnicodeChar, &char_string, 1);
            if (success) {
                success = write_exact(pty_fd, char_string);
                if (!success)
                    log(LOG_ERROR, "[process_input_record] Failed to write converted UnicodeChar.");
            } else
                logf(LOG_ERROR, "[process_input_record] Failed to convert UnicodeChar %i.",
                     record.Event.KeyEvent.uChar.UnicodeChar);
            if (char_string)
                free(char_string);
            return success;
        }
        default: {
            logf(LOG_WARN, "[process_input_record] Event of type %i received, and will be ignored.", record.EventType);
            return true;
        }
    }
}

// Keeping records at root level so that we can try again in the next cycle if the processing fails.
static INPUT_RECORD records[INPUT_RECORDS_PER_CYCLE];
static int record_index{0};
static int record_count{0};

static bool process_input_records(int pty_fd, HANDLE h_out, HANDLE h_in_rec, bool& exhausted) {
    exhausted = false;
    const auto write_old = record_count > 0;
    if (!write_old) {
        int records_read{0};
        if (!read_input_records(h_out, h_in_rec, records, INPUT_RECORDS_PER_CYCLE, records_read))
            return false;
        record_count = records_read;
        record_index = 0;
        exhausted = records_read < INPUT_RECORDS_PER_CYCLE;
    }
    if (record_index < record_count)
        _something_happened = true;
    while (record_index < record_count) {
        if (!process_input_record(pty_fd, h_out, records[record_index]))
            return false;
        ++record_index;
    }
    record_count = 0;
    record_index = 0;
    return write_old ? process_input_records(pty_fd, h_out, h_in_rec, exhausted) : true;
}

// Keeping output buffer at root level so that we can try again in the next cycle if the processing fails.
static char input_buffer[PTY_BUFFER_SIZE];
static int input_buffer_count{0};
static int input_buffer_index{0};

static bool process_input(int pty_fd, HANDLE h_in) {
    const auto write_old = input_buffer_count > 0;
    if (!write_old) {
        DWORD read{0};
        if (!try_read_bytes(h_in, input_buffer, PTY_BUFFER_SIZE, &read))
            return false;
        input_buffer_count = (int) read;
        input_buffer_index = 0;
        if (input_buffer_count > 0)
            logf(LOG_TRACE, "[process_input] %i bytes read from the input stream.", input_buffer_count);
    }
    if (input_buffer_count > 0) {
        _something_happened = true;
        logf(LOG_TRACE, "[process_input] Attempt writing %i bytes to PTY.", input_buffer_count - input_buffer_index);
        auto bytes_written = write(pty_fd, input_buffer + input_buffer_index,
                                   input_buffer_count - input_buffer_index);
        if (bytes_written <= 0) {
            log_lin_error(LOG_ERROR, "[process_input] 'write' call failed.");
            return false;
        }
        logf(LOG_TRACE, "process_input] %i bytes written to PTY.", bytes_written);
        input_buffer_index += bytes_written;
        if (input_buffer_index == input_buffer_count) {
            // Everything written
            input_buffer_index = 0;
            input_buffer_count = 0;
        }
    }
    return write_old && input_buffer_index == 0 ? process_input(pty_fd, h_in) : true;
}

void run(int pty_fd, int slave_pid, HANDLE h_in, HANDLE h_in_rec, HANDLE h_out, HANDLE h_cin, HANDLE h_cout) {
    log(LOG_INFO, "[run] Event loop started.");
    if (h_out == nullptr)
        // In stand-alone mode we need to do:
        disable_processed_input();
    auto process_output_error_counter{0};
    auto process_input_records_error_counter{0};
    auto process_input_stream_error_counter{0};
    while(true) {
        if (!process_active(slave_pid)) {
            log(LOG_WARN, "[run] 'process_active' returned false.");
            break;
        }
        if (!slave_process_running(slave_pid)) {
            log(LOG_WARN, "[run] 'slave_process_running' returned false.");
            break;
        }
#if (HEART_BEAT_CYCLES > 0)
        if (_something_happened)
            _nothing_happened_count = 0;
        else if (++_nothing_happened_count > HEART_BEAT_CYCLES) {
            logf(LOG_TRACE, "[run] Nothing happened in the last %i passes through I/O loop.", HEART_BEAT_CYCLES);
            _nothing_happened_count = 0;
            //test();
        }
        _something_happened = false;
#endif
        // Processing commands:
        if (!process_commands(pty_fd, h_cin, h_cout)) {
            // Here we cannot ignore errors (command streams may be corrupted)
            log(LOG_ERROR, "[run] Failed to process commands. Exiting.");
            break;
        }
        // Processing slave process output:
        bool slave_output_exhausted{true};
        if (process_output(h_out, pty_fd, slave_output_exhausted))
            process_output_error_counter = 0;
        else if (++process_output_error_counter > IO_ERRCOUNT_IGNORE) {
            logf(LOG_WARN, "[run] Failed to process output in %i attempts. Exiting.", IO_ERRCOUNT_IGNORE);
            break;
        } else {
            _something_happened = true;
            usleep(10000);
            continue;
        }
        if (!slave_output_exhausted)
            logf(LOG_TRACE, "[run] Slave output still isn't exhausted.");
        // Processing slave process input records
        bool slave_process_input_records_exhausted{true};
        if (process_input_records(pty_fd, h_out, h_in_rec, slave_process_input_records_exhausted))
            process_input_records_error_counter = 0;
        else if (++process_input_records_error_counter > IO_ERRCOUNT_IGNORE) {
            logf(LOG_WARN, "[run] Failed to process input records in %i attempts. Exiting.", IO_ERRCOUNT_IGNORE);
            break;
        } else {
            _something_happened = true;
            usleep(10000);
            continue;
        }
        if (!slave_process_input_records_exhausted)
            logf(LOG_TRACE, "[run] Input records still aren't exhausted.");
        // The remaining part of the loop is used only in managed mode (for reading from _h_input)
        // Also currently we aren't dealing with input before finishing with output
        if (h_in == nullptr || !slave_output_exhausted || !slave_process_input_records_exhausted)
            continue;
        // Processing slave process input
        if (process_input(pty_fd, h_in))
            process_input_stream_error_counter = 0;
        else if (++process_input_stream_error_counter > IO_ERRCOUNT_IGNORE) {
            logf(LOG_WARN, "[run] Failed to process input in %i attempts. Exiting.", IO_ERRCOUNT_IGNORE);
            break;
        } else
            usleep(10000);
    }
    log(LOG_INFO, "[run] Event loop finished.");
}

#pragma clang diagnostic pop
