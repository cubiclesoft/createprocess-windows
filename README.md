CreateProcess() Windows API Command-Line Utility
================================================

A complete, robust command-line utility to construct highly customized calls to the CreateProcess() Windows API to start new processes.  Released under a MIT or LGPL license.

This project is intended primarily for use from batch files (.bat) to execute other programs.  If it can be done with CreateProcess(), it can be done with this command-line program.

Why would you need this?  One use-case would be for programs that don't play nice even with the 'start' command.  For example, Apache 'httpd.exe' hangs the command-line even with 'start /B /MIN' but running this program with `/f=DETACHED_PROCESS` to start Apache and it runs completely in the background.  I developed this initially for Apache (way overkill for one feature), but I've had need for this program for other things on many different occasions, including starting processes from scripting languages that don't offer sufficient facilities on Windows.

Learn about security tokens, advanced usage of the CreateProcess command-line tool, and much more:

[![Windows Security Objects:  A Crash Course + A Brand New Way to Start Processes on Microsoft Windows video](https://user-images.githubusercontent.com/1432111/118288197-0574ec00-b489-11eb-96e5-fab0f6149171.png)](https://www.youtube.com/watch?v=pmteqkbBfAY "Windows Security Objects:  A Crash Course + A Brand New Way to Start Processes on Microsoft Windows")

[![Donate](https://cubiclesoft.com/res/donate-shield.png)](https://cubiclesoft.com/donate/) [![Discord](https://img.shields.io/discord/777282089980526602?label=chat&logo=discord)](https://cubiclesoft.com/product-support/github/)

Features
--------

* Command-line action!
* Verbose mode tells you exactly how CreateProcess() will be called.  No more guessing!
* Can redirect stdin, stdout, and stderr to TCP/IP sockets.  Avoid blocking on anonymous pipes or storing output in files!
* Can use named mutexes and semaphores to control how many processes can run at the same time.
* Start elevated processes (UAC support).
* Start child processes as any valid user without requiring the user's credentials.  Including the powerful NT AUTHORITY\SYSTEM account!
* Pre-built binaries using Visual Studio (statically linked C++ runtime, minimal file size of ~162K, direct Win32 API calls).
* Console and Windows subsystem variants.
* Unicode support.
* Offers almost everything CreateProcess() offers plus a couple of nice extras (e.g. output the process ID to a file).
* Has a liberal open source license.  MIT or LGPL, your choice.
* Sits on GitHub for all of that pull request and issue tracker goodness to easily submit changes and ideas respectively.

Useful Information
------------------

Running the command by itself will display the options:

```
(C) 2021 CubicleSoft.  All Rights Reserved.

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

        /runelevated
        Calls CreateProcess() as a high Integrity Level elevated process.
        /w should be specified before this option.
        May trigger elevation.  Not compatible with /term when not elevated.

        /elevatedtoken
        Uses an elevated token to create a child process.
        May create a temporary SYSTEM service to copy the primary token via
        undocumented Windows kernel APIs.

        /systemtoken
        Uses a SYSTEM token to create a child process.
        May create a temporary SYSTEM service to copy the primary token via
        undocumented Windows kernel APIs.

        /usetoken=PIDorSIDsAndPrivileges
        Uses the primary token of the specified process ID,
        or a process matching specific comma-separated user/group SIDs
        and/or a process with specific privileges.
        May trigger elevation.  See /elevatedtoken.

        /createtoken=Parameters
        Creates a primary token from scratch.
        May trigger elevation.  See /elevatedtoken.
        Uses an undocumented Windows kernel API.
        The 'Parameters' are semicolon separated:
                UserSID;
                GroupSID:Attr,GroupSID:Attr,...;
                Privilege:Attr,Privilege:Attr,...;
                OwnerSID;
                PrimaryGroupSID;
                DefaultDACL;
                SourceInHex:SourceLUID

        /mergeenv
        Merges the current environment with another user environment.
        Use with /elevatedtoken, /systemtoken, /usetoken, /createtoken.

        /mutex=MutexName
        Creates a mutex with the specified name.
        Use the named mutex with /singleton or other software
        to detect an already running instance.

        /singleton[=Milliseconds]
        Only starts the target process if named /mutex is the only instance.
        If Milliseconds is specified, the number of milliseconds to wait.

        /semaphore=MaxCount,SemaphoreName
        Creates a semaphore with the specified name and limit/count.
        Use the named semaphore with /multiton
        to limit the number of running processes.

        /multiton[=Milliseconds]
        Checks or waits for a named /semaphore.
        If Milliseconds is specified, the number of milliseconds to wait.

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
        Multiple /f options can be specified.
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
        Sets the STARTUPINFO.dwFillAttribute member text and background colors.
        Multiple /f options can be specified.
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
        Multiple /f options can be specified.
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

        /attach[=ProcessID]
        Attempt to attach to a parent OR a specific process' console.
        Also resets standard handles back to defaults.
```

Example usage:

```
C:\>createprocess /f=DETACHED_PROCESS "C:\Program Files\Apache\httpd.exe"
```

That starts Apache with a detached console so it runs entirely in the background.

Another example:

```
C:\>whoami
my-pc\john-doh

C:\>whoami /priv

PRIVILEGES INFORMATION
----------------------

Privilege Name                Description                          State
============================= ==================================== ========
SeShutdownPrivilege           Shut down the system                 Disabled
SeChangeNotifyPrivilege       Bypass traverse checking             Enabled
SeUndockPrivilege             Remove computer from docking station Disabled
SeIncreaseWorkingSetPrivilege Increase a process working set       Disabled
SeTimeZonePrivilege           Change the time zone                 Disabled

C:\>set MYVAR=123

C:\>createprocess /w /systemtoken /mergeenv C:\Windows\System32\cmd.exe
Microsoft Windows [Version 10.0.19042.867]
(c) 2020 Microsoft Corporation. All rights reserved.

C:\>whoami
nt authority\system

C:\>whoami /priv

PRIVILEGES INFORMATION
----------------------

Privilege Name                            Description                                                        State
========================================= ================================================================== ========
SeAssignPrimaryTokenPrivilege             Replace a process level token                                      Enabled
SeLockMemoryPrivilege                     Lock pages in memory                                               Enabled
SeIncreaseQuotaPrivilege                  Adjust memory quotas for a process                                 Enabled
SeTcbPrivilege                            Act as part of the operating system                                Enabled
SeSecurityPrivilege                       Manage auditing and security log                                   Disabled
SeTakeOwnershipPrivilege                  Take ownership of files or other objects                           Disabled
SeLoadDriverPrivilege                     Load and unload device drivers                                     Disabled
SeDebugPrivilege                          Debug programs                                                     Enabled
...  [It's a fairly lengthy list of powerful privileges]
SeDelegateSessionUserImpersonatePrivilege Obtain an impersonation token for another user in the same session Enabled

C:\>set MYVAR
MYVAR=123
```

That starts a Command Prompt child process as NT AUTHORITY\SYSTEM (the most powerful user account in Windows) in the same console session of the parent non-elevated process with full SYSTEM privileges and merges the current environment variables with the SYSTEM user's environment variables.

Learn how the various `/...token` options work by watching the video linked to at the top of this repo or just [look at the insane diagram](https://github.com/cubiclesoft/createprocess-windows/blob/master/starting_a_child_process_as_another_user_diagram.png).

Even the most hardcore command-line enthusiast should be drooling right now due to brain melt.  Be sure to check out the source code.

Windows Subsystem Variant
-------------------------

While `createprocess.exe` is intended for use with console apps, `createprocess-win.exe` is intended for detached console and GUI applications.  Starting `createprocess.exe` in certain situations will briefly flash a console window before starting the target process.  Calling `createprocess-win.exe` instead will no longer show the console window.

Why not just use `createprocess-win.exe`?  Since `createprocess-win.exe` starts as a Windows GUI application, there is the tendency for it to be run in the background and may not behave as expected with various handles.  The software is a little bit trickier to work with as a result.  It's also a few KB larger than `createprocess.exe`.

There is one additional option specifically for `createprocess-win.exe` called `/attach` which attempts to attach to the console of the parent process (if any) and will also reset the standard handles.  The `/attach` option, if used, should generally be specified before other options.

TCP/IP Notes
------------

The TCP/IP socket options represent a security risk so take proper precautions.  Example usage can be seen in the [ProcessHelper class](https://github.com/cubiclesoft/php-misc/blob/master/support/process_helper.php).

In addition, passing SOCKET handles to the target process causes problems.  Sometimes the target process works just fine and sometimes it doesn't.  To deal with this issue, up to three threads are started, one for each of the standard handles.  Each thread routes data between its socket handle and an associated anonymous pipe of the started process.  As a consequence of using the TCP/IP socket option, the `/w` option is always set so that the started process is waited on (i.e. so the threads can transfer data).  This doesn't exactly matter as the `/w` option would be used anyway by the caller when passing socket options.

Sources
-------

The CreateProcess() API in MSDN Library has the intimate details on most options:

http://msdn.microsoft.com/en-us/library/windows/desktop/ms682425%28v=vs.85%29.aspx

Related Tools
-------------

* [PHP Process Helper Class](https://github.com/cubiclesoft/php-misc) - Uses this software to start processes on Windows in the background with non-blocking stdin/stdout/stderr via TCP/IP sockets.
* [GetTokenInformation](https://github.com/cubiclesoft/gettokeninformation-windows) - Dumps information about Windows security tokens (SIDs, privileges, etc) as JSON.
* [GetSIDInfo](https://github.com/cubiclesoft/getsidinfo-windows) - Dumps information about Windows Security Identifiers (SIDs) as JSON.
