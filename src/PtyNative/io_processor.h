/*
 Created by Fat Dragon on 12/14/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#ifndef PTYNATIVE_IO_PROCESSOR_H
#define PTYNATIVE_IO_PROCESSOR_H

#include "includes.h"

void run(int pty_fd, int slave_pid, HANDLE h_in, HANDLE h_in_rec, HANDLE h_out, HANDLE h_cin, HANDLE h_cout);

#endif //PTYNATIVE_IO_PROCESSOR_H
