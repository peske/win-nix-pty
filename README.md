# win-nix-pty

Windows emulated *nix PTY

# Resources

The project is inspired by [mintty](https://github.com/mintty/mintty) (Copyright (c) 2008-13 Andy Koppe, 2015-18 Thomas Wolff) and [ConEmu cygwin-connector](https://github.com/Maximus5/cygwin-connector/) (Copyright (c) 2015-present Maximus5).

# Build PtyNative

The build script (`do_build.cmd`) is copied from [ConEmu cygwin-connector](https://github.com/Maximus5/cygwin-connector/). The steps:

- Rename `set_vars_user.sample.cmd` file to `set_vars_user.cmd`;
- Change `set_vars_user.cmd` content so that it corresponds to your Cygwin / MSYS2 environment;
- Execute `do_build.cmd`.

