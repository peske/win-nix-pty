/*
 Created by Fat Dragon on 12/21/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#include "stand_alone_io.h"

#include "logging.h"

bool disable_processed_input() {
    HANDLE inh = GetStdHandle(STD_INPUT_HANDLE);
    if (inh == INVALID_HANDLE_VALUE) {
        log_win_error(LOG_ERROR, "[protect_ctrl_break_trap] 'GetStdHandle' returned INVALID_HANDLE_VALUE.");
        return false;
    }
    DWORD con_input_mode{0};
    if (!GetConsoleMode(inh, &con_input_mode)) {
        log_win_error(LOG_ERROR, "[protect_ctrl_break_trap] 'GetConsoleMode' call failed.");
        return false;
    }
    if (con_input_mode & ENABLE_PROCESSED_INPUT) { // NOLINT(hicpp-signed-bitwise)
        if (!SetConsoleMode(inh, con_input_mode & ~ENABLE_PROCESSED_INPUT)) { // NOLINT(hicpp-signed-bitwise)
            log_win_error(LOG_ERROR, "[protect_ctrl_break_trap] 'GetConsoleMode' call failed.");
            return false;
        } else
            log(LOG_DEBUG, "[protect_ctrl_break_trap] ENABLE_PROCESSED_INPUT disabled.");
    }
    return true;
}

bool read_input_records_from_console(INPUT_RECORD* records, int count, int& records_read) {
    records_read = 0;
    HANDLE inh = GetStdHandle(STD_INPUT_HANDLE);
    if (inh == INVALID_HANDLE_VALUE) {
        log_win_error(LOG_ERROR, "[read_input_record_from_console] 'GetStdHandle' returned INVALID_HANDLE_VALUE.");
        return false;
    }
    auto pos = records;
    while (count > 0) {
        DWORD available{0};
        if (!GetNumberOfConsoleInputEvents(inh, &available)) {
            log_win_error(LOG_ERROR, "[read_input_record_from_console] 'GetNumberOfConsoleInputEvents' call failed.");
            return false;
        }
        if (available < 1)
            break;
        while (available > 0) {
            DWORD read{0};
            if (!ReadConsoleInputW(inh, pos, count, &read)) {
                log_win_error(LOG_ERROR, "[read_input_record_from_console] 'ReadConsoleInput' call failed.");
                return false;
            }
            if (read > 0) {
                available -= read;
                pos += (int) read;
                count -= (int) read;
            }
        }
    }
    return true;
}

//bool read_input_records_from_console(INPUT_RECORD* records, int count, int& records_read) {
//    records_read = 0;
//    HANDLE inh = GetStdHandle(STD_INPUT_HANDLE);
//    if (inh == INVALID_HANDLE_VALUE) {
//        log_win_error(LOG_ERROR, "[read_input_record_from_console] 'GetStdHandle' returned INVALID_HANDLE_VALUE.");
//        return false;
//    }
//    auto pos = records;
//    while (count > 0) {
//        DWORD available{0};
//        if (!PeekConsoleInputW(inh, records, count, &available)) {
//            log_win_error(LOG_ERROR, "[read_input_record_from_console] 'PeekConsoleInputW' call failed.");
//            return false;
//        }
//        if (available < 1)
//            return true;
//        DWORD read{0};
//        if (!ReadConsoleInputW(inh, pos, available, &read)) {
//            log_win_error(LOG_ERROR, "[read_input_record_from_console] 'ReadConsoleInput' call failed.");
//            return false;
//        }
//        pos += (int) read;
//        count -= (int) read;
//    }
//    return true;
//}

bool write_output_to_console(char* buff, int length) {
    logf(LOG_TRACE, "[write_output_to_console] About to try to write %i bytes to console output.", length);
    HANDLE inh = GetStdHandle(STD_OUTPUT_HANDLE);
    if (inh == INVALID_HANDLE_VALUE) {
        log_win_error(LOG_ERROR, "[write_output_to_console] 'GetStdHandle' returned INVALID_HANDLE_VALUE.");
        return false;
    }
    while (length > 0) {
        logf(LOG_TRACE, "[write_output_to_console] About to call 'WriteConsoleA' try to write %i bytes.", length);
        DWORD written{0};
        const auto result = WriteConsoleA(inh, buff, length, &written, nullptr);
        if (result == 0) {
            log_win_error(LOG_ERROR, "[write_output_to_console] 'WriteConsoleA' call failed.");
            return false;
        }
        length -= (int)written;
        buff += (int)written;
    }
    return true;
}

bool try_override_win_size(winsize& win_size) {
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h_out == INVALID_HANDLE_VALUE) {
        log_win_error(LOG_ERROR, "[try_override_win_size] 'GetStdHandle' returned INVALID_HANDLE_VALUE.");
        return false;
    }
    CONSOLE_SCREEN_BUFFER_INFO buffer_info{};
    if (!GetConsoleScreenBufferInfo(h_out, &buffer_info)) {
        log_win_error(LOG_ERROR, "[try_override_win_size] 'GetConsoleScreenBufferInfo' call failed.");
        return false;
    }
    win_size.ws_row = buffer_info.srWindow.Bottom - buffer_info.srWindow.Top + 1;
    win_size.ws_col = buffer_info.dwSize.X;
    return true;
}
