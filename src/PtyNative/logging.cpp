/*
 Created by Fat Dragon on 12/13/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#include "logging.h"

#include "helpers.h"

#define MAX_WIN_ERROR_MESSAGE_LENGTH 256
#define LOG_FILE_NAME_LENGTH 50

int _min_log_level{0};//{LOG_ERROR + 1};//Turned off by default.
bool _debug_view{true};//{false};
static volatile int _log_fd{0};
static int _parent_pid{0};

static void log_args(int log_fd) {
    const auto cmd_line = GetCommandLineW();
    if (!cmd_line)
        return;
    char* str_ptr{nullptr};
    if (wchar_to_char_string(CP_UTF8, cmd_line, &str_ptr) && write_exact(log_fd, str_ptr))
        write(log_fd, "\n", 1);
    if (str_ptr)
        free(str_ptr);
}

static int get_log_file() {
    if (_log_fd > 0)
        return _log_fd;
    // Creating the log file
    auto raw_time = time(nullptr);
    auto time_info = localtime(&raw_time);
    char log_file_name[LOG_FILE_NAME_LENGTH]{0};
    if (time_info == nullptr) {
        if (_parent_pid > 0)
            snprintf(log_file_name, LOG_FILE_NAME_LENGTH, "./pty-%i-%i.log", _parent_pid, getpid());
        else
            snprintf(log_file_name, LOG_FILE_NAME_LENGTH, "./pty-%i.log", getpid());
    } else {
        if (_parent_pid > 0)
            snprintf(log_file_name, LOG_FILE_NAME_LENGTH, "./pty-%04i%02i%02i-%02i%02i%02i-%i-%i.log",
                    time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday + 1, time_info->tm_hour,
                    time_info->tm_min, time_info->tm_sec, _parent_pid, getpid());
        else
            snprintf(log_file_name, LOG_FILE_NAME_LENGTH, "./pty-%04i%02i%02i-%02i%02i%02i-%i.log",
                     time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday + 1, time_info->tm_hour,
                     time_info->tm_min, time_info->tm_sec, getpid());
    }
    _log_fd = open(log_file_name, O_WRONLY|O_CREAT|O_TRUNC, 0600); // NOLINT(hicpp-signed-bitwise)
    if (_log_fd <= 0) {
        _log_fd = 0;
        return 0;
    }
    fchmod(_log_fd, 0600);
    log_args(_log_fd);
    if (_parent_pid > 0)
        write_exact(_log_fd, "---SLAVE PROCESS---\n");
    return _log_fd;
}

static void log_to_file(int log_fd, const char* message) {
    const auto len = strlen(message);
    if (write_exact(log_fd, message, len, true) && message[len - 1] != '\n')
        write(log_fd, "\n", 1);
}

static void log_to_file(const char* message) {
    const auto log_fd = get_log_file();
    if (log_fd <= 0 || !message)
        return;
    timespec raw_time{};
    if (clock_gettime(CLOCK_REALTIME, &raw_time) == 0) {
        tm* time_info = localtime(&raw_time.tv_sec);
        if (time_info != nullptr) {
            const auto new_size = strlen(message) + 30;
            char message_with_time[new_size];
            memset(message_with_time, 0, new_size);
            snprintf(message_with_time, new_size, "[%02i:%02i:%02i.%03i]", time_info->tm_hour, time_info->tm_min,
                    time_info->tm_sec, (int) raw_time.tv_nsec / 1000000);
            strcat(message_with_time, message);
            log_to_file(log_fd, message_with_time);
            return;
        }
    }
    log_to_file(log_fd, message);
}

static void log_to_dbg(char* message) {
    if (!_debug_view|| !message)
        return;
    const auto len = strlen(message);
    if (message[len] != '\n') {
        // I can modify the message because it's a new buffer created in log method below.
        message[len] = '\n';
        // I can write at len because I know that the buffer is big enough - it's created in log method below.
        message[len + 1] = 0;
    }
    OutputDebugStringA(message);
}

static void log_terminal_info(int log_level, int fd, const char* fd_name) {
    auto result = isatty(fd);
    if (result == 0) {
        logf(log_level, "[log_terminal_info] 'isatty' returned %i, meaning that %s isn't a terminal. Error: %i (%s)",
             result, fd_name, errno, strerror(errno));
        return;
    }
    auto name = ttyname(fd);
    if (name == nullptr)
        logf(log_level, "[log_terminal_info] 'ttyname' call failed for %s. Error: %i (%s)", fd_name, errno,
             strerror(errno));
    auto pgrp = tcgetpgrp(fd);
    if (pgrp < 0){
        logf(log_level, "[log_terminal_info] 'tcgetpgrp' call failed for %s. Error: %i (%s)", fd_name, errno,
             strerror(errno));
        logf(log_level, "[log_terminal_info] %s is a terminal with name \"%s\", process group ID <N/A>.",
             fd_name, name == nullptr ? "<N/A>" : name);
    }
    logf(log_level, "[log_terminal_info] %s is a terminal with name \"%s\", process group ID %i.",
         fd_name, name == nullptr ? "<N/A>" : name, pgrp);
}

void reset_log_file(int parent_pid) {
    if (_log_fd > 0)
        close(_log_fd);
    _log_fd = 0;
    _parent_pid = parent_pid;
}

void log(int level, const char* message) {
    if (_min_log_level > level || !message)
        return;
    const auto buff_size = strlen(message) + 20;
    char buffer[buff_size];
    memset(buffer, 0, buff_size);
    strcat(buffer, "FD:");
    switch (level) {
        case LOG_TRACE:
            strcat(buffer, "[TRC]");
            break;
        case LOG_DEBUG:
            strcat(buffer, "[DBG]");
            break;
        case LOG_INFO:
            strcat(buffer, "[INF]");
            break;
        case LOG_WARN:
            strcat(buffer, "[WRN]");
            break;
        case LOG_ERROR:
            strcat(buffer, "[ERR]");
            break;
        default:
            strcat(buffer, "[???]");
            break;
    }
    strcat(buffer, message);
    log_to_file(buffer);
    log_to_dbg(buffer);
}

void logf(int level, const char* format,...) {
    if (_min_log_level > level || !format)
        return;
    va_list ap;
    char buf[DEBUG_LOG_MAX_BUFFER]{0};
    va_start(ap, format);
    vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);
    log(level, buf);
}

void log_env() {
    if (_min_log_level > LOG_DEBUG)
        return;
    char** envs = environ;
    if (envs == nullptr) {
        log(LOG_WARN, "[log_env] 'environ' is nullptr.");
        return;
    }
    log(LOG_DEBUG, "---Environment Variables BEGIN---");
    while (*envs != nullptr)
        logf(LOG_DEBUG, "[log_env] %s", *(envs++));
    log(LOG_DEBUG, "---Environment Variables END---");
}

void log_terminal_info() {
    if (_min_log_level > LOG_DEBUG)
        return;
    log_terminal_info(LOG_DEBUG, STDIN_FILENO, "STDIN");
    log_terminal_info(LOG_DEBUG, STDOUT_FILENO, "STDOUT");
    log_terminal_info(LOG_DEBUG, STDERR_FILENO, "STDERR");
}

bool get_windows_error(int& err, char* buffer, int buff_size) {
    err = GetLastError();
    const auto size = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // NOLINT(hicpp-signed-bitwise)
            nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, buff_size - 1, // NOLINT(hicpp-signed-bitwise)
            nullptr);
    if (size <= 0)
        return false;
    buffer[size] = 0;
    // buffer may contain trailing \r\n, and we want to get rid of this
    for (int i = (int) size - 1; i >= 0; --i) {
        if (buffer[i] == 0)
            continue;
        if (buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i] = 0;
    }
    return true;
}

void log_win_error(int level, const char* format) {
    if (_min_log_level > level)
        return;
    int err{0};
    char buff[MAX_WIN_ERROR_MESSAGE_LENGTH];
    if (!get_windows_error(err, buff, MAX_WIN_ERROR_MESSAGE_LENGTH)) {
        log(level, format);
        return;
    }
    // Create the new format, and log
    const auto new_count = strlen(format) + 20;
    char new_format[new_count];
    memset(new_format, 0, new_count);
    strcat(new_format, format);
    strcat(new_format, " Error: %i (%s)");
    logf(level, new_format, err, buff);
}

void log_lin_error(int level, const char* format) {
    if (_min_log_level > level)
        return;
    // Create the new format, and log
    const auto new_count = strlen(format) + 20;
    char new_format[new_count];
    memset(new_format, 0, new_count);
    strcat(new_format, format);
    strcat(new_format, " Error: %i (%s)");
    logf(level, new_format, errno, strerror(errno));
}
