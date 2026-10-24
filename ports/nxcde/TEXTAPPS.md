# CDE terminal applications and login startup

This experiment extends the stacking-test desktop. It does not replace the
hardware-confirmed release.

The Apps drawer adds three installed text programs, each in its own nxterm:

- **Kilo Editor** opens `/root/notes.txt`; Ctrl-S saves, Ctrl-Q quits.
- **Text Files** runs the existing `fm` text file manager.
- **BASIC** runs the existing BASIC interpreter.

Entries require both nxterm and the underlying program to be installed.
The first four quick-launch buttons retain their existing order. No new
application packages are installed.

The build patches upstream nxterm to accept `-t title -e program [args...]`.
It directly executes the program on the ELKS PTY instead of injecting a
command into the shell. Existing bare nxterm and legacy command-string use
remain available. Closing a terminal sends SIGHUP to its child process group;
clicking the client raises its parent frame and gives it keyboard focus.

After root logs in on `/dev/tty1`, elkfetch runs and nxstart opens automatically
in the foreground. Exit returns to that login shell without restarting the
desktop. Serial logins and PTY shells do not auto-start it. Create `/root/nox`
to disable startup (`touch /root/nox`), and remove it to enable startup again.
Manual startup remains `nxstart`. Nothing launches the desktop in rc.sys.

## Hardware checks still required

1. Boot on the 2 MB PS/2, log in as root, check elkfetch then desktop startup.
2. Open each text application. Test arrows, function keys, editing and exit.
3. Save a short note with Kilo, reopen it, and verify the saved contents.
4. Move/overlap terminal windows, click to focus, and check repainting.
5. Close an application using its titlebar and check for orphan processes
   with `ps` after leaving the desktop. Repeat launches to check PTY reuse.
6. Exit to shell, run nxstart again, then test `/root/nox` across a new login.
7. Check that terminal and serial shells do not start another desktop.

Each terminal also needs a child process and RAM. Start with one text app
at a time on 2 MB hardware; build success does not establish runtime stability.
