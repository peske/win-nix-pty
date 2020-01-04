/*
 Created by Fat Dragon on 12/20/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#ifndef PTYNATIVE_HELPERS_H
#define PTYNATIVE_HELPERS_H

#include "includes.h"

bool write_exact(int fd, const char* buffer, int to_write = -1, bool no_logs = false);

bool wchar_to_char_string(unsigned int code_page, LPCWCH lp_wide_char_str, char** char_string,
        int lp_wide_char_length = -1);

#endif //PTYNATIVE_HELPERS_H
