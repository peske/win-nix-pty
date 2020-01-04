/*
 Created by Fat Dragon on 12/21/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#ifndef PTYNATIVE_STAND_ALONE_IO_H
#define PTYNATIVE_STAND_ALONE_IO_H

#include "includes.h"

bool disable_processed_input();

bool read_input_records_from_console(INPUT_RECORD* records, int count, int& records_read);

bool write_output_to_console(char* buff, int length);

bool try_override_win_size(winsize& win_size);

#endif //PTYNATIVE_STAND_ALONE_IO_H
