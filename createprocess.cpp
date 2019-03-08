// A simple program whose sole job is to run custom CreateProcess() commands.
// Useful for executing programs from batch files that don't play nice (e.g. Apache)
// or working around limitations in scripting languages.
//
// (C) 2019 CubicleSoft.  All Rights Reserved.

// Implemented as a single file compilation unit.

#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#ifdef _MBCS
#undef _MBCS
#endif

#include <cstdio>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>
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

#ifdef SUBSYSTEM_WINDOWS
// If the caller is a console application and is waiting for this application to complete, then attach to the console.
void InitVerboseMode(void)
{
	if (::AttachConsole(ATTACH_PARENT_PROCESS))
	{
		if (::GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
		{
			freopen("CONOUT$", "w", stdout);
			setvbuf(stdout, NULL, _IONBF, 0);
		}

		if (::GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
		{
			freopen("CONOUT$", "w", stderr);
			setvbuf(stderr, NULL, _IONBF, 0);
		}
	}
}
#endif

void DumpSyntax(TCHAR *currfile)
{
#ifdef SUBSYSTEM_WINDOWS
	InitVerboseMode();
#endif

	_tprintf(_T("(C) 2019 CubicleSoft.  All Rights Reserved.\n\n"));

	_tprintf(_T("Syntax:  %s [options] EXEToRun [arguments]\n\n"), currfile);

	_tprintf(_T("Options:\n"));

	_tprintf(_T("\t/v\n\
\tVerbose mode.\n\
\n\
\t/w[=Milliseconds]\n\
\tWaits for the process to complete before exiting.\n\
\tThe default behavior is to return immediately.\n\
\tIf Milliseconds is specified, the number of milliseconds to wait.\n\
\tReturn code, if any, is returned to caller.\n\
\n\
\t/pid=File\n\
\tWrites the process ID to the specified file.\n\
\n\
\t/term\n\
\tUsed with /w=Milliseconds.\n\
\tTerminates the process when the wait time is up.\n\
\n\
\t/f=PriorityClass\n\
\t\tSets the priority class of the new process.\n\
\t\tThere is only one priority class per process.\n\
\t\tThe 'PriorityClass' can be one of:\n\
\t\tABOVE_NORMAL_PRIORITY_CLASS\n\
\t\tBELOW_NORMAL_PRIORITY_CLASS\n\
\t\tHIGH_PRIORITY_CLASS\n\
\t\tIDLE_PRIORITY_CLASS\n\
\t\tNORMAL_PRIORITY_CLASS\n\
\t\tREALTIME_PRIORITY_CLASS\n\
\n\
\t/f=CreateFlag\n\
\t\tSets a creation flag for the new process.\n\
\t\tMultiple /f options can be specified.\n\
\t\tEach 'CreateFlag' can be one of:\n\
\t\tCREATE_DEFAULT_ERROR_MODE\n\
\t\tCREATE_NEW_CONSOLE\n\
\t\tCREATE_NEW_PROCESS_GROUP\n\
\t\tCREATE_NO_WINDOW\n\
\t\tCREATE_PROTECTED_PROCESS\n\
\t\tCREATE_PRESERVE_CODE_AUTHZ_LEVEL\n\
\t\tCREATE_SEPARATE_WOW_VDM\n\
\t\tCREATE_SHARED_WOW_VDM\n\
\t\tDEBUG_ONLY_THIS_PROCESS\n\
\t\tDEBUG_PROCESS\n\
\t\tDETACHED_PROCESS\n\
\t\tINHERIT_PARENT_AFFINITY\n\
\n\
\t/dir=StartDir\n\
\t\tSets the starting directory of the new process.\n\
\n\
\t/desktop=Desktop\n\
\t\tSets the STARTUPINFO.lpDesktop member to target a specific desktop.\n\
\n\
\t/title=WindowTitle\n\
\t\tSets the STARTUPINFO.lpTitle member to a specific title.\n\
\n\
\t/x=XPositionInPixels\n\
\t\tSets the STARTUPINFO.dwX member to a specific x-axis position, in pixels.\n\
\n\
\t/y=YPositionInPixels\n\
\t\tSets the STARTUPINFO.dwY member to a specific y-axis position, in pixels.\n\
\n\
\t/width=WidthInPixels\n\
\t\tSets the STARTUPINFO.dwXSize member to a specific width, in pixels.\n\
\n\
\t/height=HeightInPixels\n\
\t\tSets the STARTUPINFO.dwYSize member to a specific height, in pixels.\n\
\n\
\t/xchars=BufferWidthInCharacters\n\
\t\tSets the STARTUPINFO.dwXCountChars member to buffer width, in characters.\n\
\n\
\t/ychars=BufferHeightInCharacters\n\
\t\tSets the STARTUPINFO.dwYCountChars member to buffer height, in characters.\n\
\n\
\t/f=FillAttribute\n\
\t\tSets the STARTUPINFO.dwFillAttribute member to buffer height, in characters.\n\
\t\tMultiple /f options can be specified.\n\
\t\tEach 'FillAttribute' can be one of:\n\
\t\tFOREGROUND_RED\n\
\t\tFOREGROUND_GREEN\n\
\t\tFOREGROUND_BLUE\n\
\t\tFOREGROUND_INTENSITY\n\
\t\tBACKGROUND_RED\n\
\t\tBACKGROUND_GREEN\n\
\t\tBACKGROUND_BLUE\n\
\t\tBACKGROUND_INTENSITY\n\
\n\
\t/f=StartupFlag\n\
\t\tSets the STARTUPINFO.dwFlags flag for the new process.\n\
\t\tMultiple /f options can be specified.\n\
\t\tEach 'StartupFlag' can be one of:\n\
\t\tSTARTF_FORCEONFEEDBACK\n\
\t\tSTARTF_FORCEOFFFEEDBACK\n\
\t\tSTARTF_PREVENTPINNING\n\
\t\tSTARTF_RUNFULLSCREEN\n\
\t\tSTARTF_TITLEISAPPID\n\
\t\tSTARTF_TITLEISLINKNAME\n\
\n\
\t/f=ShowWindow\n\
\t\tSets the STARTUPINFO.wShowWindow flag for the new process.\n\
\t\tThere is only one show window option per process.\n\
\t\tThe 'ShowWindow' value can be one of:\n\
\t\tSW_FORCEMINIMIZE\n\
\t\tSW_HIDE\n\
\t\tSW_MAXIMIZE\n\
\t\tSW_MINIMIZE\n\
\t\tSW_RESTORE\n\
\t\tSW_SHOW\n\
\t\tSW_SHOWDEFAULT\n\
\t\tSW_SHOWMAXIMIZED\n\
\t\tSW_SHOWMINIMIZED\n\
\t\tSW_SHOWMINNOACTIVE\n\
\t\tSW_SHOWNA\n\
\t\tSW_SHOWNOACTIVATE\n\
\t\tSW_SHOWNORMAL\n\
\n\
\t/hotkey=HotkeyValue\n\
\t\tSets the STARTUPINFO.hStdInput handle for the new process.\n\
\t\tSpecifies the wParam member of a WM_SETHOKEY message to the new process.\n\
\n\
\t/socketip=IPAddress\n\
\t\tSpecifies the IP address to connect to over TCP/IP.\n\
\n\
\t/socketport=PortNumber\n\
\t\tSpecifies the port number to connect to over TCP/IP.\n\
\n\
\t/sockettoken=Token\n\
\t\tSpecifies the token to send to each socket.\n\
\t\tLess secure than using /sockettokenlen and stdin.\n\
\n\
\t/sockettokenlen=TokenLength\n\
\t\tSpecifies the length of the token to read from stdin.\n\
\t\tWhen specified, a token must be sent for each socket.\n\
\n\
\t/stdin=FileOrEmptyOrsocket\n\
\t\tSets the STARTUPINFO.hStdInput handle for the new process.\n\
\t\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n\
\t\tWhen this option is 'socket', the /socket IP and port are used.\n\
\t\tWhen this option is not specified, the current stdin is used.\n\
\n\
\t/stdout=FileOrEmptyOrsocket\n\
\t\tSets the STARTUPINFO.hStdOutput handle for the new process.\n\
\t\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n\
\t\tWhen this option is 'socket', the /socket IP and port are used.\n\
\t\tWhen this option is not specified, the current stdout is used.\n\
\n\
\t/stderr=FileOrEmptyOrstdoutOrsocket\n\
\t\tSets the STARTUPINFO.hStdError handle for the new process.\n\
\t\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n\
\t\tWhen this option is 'stdout', the value of stdout is used.\n\
\t\tWhen this option is 'socket', the /socket IP and port are used.\n\
\t\tWhen this option is not specified, the current stderr is used.\n\n"));

#ifdef SUBSYSTEM_WINDOWS
	_tprintf(_T("\t/attach\n"));
	_tprintf(_T("\t\tAttempt to attach to a parent console if it exists.\n"));
	_tprintf(_T("\t\tAlso resets standard handles back to defaults.\n\n"));
#endif
}

bool GxNetworkStarted = false;
SOCKET ConnectSocketHandle(TCHAR *socketip, unsigned short socketport, char fd, TCHAR *token, unsigned short tokenlen)
{
	if (socketip == NULL || socketport == 0 || tokenlen > 512)  return INVALID_SOCKET;

	// Initialize Winsock.
	if (!GxNetworkStarted)
	{
		WSADATA WSAData;
		if (::WSAStartup(MAKEWORD(2, 2), &WSAData))  return INVALID_SOCKET;
		if (LOBYTE(WSAData.wVersion) != 2 || HIBYTE(WSAData.wVersion) != 2)  return INVALID_SOCKET;

		GxNetworkStarted = true;
	}

	// Determine IPv4 or IPv6.
	SOCKET s;
	if (_tcschr(socketip, _T(':')) != NULL)
	{
		struct sockaddr_in6 si = {0};

		si.sin6_family = AF_INET6;
		si.sin6_port = ::htons(socketport);
		if (::InetPton(AF_INET6, socketip, &si.sin6_addr) != 1)  return INVALID_SOCKET;

		s = ::WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
		if (s == INVALID_SOCKET)  return INVALID_SOCKET;

		if (::connect(s, (sockaddr *)&si, sizeof(si)) != 0)
		{
			::closesocket(s);

			return INVALID_SOCKET;
		}
	}
	else
	{
		struct sockaddr_in si = {0};

		si.sin_family = AF_INET;
		si.sin_port = ::htons(socketport);
		if (::InetPton(AF_INET, socketip, &si.sin_addr) != 1)  return INVALID_SOCKET;

		s = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
		if (s == INVALID_SOCKET)  return INVALID_SOCKET;

		if (::connect(s, (sockaddr *)&si, sizeof(si)) != 0)
		{
			::closesocket(s);

			return INVALID_SOCKET;
		}
	}

	// Do authentication first.  Send token to the target.
	if (tokenlen)
	{
		char tokenbuf[512];
		DWORD bytesread;

		// There's no particularly good way to handle errors here without completely rewriting the function.
		if (!::ReadFile(::GetStdHandle(STD_INPUT_HANDLE), tokenbuf, tokenlen, &bytesread, NULL) || (unsigned short)bytesread != tokenlen || ::send(s, tokenbuf, (int)tokenlen, 0) != tokenlen)
		{
			::closesocket(s);

			return INVALID_SOCKET;
		}
	}
	else if (token != NULL)
	{
		char tokenbuf[512];

		for (size_t x = 0; x < 512 && token[x]; x++)
		{
			if (token[x] <= 0xFF)  tokenbuf[tokenlen++] = (char)token[x];
		}

		if (!tokenlen || ::send(s, tokenbuf, (int)tokenlen, 0) != tokenlen)
		{
			::closesocket(s);

			return INVALID_SOCKET;
		}
	}

	// Send one byte of data to the target so that it knows which file descriptor this socket is associated with.
	if (::send(s, &fd, 1, 0) != 1)
	{
		::closesocket(s);

		return INVALID_SOCKET;
	}

	return s;
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
	TCHAR *socketip = NULL;
	unsigned short socketport = 0;
	TCHAR *sockettoken = NULL;
	unsigned short sockettokenlen = 0;
	SOCKET stdinsocket = INVALID_SOCKET;
	SOCKET stdoutsocket = INVALID_SOCKET;
	SOCKET stderrsocket = INVALID_SOCKET;
	PROCESS_INFORMATION procinfo = {0};
	DWORD exitcode = 0;

	if (argc < 2)
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
		else if (!_tcsicmp(argv[x], _T("/f=SW_HIDE")))
		{
			startinfo.dwFlags |= STARTF_USESHOWWINDOW;
			startinfo.wShowWindow = SW_HIDE;
		}
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
			startinfo.dwFlags &= ~STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/socketip="), 10))  socketip = argv[x] + 10;
		else if (!_tcsncicmp(argv[x], _T("/socketport="), 12))  socketport = (unsigned short)_tstoi(argv[x] + 12);
		else if (!_tcsncicmp(argv[x], _T("/sockettoken="), 13))  sockettoken = argv[x] + 13;
		else if (!_tcsncicmp(argv[x], _T("/sockettokenlen="), 16))  sockettokenlen = (unsigned short)_tstoi(argv[x] + 16);
		else if (!_tcsncicmp(argv[x], _T("/stdin="), 7))
		{
			if (argv[x][7] == _T('\0'))
			{
				startinfo.hStdInput = NULL;
				stdinstr = _T(":null");
			}
			else if (!_tcsicmp(argv[x] + 7, _T("socket")))
			{
				stdinsocket = ConnectSocketHandle(socketip, socketport, '\x00', sockettoken, sockettokenlen);

				if (stdinsocket != INVALID_SOCKET)
				{
					startinfo.hStdInput = (HANDLE)stdinsocket;
					stdinstr = _T(":socket");
				}
			}
			else
			{
				startinfo.hStdInput = ::CreateFile(argv[x] + 7, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &secattr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				stdinstr = argv[x] + 7;
			}

			startinfo.dwFlags &= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/stdout="), 8))
		{
			if (argv[x][8] == _T('\0'))
			{
				startinfo.hStdOutput = NULL;
				stdoutstr = _T(":null");
			}
			else if (!_tcsicmp(argv[x] + 8, _T("socket")))
			{
				stdoutsocket = ConnectSocketHandle(socketip, socketport, '\x01', sockettoken, sockettokenlen);

				if (stdoutsocket != INVALID_SOCKET)
				{
					startinfo.hStdOutput = (HANDLE)stdoutsocket;
					stdoutstr = _T(":socket");
				}
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

			startinfo.dwFlags &= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/stderr="), 8))
		{
			if (argv[x][8] == _T('\0'))
			{
				startinfo.hStdError = NULL;
				stderrstr = _T(":null");
			}
			else if (!_tcsicmp(argv[x] + 8, _T("socket")))
			{
				stderrsocket = ConnectSocketHandle(socketip, socketport, '\x02', sockettoken, sockettokenlen);

				if (stderrsocket != INVALID_SOCKET)
				{
					startinfo.hStdError = (HANDLE)stderrsocket;
					stderrstr = _T(":socket");
				}
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

			startinfo.dwFlags &= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else if (!_tcsicmp(argv[x], _T("/attach")))
		{
#ifdef SUBSYSTEM_WINDOWS
			// For the Windows subsystem only, attempt to attach to a parent console if it exists.
			InitVerboseMode();
#endif

			// Reset handles.
			startinfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
			startinfo.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
			startinfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);

			startinfo.dwFlags &= ~STARTF_USEHOTKEY;
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
#ifdef SUBSYSTEM_WINDOWS
		InitVerboseMode();
#endif

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
	if (startinfo.wShowWindow)  startinfo.dwFlags |= STARTF_USESHOWWINDOW;
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
#ifdef SUBSYSTEM_WINDOWS
		InitVerboseMode();
#endif

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
		if (!(startinfo.dwFlags & STARTF_USESHOWWINDOW))  _tprintf(_T("(Unused)"));
		else if (startinfo.wShowWindow == SW_FORCEMINIMIZE)  _tprintf(_T("SW_FORCEMINIMIZE"));
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

#ifdef SUBSYSTEM_WINDOWS
		InitVerboseMode();
#endif

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
			if (hpidfile == INVALID_HANDLE_VALUE)
			{
#ifdef SUBSYSTEM_WINDOWS
				InitVerboseMode();
#endif

				_tprintf(_T("PID file '%s' was unable to be opened.\n"), pidfile);
			}
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

	if (GxNetworkStarted)
	{
		if (stdinsocket != INVALID_SOCKET)  ::closesocket(stdinsocket);
		if (stdoutsocket != INVALID_SOCKET)  ::closesocket(stdoutsocket);
		if (stderrsocket != INVALID_SOCKET)  ::closesocket(stderrsocket);

		::WSACleanup();
	}

	// Let the OS clean up after this program.  It is lazy, but whatever.
	if (verbose)  _tprintf(_T("Return code = %i\n"), (int)exitcode);

	return (int)exitcode;
}

#ifdef SUBSYSTEM_WINDOWS
#ifndef UNICODE
// Swiped from:  https://stackoverflow.com/questions/291424/canonical-way-to-parse-the-command-line-into-arguments-in-plain-c-windows-api
LPSTR* CommandLineToArgvA(LPSTR lpCmdLine, INT *pNumArgs)
{
	int retval;
	retval = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);
	if (!SUCCEEDED(retval))  return NULL;

	LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));
	if (lpWideCharStr == NULL)  return NULL;

	retval = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);
	if (!SUCCEEDED(retval))
	{
		free(lpWideCharStr);

		return NULL;
	}

	int numArgs;
	LPWSTR* args;
	args = ::CommandLineToArgvW(lpWideCharStr, &numArgs);
	free(lpWideCharStr);
	if (args == NULL)  return NULL;

	int storage = numArgs * sizeof(LPSTR);
	for (int i = 0; i < numArgs; i++)
	{
		BOOL lpUsedDefaultChar = FALSE;
		retval = ::WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);
		if (!SUCCEEDED(retval))
		{
			::LocalFree(args);

			return NULL;
		}

		storage += retval;
	}

	LPSTR* result = (LPSTR *)::LocalAlloc(LMEM_FIXED, storage);
	if (result == NULL)
	{
		LocalFree(args);

		return NULL;
	}

	int bufLen = storage - numArgs * sizeof(LPSTR);
	LPSTR buffer = ((LPSTR)result) + numArgs * sizeof(LPSTR);
	for (int i = 0; i < numArgs; ++ i)
	{
		BOOL lpUsedDefaultChar = FALSE;
		retval = ::WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);
		if (!SUCCEEDED(retval))
		{
			::LocalFree(result);
			::LocalFree(args);

			return NULL;
		}

		result[i] = buffer;
		buffer += retval;
		bufLen -= retval;
	}

	::LocalFree(args);

	*pNumArgs = numArgs;
	return result;
}
#endif

int CALLBACK WinMain(HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */, LPSTR lpCmdLine, int /* nCmdShow */)
{
	int argc;
	TCHAR **argv;
	int result;

#ifdef UNICODE
	argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
#else
	argv = CommandLineToArgvA(lpCmdLine, &argc);
#endif

	if (argv == NULL)  return 0;

	result = _tmain(argc, argv);

	::LocalFree(argv);

	return result;
}
#endif
