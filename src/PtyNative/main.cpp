/*
 Created by Fat Dragon on 12/13/2019.

 Copyright (c) 2019-present Fat Dragon

 This file is part of win-nix-pty project (https://github.com/peske/win-nix-pty)
 which is released under BSD-3-Clause license (https://github.com/peske/win-nix-pty/blob/master/LICENSE).

 Parts of this file are inspired by / taken from:
 - https://github.com/Maximus5/cygwin-connector Copyright (c) 2015-present Maximus5
 - https://github.com/mintty/mintty Copyright (c) 2008-13 Andy Koppe, 2015-18 Thomas Wolff
*/

#include "includes.h"

#include "io_processor.h"
#include "logging.h"
#include "stand_alone_io.h"
#include "version.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-W#pragma-messages"

#ifdef __CYGWIN__
#include <cygwin/version.h>

#if (__GNUC_MINOR__ >= 9) || (CYGWIN_VERSION_API_MINOR>=93)
#pragma message("GCC/Cygwin versions OK")
#else
static_assert(false, "GCC/Cygwin versions check failed.");
#endif
#else
static_assert(false, "__CYGWIN__ isn't defined.")
#endif

//#define USE_MINTTY_CONF

#ifdef USE_MINTTY_CONF
#include <langinfo.h>
#endif

#pragma clang diagnostic pop

#define EXIT_CODE_ARGUMENTS 1
#define EXIT_CODE_API_CALL_FAILED 2
#define EXIT_CODE_SHELL_LAUNCH_FAILED 3
#define EXIT_CODE_UNEXPECTED_HAPPENED -1

#define DEFAULT_ROWS 25
#define DEFAULT_COLUMNS 80

#define MAX_USERNAME_LENGTH 100
#define MAX_PTYNAME_LENGTH 256

#define SYNC_SLEEP_PERIOD_MICROSECONDS 3000000

static int _pty_fd2{0};
static int _slave_pid2{0};
static bool _sigusr1_received{false};

static void print_help() {
    printf("Usage: <executable.exe> [args] [-] <shell.exe> [shell_args]\n");
    printf("  <executable.exe> The name of this executable (i.e. pty-cyg-32.exe...)\n\n");
    printf("Possible [args]:\n");
    printf("  --help         This help.\n");
    printf("  --out <hout>   Output pipe handle. If this option is specified, than the process\n");
    printf("                 will run in \"managed mode\" (i.e. from another app), otherwise\n");
    printf("                 it'll run in a \"stand-alone\" mode. <hout> is an integer which\n");
    printf("                 represents the output pipe handle.(i.e. `--out 123`). This option\n");
    printf("                 requires that at least one of `--ins` and `--inr` options is also\n");
    printf("                 specified.");
    printf("  --ins <hins>   Input pipe handle (i.e. `--ins 124`). It is an input pipe handle\n");
    printf("                 which is used in a regular, stream-like fashion. This option is\n");
    printf("                 ignored in \"stand-alone mode\" (if `--out` isn't specified).\n");
    printf("  --inr <hinr>   Input-by-record handle (i.e. `--inr 125`). It's ignored in\n");
    printf("                 \"stand-alone mode\". It is an input pipe for record-by-record\n");
    printf("                 (INPUT_RECORD) reading. It can be used for regular text input\n");
    printf("                 (KEY_EVENT_RECORD), but also for sending mouse events\n");
    printf("                 (MOUSE_EVENT_RECORD) and resize terminal events\n");
    printf("                 (WINDOW_BUFFER_SIZE_RECORD). Note that the calling app should not\n");
    printf("                 repeat the same input on both `--ins` and `--inr` pipes. Also note\n");
    printf("                 that the calling app should specify at least one of `--inr` and\n");
    printf("                 `--cmd` in order to be able to resize the terminal.\n");
    printf("  --cmd <hcmd>   Semi-column-separated list of command pipe handles\n");
    printf("                 (i.e. `--hcmd 789;987`). It must contain exactly two pipe handles,\n");
    printf("                 in the following order: `--hcmd <cmdin>;<cmdout>`. This argument is\n");
    printf("                 ignored in \"stand-alone mode\".\n");
    printf("  --rows <rows>  Terminal height in rows (defaults to %i). This argument is ignored\n", DEFAULT_ROWS);
    printf("                 in \"stand-alone mode\".\n");
    printf("  --cols <cols>  Terminal width in columns (defaults to %i). Also ignored in\n", DEFAULT_COLUMNS);
    printf("                 \"stand-alone mode\".\n");
    printf("  --dir <dir>    Change working directory to `dir` before starting the shell. It\n");
    printf("                 also sets CHERE_INVOKING env var to 1.\n");
    printf("  --log <level>  Log level. <level> has to be an integer. Possible levels are:\n");
    printf("                 %i - Trace, %i - Debug, %i - Info, %i - Warning, and %i - Error.\n", LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR);
    printf("                 The log files can be found in the current working directory.\n");
    printf("  --syslog       If specified, the logs will also be sent to system debug log, for\n");
    printf("                 real-time tracking in DebugView or similar tool.\n\n");
    printf("  <shell.exe>    Shell executable to launch (i.e. `C:\\cygwin64\\bin\\bash.exe`)\n");
    printf("  [shell_args]   Additional arguments to use when starting the shell.\n");
    printf("                 (i.e. `-i --login` which are often used with `bash`).\n\n");
}

static HANDLE read_handle(char* value, char** ptr) {
    logf(LOG_TRACE, "[read_handle] value = '%s'", value);
    *ptr = value;
    auto val = strtol(value, ptr, 0);
    if (val < 1 || *ptr == value) {
        printf("Invalid arguments. Failed reading handle value from %s\n\n", value);
        print_help();
        exit(EXIT_CODE_ARGUMENTS);
    }
    return HANDLE(UINT_PTR(val));
}

static HANDLE read_handle(char* value) {
    char* ptr{nullptr};
    const auto result = read_handle(value, &ptr);
    if (ptr[0] != 0) {
        // Not all chars are read
        printf("Invalid arguments. Failed reading handle value from %s\n\n", value);
        print_help();
        exit(EXIT_CODE_ARGUMENTS);
    }
    return result;
}

static unsigned short read_ushort(char* value) {
    char* ptr{nullptr};
    auto val = strtol(value, &ptr, 0);
    if (val < 1 || ptr[0] != 0) {
        printf("Invalid arguments. Failed reading unsigned short value from %s\n\n", value);
        print_help();
        exit(EXIT_CODE_ARGUMENTS);
    }
    return static_cast<unsigned short>(val);
}

static void exit_signal(int sig) {
    if (sig == SIGINT) {
        log(LOG_WARN, "[exit_signal] Unexpected SIGINT signal. Sending Ctrl+C to slave through PTY.");
        if (write(_pty_fd2, "\3", 1) < 1)
            log_lin_error(LOG_WARN, "[exit_signal] 'write' call failed.");
        return;
    }
    logf(LOG_DEBUG, "[exit_signal] Exit signal received: %i. Killing the slave process, unsubscribing and re-emitting.",
            sig);
    if (_slave_pid2 > 0)
        kill(-_slave_pid2, SIGHUP);
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

static void sigusr1_signal(int sig) {
    if (sig == SIGUSR1) {
        _sigusr1_received = true;
        log(LOG_DEBUG, "[sigusr1_signal] SIGUSR1 signal received.");
    }
}

static BOOL WINAPI ctrl_handler_routine(DWORD ctrl_type) {
    // We do not expect to receive CTRL_C_EVENT/CTRL_BREAK_EVENT because of ProtectCtrlBreakTrap
    switch (ctrl_type)
    {
        case CTRL_C_EVENT:
            log(LOG_DEBUG, "[ctrl_handler_routine] CTRL_C_EVENT received. Doing nothing.");
            return FALSE;
        case CTRL_BREAK_EVENT:
            log(LOG_DEBUG, "[ctrl_handler_routine] CTRL_BREAK_EVENT received. Marking it as processed.");
            return TRUE;
        case CTRL_CLOSE_EVENT:
            log(LOG_DEBUG, "[ctrl_handler_routine] CTRL_CLOSE_EVENT received. Killing the slave process.");
            break;
        case CTRL_LOGOFF_EVENT:
            log(LOG_DEBUG, "[ctrl_handler_routine] CTRL_LOGOFF_EVENT received. Killing the slave process.");
            break;
        case CTRL_SHUTDOWN_EVENT:
            log(LOG_DEBUG, "[ctrl_handler_routine] CTRL_SHUTDOWN_EVENT received. Killing the slave process.");
            break;
        default:
            logf(LOG_ERROR, "[ctrl_handler_routine] Unknown event (%i) received. Doing nothing.", ctrl_type);
            return FALSE;
    }
    if (_slave_pid2 > 0)
        kill(-_slave_pid2, SIGHUP);
    return FALSE;
}

//static void ensure_terminal() {
//    auto hwnd = GetConsoleWindow();
//    if (hwnd) {
//        log(LOG_DEBUG, "[ensure_terminal] Console window already exists.");
//        return;
//    }
//    log(LOG_DEBUG, "[ensure_terminal] Console isn't allocated. Attempting to allocate...");
//    if (!AllocConsole()) {
//        log_win_error(LOG_ERROR, "[ensure_terminal] 'AllocConsole' call failed.");
//        return;
//    }
//    log(LOG_DEBUG, "[ensure_terminal] Console allocated successfully.");
//    hwnd = GetConsoleWindow();
//    if (!hwnd) {
//        log_win_error(LOG_ERROR, "[ensure_terminal] 'GetConsoleWindow' call failed although the console is allocated successfully.");
//        return;
//    }
//    if (ShowWindow(hwnd, SW_HIDE))
//        log(LOG_DEBUG, "[ensure_terminal] Console window was visible, and now it's hidden.");
//    else
//        log(LOG_DEBUG, "[ensure_terminal] Console window was hidden, and remained hidden.");
//}

#ifdef USE_MINTTY_CONF
static void __unused slave_term_config(termios& attr) {
    log(LOG_DEBUG, "[slave_term_config1] Using Mintty terminal configuration.");
    attr.c_cc[VERASE] = CDEL;
    attr.c_iflag |= IXANY | IMAXBEL; // NOLINT(hicpp-signed-bitwise)
#ifdef IUTF8
    bool utf8 = strcmp(nl_langinfo(CODESET), "UTF-8") == 0;
    if (utf8)
        attr.c_iflag |= IUTF8; // NOLINT(hicpp-signed-bitwise)
    else
        attr.c_iflag &= ~IUTF8; // NOLINT(hicpp-signed-bitwise)
#endif
    attr.c_lflag |= ECHOE | ECHOK | ECHOCTL | ECHOKE; // NOLINT(hicpp-signed-bitwise)
}
#else
static void slave_term_config(termios& attr) {
    log(LOG_DEBUG, "[slave_term_config1] Using ConEmu terminal configuration.");
    attr.c_cc[VERASE] = CDEL;
    attr.c_iflag |= IXANY | IMAXBEL; // NOLINT(hicpp-signed-bitwise)
    attr.c_lflag |= ECHOE | ECHOK | ECHOCTL | ECHOKE; // NOLINT(hicpp-signed-bitwise)
}
#endif

static void do_slave(int parent_pid, char** argv, char* dir) {
    // Slave process. Let's first destroy things we shouldn't use anyway
    reset_log_file(parent_pid);
    logf(LOG_INFO, "[do_slave] Hello from the slave process (PID=%i)!", getpid());
    log_env();
    log_terminal_info();
    const auto reported_parent_pid = getppid();
    if (reported_parent_pid != parent_pid)
        logf(LOG_WARN, "[do_slave] getppid() returns %i although parent PID is %i.", reported_parent_pid, parent_pid);
    // Wait for a signal from the parent process
    if (_sigusr1_received)
        log(LOG_DEBUG, "[do_slave] We're already signaled to continue.");
    else {
        auto ms = GetTickCount();
        usleep(SYNC_SLEEP_PERIOD_MICROSECONDS);
        ms = GetTickCount() - ms;
        if (!_sigusr1_received) {
            logf(LOG_ERROR, "[do_slave] After %i ms we still haven't got SIGUSR1 signal from the parent. Exiting.", ms);
            exit(EXIT_CODE_UNEXPECTED_HAPPENED);
        }
        logf(LOG_DEBUG, "[do_slave] SIGUSR1 signal from the parent received after %i ms.", ms);
    }
    // Reset signals
    if (signal(SIGUSR1, SIG_DFL) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to reset signal handler for SIGUSR1 signal to SIG_DFL.");
    if (signal(SIGHUP, SIG_DFL) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to reset signal handler for SIGHUP signal to SIG_DFL.");
    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to reset signal handler for SIGINT signal to SIG_DFL.");
    if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to reset signal handler for SIGQUIT signal to SIG_DFL.");
    if (signal(SIGTERM, SIG_DFL) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to reset signal handler for SIGTERM signal to SIG_DFL.");
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to reset signal handler for SIGCHLD signal to SIG_DFL.");
    // Mimic login's behavior by disabling the job control signals
    if (signal(SIGTSTP, SIG_IGN) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to set signal handler for SIGTSTP signal to SIG_IGN.");
    if (signal(SIGTTIN, SIG_IGN) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to set signal handler for SIGTTIN signal to SIG_IGN.");
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[do_slave] Failed to set signal handler for SIGTTOU signal to SIG_IGN.");
    log(LOG_TRACE, "[do_slave] Signal handlers reset successfully.");
    // Terminal line settings
    termios attr{};
    if (tcgetattr(0, &attr) < 0) {
        log_lin_error(LOG_ERROR, "[do_slave] 'tcgetattr' call failed.");
        exit(EXIT_CODE_API_CALL_FAILED);
    }
    log(LOG_DEBUG, "[do_slave] 'tcgetattr' succeeded.");
    slave_term_config(attr);
    if (tcsetattr(0, TCSANOW, &attr) < 0) {
        log_lin_error(LOG_ERROR, "[do_slave] 'tcsetattr' call failed.");
        exit(EXIT_CODE_API_CALL_FAILED);
    }
    log(LOG_DEBUG, "[do_slave] 'tcsetattr' succeeded.");
    // Change working directory (if required)
    if (dir != nullptr) {
        if (chdir(dir) < 0)
            logf(LOG_ERROR, "[do_slave] 'chdir(\"%s\")' call failed. Error: %i (%s)", dir, errno, strerror(errno));
        else {
            logf(LOG_DEBUG, "[do_slave] 'chdir(\"%s\")' succeeded.", dir);
            if (setenv("CHERE_INVOKING", "1", true) == 0)
                log(LOG_DEBUG, R"([do_slave] 'setenv("CHERE_INVOKING", "1", true)' call succeeded.)");
            else
                log_lin_error(LOG_ERROR, R"([do_slave] 'setenv("CHERE_INVOKING", "1", true)' call failed.)");
        }
    }
    if (_min_log_level <= LOG_DEBUG) {
        char cwd[1000];
        if (getcwd(cwd, 1000) == nullptr)
            log_lin_error(LOG_ERROR, "[do_slave] 'getcwd' call failed.");
        else
            logf(LOG_DEBUG, "[do_slave] Current working directory: %s", cwd);
        if (FILE *shell_file = fopen(argv[0], "r")) {
            fclose(shell_file);
            logf(LOG_DEBUG, "[do_slave] Shell found: %s", argv[0]);
        } else
            logf(LOG_ERROR, "[do_slave] Shell not found: %s", argv[0]);
        if (argv[1] == nullptr)
            logf(LOG_DEBUG, "[do_slave] No shell args.");
        else
            logf(LOG_DEBUG, "[do_slave] First shell argument: %s", argv[1]);
    }
    auto parent_notified{false};
    for (auto i{0}; i < 5; ++i){
        if (kill(parent_pid, SIGUSR1) == 0) {
            log(LOG_DEBUG, "[do_slave] Parent process notified that we're ready.");
            parent_notified = true;
            break;
        }
        logf(LOG_ERROR, "[do_slave] 'kill' call failed for parent PID = %i, attempt %i. Error: %i (%s)",
                parent_pid, i + 1, errno, strerror(errno));
        usleep(20000);
    }
    if (!parent_notified) {
        log(LOG_ERROR, "[do_slave] Failed to notify parent (in 5 attempts) that we're ready. Exiting.");
        exit(EXIT_CODE_API_CALL_FAILED);
    }
    logf(LOG_DEBUG,"[do_slave] About to call 'execvp'. Shell: %s", argv[0]);
    execvp(argv[0], argv);
    // If we're here 'execvp' has failed.
    log_lin_error(LOG_ERROR, "[do_slave] 'execvp' call failed.");
    exit(EXIT_CODE_SHELL_LAUNCH_FAILED);
}

static void do_master(int pty_fd, int slave_pid, HANDLE h_in, HANDLE h_in_rec, HANDLE h_out, HANDLE h_cin, HANDLE h_cout) {
    _pty_fd2 = pty_fd;
    _slave_pid2 = slave_pid;
    char pty_name_buff[MAX_PTYNAME_LENGTH];
    if (ttyname_r(pty_fd, pty_name_buff, MAX_PTYNAME_LENGTH) != 0)
        log_lin_error(LOG_ERROR, "[do_master] 'ptsname' call failed.");
    else {
        logf(LOG_DEBUG, "[do_master] PTY created. Name: %s", pty_name_buff);
        char username[MAX_USERNAME_LENGTH];
        if (getlogin_r(username, MAX_USERNAME_LENGTH) != 0) {
            log_lin_error(LOG_ERROR, "[do_master] 'getlogin_r' call failed.");
            username[0] = 0;
            strcat(username, "?");
        } else
            logf(LOG_DEBUG, "[do_master] Username: %s", username);
        char* pty_name = pty_name_buff;
        utmp ut{};
        memset(&ut, 0, sizeof(ut));
        if (strncmp(pty_name, "/dev/", 5) == 0)
            pty_name += 5;
        lstrcpyn(ut.ut_line, pty_name, sizeof(ut.ut_line));
        if (pty_name[0] && pty_name[1] == 't' && pty_name[2] == 'y')
            pty_name += 3;
        else if (strncmp(pty_name, "pts/", 4) == 0)
            pty_name += 4;
        lstrcpyn(ut.ut_id, pty_name, sizeof(ut.ut_id));
        ut.ut_type = USER_PROCESS;
        ut.ut_pid = slave_pid;
        ut.ut_time = time(nullptr);
        lstrcpyn(ut.ut_user, username, sizeof(ut.ut_user));
        if (gethostname(ut.ut_host, sizeof(ut.ut_host)) == 0)
            logf(LOG_DEBUG, "[do_master] Hostname: %s", ut.ut_host);
        else
            log_lin_error(LOG_ERROR, "[do_master] 'gethostname' call failed.");
        errno = 0;
        login(&ut);
        if (errno != 0)
            logf(LOG_WARN, "[do_master] 'login' function has set errno to %i (%s)", errno, strerror(errno));
        else
            log(LOG_DEBUG, "[do_master] Terminal login succeeded.");
    }
    // Notify child process to continue:
    if (kill(slave_pid, SIGUSR1) != 0)
        log_lin_error(LOG_ERROR, "[do_master] 'kill' call failed.");
    else
        log(LOG_DEBUG, "[do_master] Child process notified to continue.");
    // Wait for a signal from the slave process
    auto ms = GetTickCount();
    usleep(SYNC_SLEEP_PERIOD_MICROSECONDS);
    ms = GetTickCount() - ms;
    if (!_sigusr1_received) {
        logf(LOG_ERROR, "[do_master] After %i ms we still haven't got SIGUSR1 signal from the slave. Exiting.", ms);
        exit(EXIT_CODE_UNEXPECTED_HAPPENED);
    }
    logf(LOG_DEBUG, "[do_master] SIGUSR1 signal from the slave received after %i ms.", ms);
    run(pty_fd, slave_pid, h_in, h_in_rec, h_out, h_cin, h_cout);
}

int main(int argc, char** argv) {
    if (argc < 2){
        printf("Invalid arguments.\n\n");
        print_help();
        exit(EXIT_CODE_ARGUMENTS);
    }
    // Parsing master process arguments
    unsigned short rows{DEFAULT_ROWS};
    unsigned short cols{DEFAULT_COLUMNS};
    char* dir{nullptr};
    HANDLE h_in{nullptr};
    HANDLE h_in_rec{nullptr};
    HANDLE h_out{nullptr};
    HANDLE h_cin{nullptr};
    HANDLE h_cout{nullptr};
    // Skipping the first argument (executable name):
    ++argv;
    --argc;
    while (argc > 0 && argv[0][0] == '-') {
        char* arg = argv[0];
        ++argv;
        --argc;
        if (strcmp(arg, "-") == 0)
            // Shell separator dash. Breaking the loop.
            break;
        if (strcmp(arg, "--help") == 0) {
            print_help();
            exit(0);
        }
        if (strcmp(arg, "--version") == 0) {
            printf("Cygwin/MSYS2 *nix PTY version %s.\n", VERSION_S);
            exit(0);
        }
        if (strcmp(arg, "--syslog") == 0) {
            _debug_view = true;
            continue;
        }
        if (strcmp(arg, "--out") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--out` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            h_out = read_handle(argv[0]);
            ++argv;
            --argc;
            continue;
        }
        if (strcmp(arg, "--ins") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--ins` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            h_in = read_handle(argv[0]);
            ++argv;
            --argc;
            continue;
        }
        if (strcmp(arg, "--inr") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--inr` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            h_in_rec = read_handle(argv[0]);
            ++argv;
            --argc;
            continue;
        }
        if (strcmp(arg, "--cmd") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--cmd` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            char* value = argv[0];
            ++argv;
            --argc;
            char* ptr{nullptr};
            h_cin = read_handle(value, &ptr);
            if (ptr[0] != ';') {
                printf("Invalid arguments. Invalid `--cmd`.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            value = ptr + 1;
            ptr = nullptr;
            h_cout = read_handle(value, &ptr);
            if (ptr[0] != 0) {
                printf("Invalid arguments. Invalid `--cmd`.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            continue;
        }
        if (strcmp(arg, "--rows") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--rows` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            rows = strcmp(argv[0], "0") == 0 ? 0 : read_ushort(argv[0]);
            ++argv;
            --argc;
            continue;
        }
        if (strcmp(arg, "--cols") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--cols` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            cols = strcmp(argv[0], "0") == 0 ? 0 : read_ushort(argv[0]);
            ++argv;
            --argc;
            continue;
        }
        if (strcmp(arg, "--dir") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--dir` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            dir = argv[0];
            ++argv;
            --argc;
            continue;
        }
        if (strcmp(arg, "--log") == 0) {
            if (argc < 1 || argv[0] == nullptr || argv[0][0] == '-') {
                printf("Invalid arguments. `--log` requires a value.\n\n");
                print_help();
                exit(EXIT_CODE_ARGUMENTS);
            }
            _min_log_level = strcmp(argv[0], "0") == 0 ? 0 : read_ushort(argv[0]);
            ++argv;
            --argc;
            continue;
        }
        printf("Invalid arguments. Unknown argument `%s`\n\n", arg);
        print_help();
        exit(EXIT_CODE_ARGUMENTS);
    }
    if (h_out == nullptr) {
        // Without --out we are ignoring all other handles
        h_in = nullptr;
        h_in_rec = nullptr;
        h_cin = nullptr;
        h_cout = nullptr;
        log(LOG_DEBUG, "[main] Launched in stand-alone mode.");
    } else {
        if (h_in == nullptr && h_in_rec == nullptr) {
            printf("Invalid arguments. `--out` requires at least one of `--ins` and `--inr` to be specified also.\"");
            exit(EXIT_CODE_ARGUMENTS);
        } else {
            logf(LOG_DEBUG, "[main] Launched in managed mode. h_in = %i; h_out = %i; h_in_rec = %i.", h_in, h_out, h_in_rec);
            if (h_cin == nullptr)
                log(LOG_DEBUG, "[main] Command pipes aren't specified.");
            else
                logf(LOG_DEBUG, "[main] Command pipes specified: h_cin = %i, h_cout = %i.", h_cin, h_cout);
        }
//        ensure_terminal();
    }
    if (argc < 1) {
        printf("Malformed arguments. Shell executable name is missing.\n\n");
        print_help();
        exit(EXIT_CODE_ARGUMENTS);
    }
    log_env();
    log_terminal_info();
    // Preparing slave process arguments:
    char* slave_argv[argc + 1];
    slave_argv[argc] = nullptr;
    for (auto i = 0; i < argc; ++i)
        slave_argv[i] = argv[i];
    log(LOG_TRACE, "[main] Slave process args prepared.");
    winsize win_size{.ws_row = rows, .ws_col = cols};
    if (h_out == nullptr) {
        // Stand-alone mode - let's try to get actual window size
        try_override_win_size(win_size);
        // Set master's terminal info
        termios attr{};
        if (tcgetattr(0, &attr) != 0) {
            log_lin_error(LOG_ERROR, "[main] 'tcgetattr' call failed.");
            printf("Failed to get terminal attributes.");
            exit(EXIT_CODE_API_CALL_FAILED);
        }
        attr.c_cc[VERASE] = CDEL;
        attr.c_iflag = 0;
        attr.c_lflag = ISIG;
        if (tcsetattr(0, TCSANOW, &attr) != 0) {
            log_lin_error(LOG_ERROR, "[main] 'tcsetattr' call failed.");
            printf("Failed to set terminal attributes.");
            exit(EXIT_CODE_API_CALL_FAILED);
        }
        log(LOG_DEBUG, "[main] Terminal attributes set successfully.");
    }
    // Setting signal handlers
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[main] 'signal' call for SIGHUP signal failed.");
    if (signal(SIGINT, exit_signal) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[main] 'signal' call for SIGINT signal failed.");
    if (signal(SIGTERM, exit_signal) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[main] 'signal' call for SIGTERM signal failed.");
    if (signal(SIGQUIT, exit_signal) == SIG_ERR)
        log_lin_error(LOG_ERROR, "[main] 'signal' call for SIGQUIT signal failed.");
    if (signal(SIGUSR1, sigusr1_signal) == SIG_ERR) {
        log_lin_error(LOG_ERROR, "[main] 'signal' call for SIGUSR1 signal failed.");
        // This handler is crucial for synchronization, so in this case we'll exit.
        exit(EXIT_CODE_API_CALL_FAILED);
    }
    log(LOG_DEBUG, "[main] Signal handlers set.");
    if (!SetConsoleCtrlHandler(ctrl_handler_routine, true)) {
        log_win_error(LOG_ERROR, "[main] 'SetConsoleCtrlHandler' call failed.");
        exit(EXIT_CODE_API_CALL_FAILED);
    }
    log(LOG_DEBUG, "[main] 'SetConsoleCtrlHandler' call succeeded.");
    const auto parent_pid = getpid();
    logf(LOG_DEBUG, "[main] Initial winsize: ws_col = %i ws_row = %i.", win_size.ws_col, win_size.ws_row);
    logf(LOG_DEBUG, "[main] About to fork...");
    int pty_fd{0};
    const int slave_pid = forkpty(&pty_fd, nullptr, nullptr, &win_size);
    if (slave_pid < 0) {
        log_lin_error(LOG_ERROR, "[main] 'forkpty' call failed.");
        printf("Failed to fork.");
        exit(EXIT_CODE_API_CALL_FAILED);
    }
    if (slave_pid == 0) {
        do_slave(parent_pid, argv, dir);
        // This line will never be reached.
        exit(EXIT_CODE_UNEXPECTED_HAPPENED);
    }
    // The rest is master process only.
    do_master(pty_fd, slave_pid, h_in, h_in_rec, h_out, h_cin, h_cout);
    log(LOG_INFO, "[main] Bye-bye...");
    exit(0);
}
