/*
 Created by Fat Dragon on 12/24/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).
*/

#ifndef PTYNATIVE_INCLUDES_H
#define PTYNATIVE_INCLUDES_H

#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-deprecated-headers"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <process.h>
#include <signal.h>
#include <time.h>

#pragma clang diagnostic pop

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/termios.h>
#include <sys/cygwin.h>

#include <w32api/wtypes.h>
#include <w32api/wincon.h>
#include <w32api/winuser.h>

#include <unistd.h>
#include <utmp.h>

#include <pty.h>

#endif //PTYNATIVE_INCLUDES_H
