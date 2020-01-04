/*
 Created by Fat Dragon on 12/13/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#ifndef PTYNATIVE_LOGGING_H
#define PTYNATIVE_LOGGING_H

#include "includes.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#define LOG_TRACE 0
#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4

#pragma clang diagnostic pop

#define DEBUG_LOG_MAX_BUFFER 1024

extern int _min_log_level;
extern bool _debug_view;

void reset_log_file(int parent_pid);
void log(int level, const char* message);
void logf(int level, const char* format,...);
void log_env();
void log_terminal_info();
bool get_windows_error(int& err, char* buffer, int buff_size);
void log_win_error(int level, const char* format);
void log_lin_error(int level, const char* format);

#endif //PTYNATIVE_LOGGING_H
