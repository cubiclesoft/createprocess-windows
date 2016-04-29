// A simple program whose sole job is to run custom CreateProcess() commands.
// Useful for executing programs from batch files that don't play nice (e.g. Apache).
// (C) 2016 CubicleSoft.  All Rights Reserved.

// Implemented as a single file rather than relying on my core library because I
// want full Unicode support and other people should be able to build this.

#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#ifdef _MBCS
#undef _MBCS
#endif

#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include <tchar.h>

#ifndef ERROR_ELEVATION_REQUIRED
	#define ERROR_ELEVATION_REQUIRED   740
#endif

#ifndef INHERIT_PARENT_AFFINITY
	#define INHERIT_PARENT_AFFINITY    0x00010000L
#endif

#ifndef STARTF_TITLEISLINKNAME
	#define STARTF_TITLEISLINKNAME     0x00000800L
#endif

#ifndef STARTF_TITLEISAPPID
	#define STARTF_TITLEISAPPID        0x00001000L
#endif

#ifndef STARTF_PREVENTPINNING
	#define STARTF_PREVENTPINNING      0x00002000L
#endif

void DumpSyntax(TCHAR *currfile)
{
	_tprintf(_T("(C) 2012 CubicleSoft.  All Rights Reserved.\n\n"));

	_tprintf(_T("Syntax:  %s [options] EXEToRun [arguments]\n\n"), currfile);

	_tprintf(_T("Options:\n"));

	_tprintf(_T("\t/v\n"));
	_tprintf(_T("\tVerbose mode.\n\n"));

	_tprintf(_T("\t/w[=Milliseconds]\n"));
	_tprintf(_T("\tWaits for the process to complete before exiting.\n"));
	_tprintf(_T("\tThe default behavior is to return immediately.\n"));
	_tprintf(_T("\tIf Milliseconds is specified, the number of milliseconds to wait.\n"));
	_tprintf(_T("\tReturn code, if any, is returned to caller.\n\n"));

	_tprintf(_T("\t/pid=File\n"));
	_tprintf(_T("\tWrites the process ID to the specified file.\n\n"));

	_tprintf(_T("\t/term\n"));
	_tprintf(_T("\tUsed with /w=Milliseconds.\n"));
	_tprintf(_T("\tTerminates the process when the wait time is up.\n\n"));

	_tprintf(_T("\t/f=PriorityClass\n"));
	_tprintf(_T("\t\tSets the priority class of the new process.\n"));
	_tprintf(_T("\t\tThere is only one priority class per process.\n"));
	_tprintf(_T("\t\tThe 'PriorityClass' can be one of:\n"));
	_tprintf(_T("\t\tABOVE_NORMAL_PRIORITY_CLASS\n"));
	_tprintf(_T("\t\tBELOW_NORMAL_PRIORITY_CLASS\n"));
	_tprintf(_T("\t\tHIGH_PRIORITY_CLASS\n"));
	_tprintf(_T("\t\tIDLE_PRIORITY_CLASS\n"));
	_tprintf(_T("\t\tNORMAL_PRIORITY_CLASS\n"));
	_tprintf(_T("\t\tREALTIME_PRIORITY_CLASS\n\n"));

	_tprintf(_T("\t/f=CreateFlag\n"));
	_tprintf(_T("\t\tSets a creation flag for the new process.\n"));
	_tprintf(_T("\t\tMultiple /f options can be specified.\n"));
	_tprintf(_T("\t\tEach 'CreateFlag' can be one of:\n"));
	_tprintf(_T("\t\tCREATE_DEFAULT_ERROR_MODE\n"));
	_tprintf(_T("\t\tCREATE_NEW_CONSOLE\n"));
	_tprintf(_T("\t\tCREATE_NEW_PROCESS_GROUP\n"));
	_tprintf(_T("\t\tCREATE_NO_WINDOW\n"));
	_tprintf(_T("\t\tCREATE_PROTECTED_PROCESS\n"));
	_tprintf(_T("\t\tCREATE_PRESERVE_CODE_AUTHZ_LEVEL\n"));
	_tprintf(_T("\t\tCREATE_SEPARATE_WOW_VDM\n"));
	_tprintf(_T("\t\tCREATE_SHARED_WOW_VDM\n"));
	_tprintf(_T("\t\tDEBUG_ONLY_THIS_PROCESS\n"));
	_tprintf(_T("\t\tDEBUG_PROCESS\n"));
	_tprintf(_T("\t\tDETACHED_PROCESS\n"));
	_tprintf(_T("\t\tINHERIT_PARENT_AFFINITY\n\n"));

	_tprintf(_T("\t/dir=StartDir\n"));
	_tprintf(_T("\t\tSets the starting directory of the new process.\n\n"));

	_tprintf(_T("\t/desktop=Desktop\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.lpDesktop member to target a specific desktop.\n\n"));

	_tprintf(_T("\t/title=WindowTitle\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.lpTitle member to a specific title.\n\n"));

	_tprintf(_T("\t/x=XPositionInPixels\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwX member to a specific x-axis position, in pixels.\n\n"));

	_tprintf(_T("\t/y=YPositionInPixels\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwY member to a specific y-axis position, in pixels.\n\n"));

	_tprintf(_T("\t/width=WidthInPixels\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwXSize member to a specific width, in pixels.\n\n"));

	_tprintf(_T("\t/height=HeightInPixels\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwYSize member to a specific height, in pixels.\n\n"));

	_tprintf(_T("\t/xchars=BufferWidthInCharacters\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwXCountChars member to buffer width, in characters.\n\n"));

	_tprintf(_T("\t/ychars=BufferHeightInCharacters\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwYCountChars member to buffer height, in characters.\n\n"));

	_tprintf(_T("\t/f=FillAttribute\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwFillAttribute member to buffer height, in characters.\n"));
	_tprintf(_T("\t\tMultiple /f options can be specified.\n"));
	_tprintf(_T("\t\tEach 'FillAttribute' can be one of:\n"));
	_tprintf(_T("\t\tFOREGROUND_RED\n"));
	_tprintf(_T("\t\tFOREGROUND_GREEN\n"));
	_tprintf(_T("\t\tFOREGROUND_BLUE\n"));
	_tprintf(_T("\t\tFOREGROUND_INTENSITY\n"));
	_tprintf(_T("\t\tBACKGROUND_RED\n"));
	_tprintf(_T("\t\tBACKGROUND_GREEN\n"));
	_tprintf(_T("\t\tBACKGROUND_BLUE\n"));
	_tprintf(_T("\t\tBACKGROUND_INTENSITY\n\n"));

	_tprintf(_T("\t/f=StartupFlag\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.dwFlags flag for the new process.\n"));
	_tprintf(_T("\t\tMultiple /f options can be specified.\n"));
	_tprintf(_T("\t\tEach 'StartupFlag' can be one of:\n"));
	_tprintf(_T("\t\tSTARTF_FORCEONFEEDBACK\n"));
	_tprintf(_T("\t\tSTARTF_FORCEOFFFEEDBACK\n"));
	_tprintf(_T("\t\tSTARTF_PREVENTPINNING\n"));
	_tprintf(_T("\t\tSTARTF_RUNFULLSCREEN\n"));
	_tprintf(_T("\t\tSTARTF_TITLEISAPPID\n"));
	_tprintf(_T("\t\tSTARTF_TITLEISLINKNAME\n\n"));

	_tprintf(_T("\t/f=ShowWindow\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.wShowWindow flag for the new process.\n"));
	_tprintf(_T("\t\tThere is only one show window option per process.\n"));
	_tprintf(_T("\t\tThe 'ShowWindow' value can be one of:\n"));
	_tprintf(_T("\t\tSW_FORCEMINIMIZE\n"));
	_tprintf(_T("\t\tSW_HIDE\n"));
	_tprintf(_T("\t\tSW_MAXIMIZE\n"));
	_tprintf(_T("\t\tSW_MINIMIZE\n"));
	_tprintf(_T("\t\tSW_RESTORE\n"));
	_tprintf(_T("\t\tSW_SHOW\n"));
	_tprintf(_T("\t\tSW_SHOWDEFAULT\n"));
	_tprintf(_T("\t\tSW_SHOWMAXIMIZED\n"));
	_tprintf(_T("\t\tSW_SHOWMINIMIZED\n"));
	_tprintf(_T("\t\tSW_SHOWMINNOACTIVE\n"));
	_tprintf(_T("\t\tSW_SHOWNA\n"));
	_tprintf(_T("\t\tSW_SHOWNOACTIVATE\n"));
	_tprintf(_T("\t\tSW_SHOWNORMAL\n\n"));

	_tprintf(_T("\t/hotkey=HotkeyValue\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.hStdInput handle for the new process.\n"));
	_tprintf(_T("\t\tSpecifies the wParam member of a WM_SETHOKEY message to the new process.\n\n"));

	_tprintf(_T("\t/stdin=FileOrEmpty\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.hStdInput handle for the new process.\n"));
	_tprintf(_T("\t\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n"));
	_tprintf(_T("\t\tWhen this option is not specified, the current stdin is used.\n\n"));

	_tprintf(_T("\t/stdout=FileOrEmpty\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.hStdOutput handle for the new process.\n"));
	_tprintf(_T("\t\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n"));
	_tprintf(_T("\t\tWhen this option is not specified, the current stdout is used.\n\n"));

	_tprintf(_T("\t/stderr=FileOrEmptyOrstdout\n"));
	_tprintf(_T("\t\tSets the STARTUPINFO.hStdError handle for the new process.\n"));
	_tprintf(_T("\t\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n"));
	_tprintf(_T("\t\tWhen this option is 'stdout', the value of stdout is used.\n"));
	_tprintf(_T("\t\tWhen this option is not specified, the current stderr is used.\n\n"));
}

int _tmain(int argc, TCHAR **argv)
{
	bool verbose = false;
	bool wait = false;
	bool terminate = false;
	DWORD waitamount = INFINITE;
	LPTSTR pidfile = NULL;
	DWORD priorityflag = 0;
#ifdef UNICODE
	DWORD createflags = CREATE_UNICODE_ENVIRONMENT;
#else
	DWORD createflags = 0;
#endif
	LPTSTR startdir = NULL;
	LPTSTR appname = NULL;
	STARTUPINFO startinfo = {0};
	SECURITY_ATTRIBUTES secattr = {0};
	TCHAR *stdinstr = _T(":stdin");
	TCHAR *stdoutstr = _T(":stdout");
	TCHAR *stderrstr = _T(":stderr");
	PROCESS_INFORMATION procinfo = {0};
	DWORD exitcode = 0;

	if (argc < 3)
	{
		DumpSyntax(argv[0]);

		return 1;
	}

	// Initialize structures.
	startinfo.cb = sizeof(startinfo);
	startinfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
	startinfo.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
	startinfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
	startinfo.dwFlags |= STARTF_USESTDHANDLES;

	secattr.nLength = sizeof(secattr);
	secattr.bInheritHandle = TRUE;
	secattr.lpSecurityDescriptor = NULL;

	// Process command-line options.
	int x;
	for (x = 1; x < argc; x++)
	{
		if (!_tcsicmp(argv[x], _T("/v")))  verbose = true;
		else if (!_tcsicmp(argv[x], _T("/w")))  wait = true;
		else if (!_tcsncicmp(argv[x], _T("/w="), 3))
		{
			wait = true;
			waitamount = _tstoi(argv[x] + 3);
		}
		else if (!_tcsncicmp(argv[x], _T("/pid="), 5))  pidfile = argv[x] + 5;
		else if (!_tcsicmp(argv[x], _T("/term")))  terminate = true;
		else if (!_tcsicmp(argv[x], _T("/f=ABOVE_NORMAL_PRIORITY_CLASS")))  priorityflag = ABOVE_NORMAL_PRIORITY_CLASS;
		else if (!_tcsicmp(argv[x], _T("/f=BELOW_NORMAL_PRIORITY_CLASS")))  priorityflag = BELOW_NORMAL_PRIORITY_CLASS;
		else if (!_tcsicmp(argv[x], _T("/f=HIGH_PRIORITY_CLASS")))  priorityflag = HIGH_PRIORITY_CLASS;
		else if (!_tcsicmp(argv[x], _T("/f=IDLE_PRIORITY_CLASS")))  priorityflag = IDLE_PRIORITY_CLASS;
		else if (!_tcsicmp(argv[x], _T("/f=NORMAL_PRIORITY_CLASS")))  priorityflag = NORMAL_PRIORITY_CLASS;
		else if (!_tcsicmp(argv[x], _T("/f=REALTIME_PRIORITY_CLASS")))  priorityflag = REALTIME_PRIORITY_CLASS;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_DEFAULT_ERROR_MODE")))  createflags |= CREATE_DEFAULT_ERROR_MODE;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_NEW_CONSOLE")))  createflags |= CREATE_NEW_CONSOLE;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_NEW_PROCESS_GROUP")))  createflags |= CREATE_NEW_PROCESS_GROUP;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_NO_WINDOW")))  createflags |= CREATE_NO_WINDOW;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_PROTECTED_PROCESS")))  createflags |= CREATE_PROTECTED_PROCESS;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_PRESERVE_CODE_AUTHZ_LEVEL")))  createflags |= CREATE_PRESERVE_CODE_AUTHZ_LEVEL;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_SEPARATE_WOW_VDM")))  createflags |= CREATE_SEPARATE_WOW_VDM;
		else if (!_tcsicmp(argv[x], _T("/f=CREATE_SHARED_WOW_VDM")))  createflags |= CREATE_SHARED_WOW_VDM;
		else if (!_tcsicmp(argv[x], _T("/f=DEBUG_ONLY_THIS_PROCESS")))  createflags |= DEBUG_ONLY_THIS_PROCESS;
		else if (!_tcsicmp(argv[x], _T("/f=DEBUG_PROCESS")))  createflags |= DEBUG_PROCESS;
		else if (!_tcsicmp(argv[x], _T("/f=DETACHED_PROCESS")))  createflags |= DETACHED_PROCESS;
		else if (!_tcsicmp(argv[x], _T("/f=INHERIT_PARENT_AFFINITY")))  createflags |= INHERIT_PARENT_AFFINITY;
		else if (!_tcsncicmp(argv[x], _T("/dir="), 5))  startdir = argv[x] + 5;
		else if (!_tcsncicmp(argv[x], _T("/desktop="), 9))  startinfo.lpDesktop = argv[x] + 9;
		else if (!_tcsncicmp(argv[x], _T("/title="), 7))  startinfo.lpTitle = argv[x] + 7;
		else if (!_tcsncicmp(argv[x], _T("/x="), 3))
		{
			startinfo.dwX = _tstoi(argv[x] + 3);
			startinfo.dwFlags |= STARTF_USEPOSITION;
		}
		else if (!_tcsncicmp(argv[x], _T("/y="), 3))
		{
			startinfo.dwY = _tstoi(argv[x] + 3);
			startinfo.dwFlags |= STARTF_USEPOSITION;
		}
		else if (!_tcsncicmp(argv[x], _T("/width="), 7))
		{
			startinfo.dwXSize = _tstoi(argv[x] + 7);
			startinfo.dwFlags |= STARTF_USESIZE;
		}
		else if (!_tcsncicmp(argv[x], _T("/height="), 8))
		{
			startinfo.dwYSize = _tstoi(argv[x] + 8);
			startinfo.dwFlags |= STARTF_USESIZE;
		}
		else if (!_tcsncicmp(argv[x], _T("/xchars="), 8))
		{
			startinfo.dwXCountChars = _tstoi(argv[x] + 8);
			startinfo.dwFlags |= STARTF_USECOUNTCHARS;
		}
		else if (!_tcsncicmp(argv[x], _T("/ychars="), 8))
		{
			startinfo.dwYCountChars = _tstoi(argv[x] + 8);
			startinfo.dwFlags |= STARTF_USECOUNTCHARS;
		}
		else if (!_tcsicmp(argv[x], _T("/f=FOREGROUND_RED")))  startinfo.dwFillAttribute |= FOREGROUND_RED;
		else if (!_tcsicmp(argv[x], _T("/f=FOREGROUND_GREEN")))  startinfo.dwFillAttribute |= FOREGROUND_GREEN;
		else if (!_tcsicmp(argv[x], _T("/f=FOREGROUND_BLUE")))  startinfo.dwFillAttribute |= FOREGROUND_BLUE;
		else if (!_tcsicmp(argv[x], _T("/f=FOREGROUND_INTENSITY")))  startinfo.dwFillAttribute |= FOREGROUND_INTENSITY;
		else if (!_tcsicmp(argv[x], _T("/f=BACKGROUND_RED")))  startinfo.dwFillAttribute |= BACKGROUND_RED;
		else if (!_tcsicmp(argv[x], _T("/f=BACKGROUND_GREEN")))  startinfo.dwFillAttribute |= BACKGROUND_GREEN;
		else if (!_tcsicmp(argv[x], _T("/f=BACKGROUND_BLUE")))  startinfo.dwFillAttribute |= BACKGROUND_BLUE;
		else if (!_tcsicmp(argv[x], _T("/f=BACKGROUND_INTENSITY")))  startinfo.dwFillAttribute |= BACKGROUND_INTENSITY;
		else if (!_tcsicmp(argv[x], _T("/f=STARTF_FORCEONFEEDBACK")))  startinfo.dwFlags |= STARTF_FORCEONFEEDBACK;
		else if (!_tcsicmp(argv[x], _T("/f=STARTF_FORCEOFFFEEDBACK")))  startinfo.dwFlags |= STARTF_FORCEOFFFEEDBACK;
		else if (!_tcsicmp(argv[x], _T("/f=STARTF_PREVENTPINNING")))  startinfo.dwFlags |= STARTF_PREVENTPINNING;
		else if (!_tcsicmp(argv[x], _T("/f=STARTF_RUNFULLSCREEN")))  startinfo.dwFlags |= STARTF_RUNFULLSCREEN;
		else if (!_tcsicmp(argv[x], _T("/f=STARTF_TITLEISAPPID")))  startinfo.dwFlags |= STARTF_TITLEISAPPID;
		else if (!_tcsicmp(argv[x], _T("/f=STARTF_TITLEISLINKNAME")))  startinfo.dwFlags |= STARTF_TITLEISLINKNAME;
		else if (!_tcsicmp(argv[x], _T("/f=SW_FORCEMINIMIZE")))  startinfo.wShowWindow = SW_FORCEMINIMIZE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_HIDE")))  startinfo.wShowWindow = SW_HIDE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_MAXIMIZE")))  startinfo.wShowWindow = SW_MAXIMIZE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_MINIMIZE")))  startinfo.wShowWindow = SW_MINIMIZE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_RESTORE")))  startinfo.wShowWindow = SW_RESTORE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOW")))  startinfo.wShowWindow = SW_SHOW;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWDEFAULT")))  startinfo.wShowWindow = SW_SHOWDEFAULT;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWMAXIMIZED")))  startinfo.wShowWindow = SW_SHOWMAXIMIZED;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWMINIMIZED")))  startinfo.wShowWindow = SW_SHOWMINIMIZED;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWMINNOACTIVE")))  startinfo.wShowWindow = SW_SHOWMINNOACTIVE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWNA")))  startinfo.wShowWindow = SW_SHOWNA;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWNOACTIVATE")))  startinfo.wShowWindow = SW_SHOWNOACTIVATE;
		else if (!_tcsicmp(argv[x], _T("/f=SW_SHOWNORMAL")))  startinfo.wShowWindow = SW_SHOWNORMAL;
		else if (!_tcsncicmp(argv[x], _T("/hotkey="), 8))
		{
			startinfo.hStdInput = UlongToHandle(_tstoi(argv[x] + 8));
			stdinstr = _T(":hotkey");

			startinfo.dwFlags |= STARTF_USEHOTKEY;
			startinfo.dwFlags ^= ~STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/stdin="), 7))
		{
			if (argv[x][7] == _T('\0'))
			{
				startinfo.hStdInput = NULL;
				stdinstr = _T(":null");
			}
			else
			{
				startinfo.hStdInput = ::CreateFile(argv[x] + 7, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &secattr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				stdinstr = argv[x] + 7;
			}

			startinfo.dwFlags ^= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/stdout="), 8))
		{
			if (argv[x][8] == _T('\0'))
			{
				startinfo.hStdOutput = NULL;
				stdoutstr = _T(":null");
			}
			else if (!_tcsicmp(argv[x] + 8, _T("stderr")))
			{
				startinfo.hStdOutput = startinfo.hStdError;
				stdoutstr = stderrstr;
			}
			else
			{
				startinfo.hStdOutput = ::CreateFile(argv[x] + 8, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &secattr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				stdoutstr = argv[x] + 8;
				if (startinfo.hStdOutput != INVALID_HANDLE_VALUE)
				{
					LARGE_INTEGER filepos = {0};
					::SetFilePointerEx(startinfo.hStdOutput, filepos, NULL, FILE_END);
				}
			}

			startinfo.dwFlags ^= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/stderr="), 8))
		{
			if (argv[x][8] == _T('\0'))
			{
				startinfo.hStdError = NULL;
				stderrstr = _T(":null");
			}
			else if (!_tcsicmp(argv[x] + 8, _T("stdout")))
			{
				startinfo.hStdError = startinfo.hStdOutput;
				stderrstr = stdoutstr;
			}
			else
			{
				startinfo.hStdError = ::CreateFile(argv[x] + 8, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &secattr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				stderrstr = argv[x] + 8;
				if (startinfo.hStdError != INVALID_HANDLE_VALUE)
				{
					LARGE_INTEGER filepos = {0};
					::SetFilePointerEx(startinfo.hStdError, filepos, NULL, FILE_END);
				}
			}

			startinfo.dwFlags ^= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else
		{
			// Probably reached the command to execute portion of the arguments.
			break;
		}
	}

	// Failed to find required executable.
	if (x == argc)
	{
		_tprintf(_T("Error:  'EXEToRun' not specified.\n\n"));
		DumpSyntax(argv[0]);

		return 1;
	}

	// Application name.
	appname = argv[x];

	// Piece together the command-line.
	size_t z = _tcslen(appname) + 2;
	size_t z2, z3;
	for (int y = x + 1; y < argc; y++)
	{
		z += _tcslen(argv[y]) + 3;
	}
	LPTSTR commandline = new TCHAR[z + 1];
	z = 0;
	commandline[z] = _T('\"');
	z++;
	z3 = _tcslen(appname);
	for (z2 = 0; z2 < z3; z2++)
	{
		commandline[z] = appname[z2];
		z++;
	}
	commandline[z] = _T('\"');
	z++;
	for (int y = x + 1; y < argc; y++)
	{
		commandline[z] = _T(' ');
		z++;

		z3 = _tcslen(argv[y]);
		for (z2 = 0; z2 < z3 && argv[y][z2] != _T(' '); z2++);
		if (z2 < z3)
		{
			commandline[z] = _T('\"');
			z++;

			for (z2 = 0; z2 < z3; z2++)
			{
				commandline[z] = argv[y][z2];
				z++;
			}

			commandline[z] = _T('\"');
			z++;
		}
		else
		{
			for (z2 = 0; z2 < z3; z2++)
			{
				commandline[z] = argv[y][z2];
				z++;
			}
		}
	}
	commandline[z] = _T('\0');

	createflags |= priorityflag;
	if (startinfo.dwFillAttribute)  startinfo.dwFlags |= STARTF_USEFILLATTRIBUTE;
	if (startinfo.hStdInput == INVALID_HANDLE_VALUE)
	{
		startinfo.hStdInput = NULL;
		stdinstr = _T(":null");
	}
	if (startinfo.hStdOutput == INVALID_HANDLE_VALUE)
	{
		startinfo.hStdOutput = NULL;
		stdoutstr = _T(":null");
	}
	if (startinfo.hStdError == INVALID_HANDLE_VALUE)
	{
		startinfo.hStdError = NULL;
		stderrstr = _T(":null");
	}

	// Run verbose output.
	if (verbose)
	{
		_tprintf(_T("Arguments:\n"));
		for (x = 0; x < argc; x++)
		{
			_tprintf(_T("\targv[%d] = %s\n"), x, argv[x]);
		}
		_tprintf(_T("\n"));
		_tprintf(_T("PID File = %s\n"), pidfile);
		_tprintf(_T("\n"));
		_tprintf(_T("CreateProcess(\n"));
		_tprintf(_T("\tlpApplicationName = %s,\n"), appname);
		_tprintf(_T("\tlpCommandLine = %s,\n"), commandline);
		_tprintf(_T("\tlpProcessAttributes = {\n"));
		_tprintf(_T("\t\tnLength = %d,\n"), secattr.nLength);
		_tprintf(_T("\t\tbInheritHandle = %s,\n"), (secattr.bInheritHandle ? _T("TRUE") : _T("FALSE")));
		_tprintf(_T("\t\tlpSecurityDescriptor = 0x%p,\n"), secattr.lpSecurityDescriptor);
		_tprintf(_T("\t},\n"));
		_tprintf(_T("\tlpThreadAttributes = {\n"));
		_tprintf(_T("\t\tnLength = %d,\n"), secattr.nLength);
		_tprintf(_T("\t\tbInheritHandle = %s,\n"), (secattr.bInheritHandle ? _T("TRUE") : _T("FALSE")));
		_tprintf(_T("\t\tlpSecurityDescriptor = 0x%p,\n"), secattr.lpSecurityDescriptor);
		_tprintf(_T("\t},\n"));
		_tprintf(_T("\tbInheritHandles = %s,\n"), (startinfo.dwFlags & STARTF_USESTDHANDLES ? _T("TRUE") : _T("FALSE")));
		_tprintf(_T("\tdwCreationFlags = 0"));
		if (createflags & ABOVE_NORMAL_PRIORITY_CLASS)  _tprintf(_T(" | ABOVE_NORMAL_PRIORITY_CLASS"));
		if (createflags & BELOW_NORMAL_PRIORITY_CLASS)  _tprintf(_T(" | BELOW_NORMAL_PRIORITY_CLASS"));
		if (createflags & HIGH_PRIORITY_CLASS)  _tprintf(_T(" | HIGH_PRIORITY_CLASS"));
		if (createflags & IDLE_PRIORITY_CLASS)  _tprintf(_T(" | IDLE_PRIORITY_CLASS"));
		if (createflags & NORMAL_PRIORITY_CLASS)  _tprintf(_T(" | NORMAL_PRIORITY_CLASS"));
		if (createflags & REALTIME_PRIORITY_CLASS)  _tprintf(_T(" | REALTIME_PRIORITY_CLASS"));
		if (createflags & CREATE_BREAKAWAY_FROM_JOB)  _tprintf(_T(" | CREATE_BREAKAWAY_FROM_JOB"));
		if (createflags & CREATE_DEFAULT_ERROR_MODE)  _tprintf(_T(" | CREATE_DEFAULT_ERROR_MODE"));
		if (createflags & CREATE_NEW_CONSOLE)  _tprintf(_T(" | CREATE_NEW_CONSOLE"));
		if (createflags & CREATE_NEW_PROCESS_GROUP)  _tprintf(_T(" | CREATE_NEW_PROCESS_GROUP"));
		if (createflags & CREATE_NO_WINDOW)  _tprintf(_T(" | CREATE_NO_WINDOW"));
		if (createflags & CREATE_PROTECTED_PROCESS)  _tprintf(_T(" | CREATE_PROTECTED_PROCESS"));
		if (createflags & CREATE_PRESERVE_CODE_AUTHZ_LEVEL)  _tprintf(_T(" | CREATE_PRESERVE_CODE_AUTHZ_LEVEL"));
		if (createflags & CREATE_SEPARATE_WOW_VDM)  _tprintf(_T(" | CREATE_SEPARATE_WOW_VDM"));
		if (createflags & CREATE_SHARED_WOW_VDM)  _tprintf(_T(" | CREATE_SHARED_WOW_VDM"));
		if (createflags & CREATE_SUSPENDED)  _tprintf(_T(" | CREATE_SUSPENDED"));
		if (createflags & CREATE_UNICODE_ENVIRONMENT)  _tprintf(_T(" | CREATE_UNICODE_ENVIRONMENT"));
		if (createflags & DEBUG_ONLY_THIS_PROCESS)  _tprintf(_T(" | DEBUG_ONLY_THIS_PROCESS"));
		if (createflags & DEBUG_PROCESS)  _tprintf(_T(" | DEBUG_PROCESS"));
		if (createflags & DETACHED_PROCESS)  _tprintf(_T(" | DETACHED_PROCESS"));
		if (createflags & EXTENDED_STARTUPINFO_PRESENT)  _tprintf(_T(" | EXTENDED_STARTUPINFO_PRESENT"));
		if (createflags & INHERIT_PARENT_AFFINITY)  _tprintf(_T(" | INHERIT_PARENT_AFFINITY"));
		_tprintf(_T(",\n"));
		_tprintf(_T("\tlpEnvironment = 0x%p,\n"), (void *)NULL);
		_tprintf(_T("\tlpCurrentDirectory = %s,\n"), startdir);
		_tprintf(_T("\tlpStartupInfo = {\n"));
		_tprintf(_T("\t\tcb = %d,\n"), startinfo.cb);
		_tprintf(_T("\t\tlpReserved = %s,\n"), startinfo.lpReserved);
		_tprintf(_T("\t\tlpDesktop = %s,\n"), startinfo.lpDesktop);
		_tprintf(_T("\t\tlpTitle = %s,\n"), startinfo.lpTitle);
		_tprintf(_T("\t\tdwX = %u,\n"), startinfo.dwX);
		_tprintf(_T("\t\tdwY = %u,\n"), startinfo.dwY);
		_tprintf(_T("\t\tdwXSize = %u,\n"), startinfo.dwXSize);
		_tprintf(_T("\t\tdwYSize = %u,\n"), startinfo.dwYSize);
		_tprintf(_T("\t\tdwXCountChars = %u,\n"), startinfo.dwXCountChars);
		_tprintf(_T("\t\tdwYCountChars = %u,\n"), startinfo.dwYCountChars);
		_tprintf(_T("\t\tdwFillAttribute = 0"));
		if (startinfo.dwFillAttribute & FOREGROUND_RED)  _tprintf(_T(" | FOREGROUND_RED"));
		if (startinfo.dwFillAttribute & FOREGROUND_GREEN)  _tprintf(_T(" | FOREGROUND_GREEN"));
		if (startinfo.dwFillAttribute & FOREGROUND_BLUE)  _tprintf(_T(" | FOREGROUND_BLUE"));
		if (startinfo.dwFillAttribute & FOREGROUND_INTENSITY)  _tprintf(_T(" | FOREGROUND_INTENSITY"));
		if (startinfo.dwFillAttribute & BACKGROUND_RED)  _tprintf(_T(" | BACKGROUND_RED"));
		if (startinfo.dwFillAttribute & BACKGROUND_GREEN)  _tprintf(_T(" | BACKGROUND_GREEN"));
		if (startinfo.dwFillAttribute & BACKGROUND_BLUE)  _tprintf(_T(" | BACKGROUND_BLUE"));
		if (startinfo.dwFillAttribute & BACKGROUND_INTENSITY)  _tprintf(_T(" | BACKGROUND_INTENSITY"));
		_tprintf(_T(",\n"));
		_tprintf(_T("\t\tdwFlags = 0"));
		if (startinfo.dwFlags & STARTF_FORCEONFEEDBACK)  _tprintf(_T(" | STARTF_FORCEONFEEDBACK"));
		if (startinfo.dwFlags & STARTF_FORCEOFFFEEDBACK)  _tprintf(_T(" | STARTF_FORCEOFFFEEDBACK"));
		if (startinfo.dwFlags & STARTF_PREVENTPINNING)  _tprintf(_T(" | STARTF_PREVENTPINNING"));
		if (startinfo.dwFlags & STARTF_RUNFULLSCREEN)  _tprintf(_T(" | STARTF_RUNFULLSCREEN"));
		if (startinfo.dwFlags & STARTF_TITLEISAPPID)  _tprintf(_T(" | STARTF_TITLEISAPPID"));
		if (startinfo.dwFlags & STARTF_TITLEISLINKNAME)  _tprintf(_T(" | STARTF_TITLEISLINKNAME"));
		if (startinfo.dwFlags & STARTF_USECOUNTCHARS)  _tprintf(_T(" | STARTF_USECOUNTCHARS"));
		if (startinfo.dwFlags & STARTF_USEFILLATTRIBUTE)  _tprintf(_T(" | STARTF_USEFILLATTRIBUTE"));
		if (startinfo.dwFlags & STARTF_USEHOTKEY)  _tprintf(_T(" | STARTF_USEHOTKEY"));
		if (startinfo.dwFlags & STARTF_USEPOSITION)  _tprintf(_T(" | STARTF_USEPOSITION"));
		if (startinfo.dwFlags & STARTF_USESHOWWINDOW)  _tprintf(_T(" | STARTF_USESHOWWINDOW"));
		if (startinfo.dwFlags & STARTF_USESIZE)  _tprintf(_T(" | STARTF_USESIZE"));
		if (startinfo.dwFlags & STARTF_USESTDHANDLES)  _tprintf(_T(" | STARTF_USESTDHANDLES"));
		_tprintf(_T(",\n"));
		_tprintf(_T("\t\twShowWindow = "));
		if (startinfo.wShowWindow == SW_FORCEMINIMIZE)  _tprintf(_T("SW_FORCEMINIMIZE"));
		else if (startinfo.wShowWindow == SW_HIDE)  _tprintf(_T("SW_HIDE"));
		else if (startinfo.wShowWindow == SW_MAXIMIZE)  _tprintf(_T("SW_MAXIMIZE"));
		else if (startinfo.wShowWindow == SW_MINIMIZE)  _tprintf(_T("SW_MINIMIZE"));
		else if (startinfo.wShowWindow == SW_RESTORE)  _tprintf(_T("SW_RESTORE"));
		else if (startinfo.wShowWindow == SW_SHOW)  _tprintf(_T("SW_SHOW"));
		else if (startinfo.wShowWindow == SW_SHOWDEFAULT)  _tprintf(_T("SW_SHOWDEFAULT"));
		else if (startinfo.wShowWindow == SW_SHOWMAXIMIZED)  _tprintf(_T("SW_SHOWMAXIMIZED"));
		else if (startinfo.wShowWindow == SW_SHOWMINIMIZED)  _tprintf(_T("SW_SHOWMINIMIZED"));
		else if (startinfo.wShowWindow == SW_SHOWMINNOACTIVE)  _tprintf(_T("SW_SHOWMINNOACTIVE"));
		else if (startinfo.wShowWindow == SW_SHOWNA)  _tprintf(_T("SW_SHOWNA"));
		else if (startinfo.wShowWindow == SW_SHOWNOACTIVATE)  _tprintf(_T("SW_SHOWNOACTIVATE"));
		else if (startinfo.wShowWindow == SW_SHOWNORMAL)  _tprintf(_T("SW_SHOWNORMAL"));
		else  _tprintf(_T("(Unknown/Other)"));
		_tprintf(_T(",\n"));
		_tprintf(_T("\t\tcbReserved2 = %u,\n"), (DWORD)startinfo.cbReserved2);
		_tprintf(_T("\t\tlpReserved2 = 0x%p,\n"), startinfo.lpReserved2);
		_tprintf(_T("\t\thStdInput = %s,\n"), stdinstr);
		_tprintf(_T("\t\thStdOutput = %s,\n"), stdoutstr);
		_tprintf(_T("\t\thStdError = %s\n"), stderrstr);
		_tprintf(_T("\t}\n"));
		_tprintf(_T(");\n"));
	}

	// Execute CreateProcess().
	if (!::CreateProcess(appname, commandline, &secattr, &secattr, (startinfo.dwFlags & STARTF_USESTDHANDLES ? TRUE : FALSE), createflags, NULL, startdir, &startinfo, &procinfo))
	{
		DWORD errnum = ::GetLastError();
		LPTSTR errmsg = NULL;

		::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errnum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errmsg, 0, NULL);
		_tprintf(_T("An error occurred while attempting to start the process:\n"));
		if (errmsg == NULL)  _tprintf(_T("%d - Unknown error\n\n"), errnum);
		else
		{
			_tprintf(_T("%d - %s\n"), errnum, errmsg);
			::LocalFree(errmsg);
		}
		if (errnum == ERROR_ELEVATION_REQUIRED)  _tprintf(_T("'%s' must be run as Administrator.\n"), appname);
		_tprintf(_T("lpApplicationName = %s\n"), appname);
		_tprintf(_T("lpCommandLine = %s\n"), commandline);

		exitcode = 1;
	}
	else
	{
		if (pidfile != NULL)
		{
			char pidbuffer[65];
			_itoa(procinfo.dwProcessId, pidbuffer, 10);
			HANDLE hpidfile = ::CreateFile(pidfile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &secattr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hpidfile == INVALID_HANDLE_VALUE)  _tprintf(_T("PID file '%s' was unable to be opened.\n"), pidfile);
			else
			{
				::WriteFile(hpidfile, pidbuffer, (DWORD)strlen(pidbuffer), NULL, NULL);
				::CloseHandle(hpidfile);
			}
		}

		::CloseHandle(procinfo.hThread);
		if (wait)
		{
			// Wait for process to complete.
			if (verbose)
			{
				if (waitamount == INFINITE)  _tprintf(_T("Waiting for process to complete...\n"));
				else  _tprintf(_T("Waiting for process to complete (%ims)...\n"), waitamount);
			}
			if (::WaitForSingleObject(procinfo.hProcess, waitamount) == WAIT_OBJECT_0)
			{
				if (!::GetExitCodeProcess(procinfo.hProcess, &exitcode))  exitcode = 0;
			}
			else if (terminate)
			{
				if (verbose)  _tprintf(_T("Timed out.  Terminating.\n"));
				::TerminateProcess(procinfo.hProcess, 1);
				if (!::GetExitCodeProcess(procinfo.hProcess, &exitcode))  exitcode = 0;
			}
		}

		::CloseHandle(procinfo.hProcess);
	}

	delete[] commandline;

	// Let the OS clean up after this program.  It is lazy, but whatever.
	if (verbose)  _tprintf(_T("Return code = %i\n"), (int)exitcode);

	return (int)exitcode;
}
