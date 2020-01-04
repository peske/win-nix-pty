/*
 Created by Fat Dragon on 12/14/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#include "file_helpers.h"

#include "logging.h"

union unsigned_short_serializer {
    unsigned short value;
    char           bytes[2];
};

static bool peek_for_bytes(HANDLE h_pipe, DWORD bytes_needed, bool& has_enough) {
    DWORD available{0};
    if (!PeekNamedPipe(h_pipe, nullptr, 0, nullptr, &available, nullptr)) {
        log_win_error(LOG_ERROR, "[peek_for_bytes] 'PeekNamedPipe' call failed.");
        return false;
    }
    has_enough = available >= bytes_needed;
    return true;
}

bool read_bytes(HANDLE h_file, char* buff, DWORD bytes_to_read, DWORD* bytes_read) {
    if (!ReadFile(h_file, buff, bytes_to_read, bytes_read, nullptr)) {
        log_win_error(LOG_ERROR, "[read_bytes] 'ReadFile' call failed.");
        return false;
    }
    return true;
}

bool try_read_bytes(HANDLE h_pipe, char* buff, DWORD bytes_to_read, DWORD* bytes_read) {
    *bytes_read = 0;
    bool has_enough{false};
    if (!peek_for_bytes(h_pipe, 1, has_enough))
        return false;
    if (!has_enough)
        return true;
    return read_bytes(h_pipe, buff, bytes_to_read, bytes_read);
}

bool try_read_bytes_fixed(HANDLE h_pipe, char* buff, DWORD bytes_to_read, DWORD* bytes_read) {
    *bytes_read = 0;
    bool has_enough{false};
    if (!peek_for_bytes(h_pipe, bytes_to_read, has_enough))
        return false;
    if (!has_enough)
        return true;
    const auto result = read_bytes_fixed(h_pipe, buff, bytes_to_read);
    if (result)
        *bytes_read = bytes_to_read;
    return result;
}

bool read_bytes_fixed(HANDLE h_file, char* buff, int length) {
    auto pos{0};
    while (pos < length) {
        char bf[length - pos];
        DWORD bytes_read{0};
        if (!read_bytes(h_file, bf, length - pos, &bytes_read))
            return false;
        for (auto i = 0; i < bytes_read; ++i)
            buff[pos++] = bf[i];
    }
    return true;
}

bool write_bytes(HANDLE h_file, const char* buff, int length) {
    while (length > 0) {
        DWORD bytes_written{0};
        if (!WriteFile(h_file, buff, length, &bytes_written, nullptr)) {
            log_win_error(LOG_ERROR, "[write_bytes] 'WriteFile' call failed.");
            return false;
        }
        if (bytes_written > 0) {
            length -= (int)bytes_written;
            buff += (int)bytes_written;
        }
    }
    return true;
}

bool read_unsigned_short(HANDLE h_file, unsigned short& value) {
    unsigned_short_serializer serializer{};
    if (!read_bytes_fixed(h_file, serializer.bytes, (int) sizeof(serializer.bytes)))
        return false;
    value = serializer.value;
    return true;
}

bool write_unsigned_short(HANDLE h_file, unsigned short value) {
    const unsigned_short_serializer serializer{.value = value};
    return write_bytes(h_file, serializer.bytes, (int) sizeof(serializer.bytes));
}
