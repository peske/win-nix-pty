/*
 Created by Fat Dragon on 12/14/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#include "command_processor.h"

#include "file_helpers.h"
#include "logging.h"

#define PING_PONG_COMMAND 1
#define GET_WINSIZE_COMMAND 2
#define SET_WINSIZE_COMMAND 3
#define GET_TERMIOS_COMMAND 4
#define SET_TERMIOS_COMMAND 5

#define SUCCESS_BYTE 0
#define FAILURE_BYTE 1

// Static asserts, to assure that the structs aren't changed in the future
#ifndef FROM_CLION_CMAKE
#define termios_size 44
static_assert(sizeof(termios) == termios_size);
static_assert(offsetof(struct termios, c_iflag) == 0);
static_assert(sizeof(((termios*)nullptr)->c_iflag) == 4);
static_assert(offsetof(struct termios, c_oflag) == 4);
static_assert(sizeof(((termios*)nullptr)->c_oflag) == 4);
static_assert(offsetof(struct termios, c_cflag) == 8);
static_assert(sizeof(((termios*)nullptr)->c_cflag) == 4);
static_assert(offsetof(struct termios, c_lflag) == 12);
static_assert(sizeof(((termios*)nullptr)->c_lflag) == 4);
static_assert(offsetof(struct termios, c_line) == 16);
static_assert(sizeof(((termios*)nullptr)->c_line) == 1);
static_assert(offsetof(struct termios, c_cc) == 17);
static_assert(sizeof(((termios*)nullptr)->c_cc) == 18);
static_assert(offsetof(struct termios, c_ispeed) == 36);
static_assert(sizeof(((termios*)nullptr)->c_ispeed) == 4);
static_assert(offsetof(struct termios, c_ospeed) == 40);
static_assert(sizeof(((termios*)nullptr)->c_ospeed) == 4);

static_assert(sizeof(((winsize*)nullptr)->ws_row) == 2);
static_assert(sizeof(((winsize*)nullptr)->ws_col) == 2);
#endif

static bool write_response(HANDLE h_cout, bool success, char* buff = nullptr, int length = 0) {
    const char success_byte = success ? SUCCESS_BYTE : FAILURE_BYTE;
    if (!write_bytes(h_cout, &success_byte, 1))
        return false;
    if (buff == nullptr) {
        // If buff is nullptr, we ignore length
        return true;
    }
    if (length <= 0) {
        // If length isn't specified we'll assume that buff is string
        length = strlen(buff);
    }
    return write_bytes(h_cout, buff, length);
}

static bool process_ping_command(HANDLE h_cout) {
    return write_response(h_cout, true);
}

static bool process_get_size_command(int pty_fd, HANDLE h_cout) {
    winsize win_size{};
    const auto result = ioctl(pty_fd, TIOCGWINSZ, &win_size); // NOLINT(hicpp-signed-bitwise)
    if (result == 0) {
        logf(LOG_DEBUG, "[process_get_size_command] 'ioctl' call succeeded (%i x %i)", win_size.ws_col,
                win_size.ws_row);
        return write_response(h_cout, true) && write_unsigned_short(h_cout, win_size.ws_col)
               && write_unsigned_short(h_cout, win_size.ws_row);
    }
    char buff[DEBUG_LOG_MAX_BUFFER];
    snprintf(buff, DEBUG_LOG_MAX_BUFFER, "[process_get_size_command] 'ioctl' call failed. Error: %i (%s)", errno,
            strerror(errno));
    log(LOG_ERROR, buff);
    return write_response(h_cout, false, buff);
}

static bool process_set_size_command(int pty_fd, HANDLE h_cin, HANDLE h_cout) {
    unsigned short cols{0};
    if (!read_unsigned_short(h_cin, cols))
        return false;
    unsigned short rows{0};
    if (!read_unsigned_short(h_cin, rows))
        return false;
    winsize win_size{};
    win_size.ws_row = rows;
    win_size.ws_col = cols;
    logf(LOG_TRACE, "[process_set_size_command] About to set size to (%i x %i)", win_size.ws_col, win_size.ws_row);
    const auto result = ioctl(pty_fd, TIOCSWINSZ, &win_size); // NOLINT(hicpp-signed-bitwise)
    if (result == 0) {
        logf(LOG_DEBUG, "[process_set_size_command] 'ioctl' call succeeded (%i x %i)", win_size.ws_col,
                         win_size.ws_row);
        return write_response(h_cout, true);
    }
    char buff[DEBUG_LOG_MAX_BUFFER];
    snprintf(buff, DEBUG_LOG_MAX_BUFFER, "[process_set_size_command] 'ioctl' call failed. Error: %i (%s)", errno,
            strerror(errno));
    log(LOG_ERROR, buff);
    return write_response(h_cout, false, buff);
}

static bool process_get_termios_command(int pty_fd, HANDLE h_cout) {
    termios t{};
    const auto result = tcgetattr(pty_fd, &t);
    if (result == 0) {
        log(LOG_DEBUG, "[process_get_termios_command] 'tcgetattr' call succeeded.");
        return write_response(h_cout, true, (char*) &t, (int) sizeof(termios));
    }
    char buff[DEBUG_LOG_MAX_BUFFER];
    snprintf(buff, DEBUG_LOG_MAX_BUFFER, "[process_get_termios_command] 'tcgetattr' call failed. Error: %i (%s)", errno,
            strerror(errno));
    log(LOG_ERROR, buff);
    return write_response(h_cout, false, buff);
}

static bool process_set_termios_command(int pty_fd, HANDLE h_cin, HANDLE h_cout) {
    termios t{};
    if (!read_bytes_fixed(h_cin, (char*) &t, (int) sizeof(termios)))
        return false;
    const auto result = tcsetattr(pty_fd, TCSANOW, &t);
    if (result == 0) {
        log(LOG_DEBUG, "[process_set_termios_command] 'tcsetattr' call succeeded.");
        return write_response(h_cout, true);
    }
    char buff[DEBUG_LOG_MAX_BUFFER];
    snprintf(buff, DEBUG_LOG_MAX_BUFFER, "[process_set_termios_command] 'tcsetattr' call failed. Error: %i (%s)", errno,
            strerror(errno));
    log(LOG_ERROR, buff);
    return write_response(h_cout, false, buff);
}

bool process_commands(int pty_fd, HANDLE h_cin, HANDLE h_cout) {
    if (h_cin == nullptr)
        return true;
    char single_byte[1];
    DWORD read{0};
    if (!try_read_bytes_fixed(h_cin, single_byte, 1, &read))
        return false;
    if (read < 1)
        return true;
    switch (single_byte[0]) {
        case PING_PONG_COMMAND:
            log(LOG_DEBUG, "[process_commands] Ping command received.");
            return process_ping_command(h_cout);
        case GET_WINSIZE_COMMAND:
            log(LOG_DEBUG, "[process_commands] Get-size command received.");
            return process_get_size_command(pty_fd, h_cout);
        case SET_WINSIZE_COMMAND:
            log(LOG_DEBUG, "[process_commands] Set-size command received.");
            return process_set_size_command(pty_fd, h_cin, h_cout);
        case GET_TERMIOS_COMMAND:
            log(LOG_DEBUG, "[process_commands] Get-termios command received.");
            return process_get_termios_command(pty_fd, h_cout);
        case SET_TERMIOS_COMMAND:
            log(LOG_DEBUG, "[process_commands] Set-termios command received.");
            return process_set_termios_command(pty_fd, h_cin, h_cout);
        default:
            char buff[DEBUG_LOG_MAX_BUFFER];
            snprintf(buff, DEBUG_LOG_MAX_BUFFER, "[process_commands] Unknown command received: %i.", single_byte[0]);
            log(LOG_DEBUG, buff);
            return write_response(h_cout, false, buff);
    }
}
