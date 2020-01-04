/*
 Created by Fat Dragon on 12/14/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#ifndef PTYNATIVE_FILE_HELPERS_H
#define PTYNATIVE_FILE_HELPERS_H

#include "includes.h"

bool read_bytes(HANDLE h_file, char* buff, DWORD bytes_to_read, DWORD* bytes_read);

bool try_read_bytes(HANDLE h_pipe, char* buff, DWORD bytes_to_read, DWORD* bytes_read);

bool try_read_bytes_fixed(HANDLE h_pipe, char* buff, DWORD bytes_to_read, DWORD* bytes_read);

bool read_bytes_fixed(HANDLE h_file, char* buff, int length);

bool write_bytes(HANDLE h_file, const char* buff, int length);

bool read_unsigned_short(HANDLE h_file, unsigned short& value);

bool write_unsigned_short(HANDLE h_file, unsigned short value);

#endif //PTYNATIVE_FILE_HELPERS_H
