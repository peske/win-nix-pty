/*
 Created by Fat Dragon on 12/20/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#include "helpers.h"

#include "logging.h"

bool write_exact(int fd, const char* buffer, int to_write, bool no_logs) {
    if (to_write <= 0) {
        if (!buffer)
            // Nothing to write
            return true;
        // We assume that buffer is char string.
        to_write = strlen(buffer);
    }
    while (to_write > 0) {
        const auto written = write(fd, buffer, to_write);
        if (written < 1) {
            if (!no_logs)
                log_lin_error(LOG_ERROR, "[write_exact] 'write' call failed.");
            return false;
        }
        if (to_write == written)
            return true;
        to_write -= written;
        buffer += written;
    }
    return true;
}

// Besides conversion, *char_string is also null-terminated!
bool wchar_to_char_string(unsigned int code_page, LPCWCH lp_wide_char, char** char_string,
        int lp_wide_char_length) {
    *char_string = nullptr;
    if (lp_wide_char_length <= 0) {
        if (!lp_wide_char)
            // Empty array
            return true;
        // We'll assume that lp_wide_char is null-terminated
        lp_wide_char_length = lstrlenW(lp_wide_char);
    }
    if (lp_wide_char_length < 1)
        // Empty string
        return true;
    const auto len = WideCharToMultiByte(code_page, 0, lp_wide_char, lp_wide_char_length, nullptr, 0, nullptr,
            nullptr);
    if (len <= 0) {
        log_win_error(LOG_ERROR, "[wchar_to_char_string] 'WideCharToMultiByte' call failed.");
        return false;
    }
    char* cs{nullptr};
    while (!cs)
        cs = (char*) malloc(sizeof(char) * (len + 1));
    const auto len2 = WideCharToMultiByte(code_page, 0, lp_wide_char, lp_wide_char_length, cs, len, nullptr,
            nullptr);
    if (len2 <= 0) {
        free(cs);
        log_win_error(LOG_ERROR, "[wchar_to_char_string] 'WideCharToMultiByte' call failed.");
        return false;
    }
    if (len2 != len) {
        // Should not happen ever.
        free(cs);
        log_win_error(LOG_ERROR, "[wchar_to_char_string] 'WideCharToMultiByte' call failed.");
        return false;
    }
    // To ensure termination
    cs[len] = 0;
    *char_string = cs;
    return true;
}
