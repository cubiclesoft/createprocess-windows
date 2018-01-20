CreateProcess() Windows API Command-Line Utility
================================================

A complete, robust command-line utility to construct highly customized calls to the CreateProcess() Windows API to start new processes.  Released under a MIT or LGPL license.

This project is intended primarily for use from batch files (.bat) to execute other programs.  If it can be done with CreateProcess(), it can be done with this command-line program.

Why would you need this?  One use-case would be for programs that don't play nice even with the 'start' command.  For example, Apache 'httpd.exe' hangs the command-line even with 'start /B /MIN' but using this program with /f=DETACHED_PROCESS and Apache starts completely in the background.  I developed this primarily for Apache (way overkill for one feature), but I've had need for this program for other things on many different occasions.

GitHub is a perfect fit for this sort of project.  The latest source and binaries reside here, so updating is a matter of running a 'git pull'.  You can also use it as a submodule in your own Git project.  If you know of any changes that need to be made, submit a pull request so that everyone benefits.

Features
--------

* Command-line action!
* Verbose mode tells you exactly how CreateProcess() will be called.  No more guessing!
* Can redirect stdin, stdout, and stderr to TCP/IP sockets.  Avoid blocking on anonymous pipes or storing output in files!
* Pre-built binaries against Visual Studio (statically linked C++ runtime, minimal file size of ~100K, direct Win32 API calls).
* Unicode support.
* Offers almost everything CreateProcess() offers plus a couple of nice extras (e.g. output the process ID to a file).
* Has a liberal open source license.  MIT or LGPL, your choice.
* Sits on GitHub for all of that pull request and issue tracker goodness to easily submit changes and ideas respectively.

Useful Information
------------------

Running the command by itself will display the options, but it never hurts to have a complete rundown of the options:

```
Syntax:  createprocess [options] EXEToRun [arguments]

Options:
        /v
        Verbose mode.

        /w[=Milliseconds]
        Waits for the process to complete before exiting.
        The default behavior is to return immediately.
        If Milliseconds is specified, the number of milliseconds to wait.
        Return code, if any, is returned to caller.

        /pid=File
        Writes the process ID to the specified file.

        /term
        Used with /w=Milliseconds.
        Terminates the process when the wait time is up.

        /f=PriorityClass
                Sets the priority class of the new process.
                There is only one priority class per process.
                The 'PriorityClass' can be one of:
                ABOVE_NORMAL_PRIORITY_CLASS
                BELOW_NORMAL_PRIORITY_CLASS
                HIGH_PRIORITY_CLASS
                IDLE_PRIORITY_CLASS
                NORMAL_PRIORITY_CLASS
                REALTIME_PRIORITY_CLASS

        /f=CreateFlag
                Sets a creation flag for the new process.
                Multiple -f options can be specified.
                Each 'CreateFlag' can be one of:
                CREATE_DEFAULT_ERROR_MODE
                CREATE_NEW_CONSOLE
                CREATE_NEW_PROCESS_GROUP
                CREATE_NO_WINDOW
                CREATE_PROTECTED_PROCESS
                CREATE_PRESERVE_CODE_AUTHZ_LEVEL
                CREATE_SEPARATE_WOW_VDM
                CREATE_SHARED_WOW_VDM
                DEBUG_ONLY_THIS_PROCESS
                DEBUG_PROCESS
                DETACHED_PROCESS
                INHERIT_PARENT_AFFINITY

        /dir=StartDir
                Sets the starting directory of the new process.

        /desktop=Desktop
                Sets the STARTUPINFO.lpDesktop member to target a specific desktop.

        /title=WindowTitle
                Sets the STARTUPINFO.lpTitle member to a specific title.

        /x=XPositionInPixels
                Sets the STARTUPINFO.dwX member to a specific x-axis position, in pixels.

        /y=YPositionInPixels
                Sets the STARTUPINFO.dwY member to a specific y-axis position, in pixels.

        /width=WidthInPixels
                Sets the STARTUPINFO.dwXSize member to a specific width, in pixels.

        /height=HeightInPixels
                Sets the STARTUPINFO.dwYSize member to a specific height, in pixels.

        /xchars=BufferWidthInCharacters
                Sets the STARTUPINFO.dwXCountChars member to buffer width, in characters.

        /ychars=BufferHeightInCharacters
                Sets the STARTUPINFO.dwYCountChars member to buffer height, in characters.

        /f=FillAttribute
                Sets the STARTUPINFO.dwFillAttribute member to buffer height, in characters.
                Multiple -si_fill options can be specified.
                Each 'FillAttribute' can be one of:
                FOREGROUND_RED
                FOREGROUND_GREEN
                FOREGROUND_BLUE
                FOREGROUND_INTENSITY
                BACKGROUND_RED
                BACKGROUND_GREEN
                BACKGROUND_BLUE
                BACKGROUND_INTENSITY

        /f=StartupFlag
                Sets the STARTUPINFO.dwFlags flag for the new process.
                Multiple -si_f options can be specified.
                Each 'StartupFlag' can be one of:
                STARTF_FORCEONFEEDBACK
                STARTF_FORCEOFFFEEDBACK
                STARTF_PREVENTPINNING
                STARTF_RUNFULLSCREEN
                STARTF_TITLEISAPPID
                STARTF_TITLEISLINKNAME

        /f=ShowWindow
                Sets the STARTUPINFO.wShowWindow flag for the new process.
                There is only one show window option per process.
                The 'ShowWindow' value can be one of:
                SW_FORCEMINIMIZE
                SW_HIDE
                SW_MAXIMIZE
                SW_MINIMIZE
                SW_RESTORE
                SW_SHOW
                SW_SHOWDEFAULT
                SW_SHOWMAXIMIZED
                SW_SHOWMINIMIZED
                SW_SHOWMINNOACTIVE
                SW_SHOWNA
                SW_SHOWNOACTIVATE
                SW_SHOWNORMAL

        /hotkey=HotkeyValue
                Sets the STARTUPINFO.hStdInput handle for the new process.
                Specifies the wParam member of a WM_SETHOKEY message to the new process.

        /socketip=IPAddress
                Specifies the IP address to connect to over TCP/IP.

        /socketport=PortNumber
                Specifies the port number to connect to over TCP/IP.

        /sockettoken=Token
                Specifies the token to send to each socket.
                Less secure than using /sockettokenlen and stdin.

        /sockettokenlen=TokenLength
                Specifies the length of the token to read from stdin.
                When specified, a token must be sent for each socket.

        /stdin=FileOrEmptyOrsocket
                Sets the STARTUPINFO.hStdInput handle for the new process.
                When this option is empty, INVALID_HANDLE_VALUE is used.
                When this option is 'socket', the /socket IP and port are used.
                When this option is not specified, the current stdin is used.

        /stdout=FileOrEmptyOrsocket
                Sets the STARTUPINFO.hStdOutput handle for the new process.
                When this option is empty, INVALID_HANDLE_VALUE is used.
                When this option is 'socket', the /socket IP and port are used.
                When this option is not specified, the current stdout is used.

        /stderr=FileOrEmptyOrstdoutOrsocket
                Sets the STARTUPINFO.hStdError handle for the new process.
                When this option is empty, INVALID_HANDLE_VALUE is used.
                When this option is 'stdout', the value of stdout is used.
                When this option is 'socket', the /socket IP and port are used.
                When this option is not specified, the current stderr is used.
```

That's pretty nifty.  But what is even more impressive is the verbose output option that shows exactly how CreateProcess() will be called.  I'll let you take that for a spin though.

Even the most hardcore command-line enthusiast should be drooling right now.  Be sure to check out the source code.

Sources
-------

The CreateProcess() API on MSDN Library has the intimate details on each option:

http://msdn.microsoft.com/en-us/library/windows/desktop/ms682425%28v=vs.85%29.aspx
