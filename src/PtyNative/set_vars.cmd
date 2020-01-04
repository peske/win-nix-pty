@set cygwin32toolchain=%~d0\cygwin
@set cygwin64toolchain=%~d0\cygwin64
@set msys2x32toolchain=%~d0\MSYS\msys2-x32
@set msys2x64toolchain=%~d0\MSYS\msys2-x64

@set cygwin32exe=pty-cyg-32.exe
@set cygwin64exe=pty-cyg-64.exe
@set msys2x32exe=pty-msys2-32.exe
@set msys2x64exe=pty-msys2-64.exe

@set LOGGING=
@if "%debug_log%"   == "YES" ( set "LOGGING=%LOGGING% -D_USE_DEBUG_LOG" )

@set "USE_GCC_STATIC=-static-libgcc -static-libstdc++"

@set "dumpbin=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\dumpbin.exe"
@set "grep=%~d0\GitSDK\usr\bin\grep.exe"

@if exist "%~dp0set_vars_user.cmd" @call "%~dp0set_vars_user.cmd"
