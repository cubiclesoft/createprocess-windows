// A simple program whose sole job is to run custom CreateProcess() commands.
// Useful for executing programs from batch files that don't play nice (e.g. Apache)
// or working around limitations in scripting languages.
//
// (C) 2021 CubicleSoft.  All Rights Reserved.

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
#include <tlhelp32.h>
#include <winternl.h>
#include <lm.h>
#include <sddl.h>
//#include <ntsecapi.h>
//#include <wincred.h>
#include <userenv.h>
#include <tchar.h>

#include "templates/static_wc_mixed_var.h"
#include "templates/static_mixed_var.h"
#include "templates/shared_lib.h"
#include "templates/packed_ordered_hash.h"

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

	_tprintf(_T("(C) 2021 CubicleSoft.  All Rights Reserved.\n\n"));

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
\t/runelevated\n\
\tCalls CreateProcess() as a high Integrity Level elevated process.\n\
\t/w should be specified before this option.\n\
\tMay trigger elevation.  Not compatible with /term when not elevated.\n\
\n\
\t/elevatedtoken\n\
\tUses an elevated token to create a child process.\n\
\tMay create a temporary SYSTEM service to copy the primary token via\n\
\tundocumented Windows kernel APIs.\n\
\n\
\t/systemtoken\n\
\tUses a SYSTEM token to create a child process.\n\
\tMay create a temporary SYSTEM service to copy the primary token via\n\
\tundocumented Windows kernel APIs.\n\
\n\
\t/usetoken=PIDorSIDsAndPrivileges\n\
\tUses the primary token of the specified process ID,\n\
\tor a process matching specific comma-separated user/group SIDs\n\
\tand/or a process with specific privileges.\n\
\tMay trigger elevation.  See /elevatedtoken.\n\
\n\
\t/createtoken=Parameters\n\
\tCreates a primary token from scratch.\n\
\tMay trigger elevation.  See /elevatedtoken.\n\
\tUses an undocumented Windows kernel API.\n\
\tThe 'Parameters' are semicolon separated:\n\
\t\tUserSID;\n\
\t\tGroupSID:Attr,GroupSID:Attr,...;\n\
\t\tPrivilege:Attr,Privilege:Attr,...;\n\
\t\tOwnerSID;\n\
\t\tPrimaryGroupSID;\n\
\t\tDefaultDACL;\n\
\t\tSourceInHex:SourceLUID\n\
\n\
\t/mergeenv\n\
\tMerges the current environment with another user environment.\n\
\tUse with /elevatedtoken, /systemtoken, /usetoken, /createtoken.\n\
\n\
\t/mutex=MutexName\n\
\tCreates a mutex with the specified name.\n\
\tUse the named mutex with /singleton or other software\n\
\tto detect an already running instance.\n\
\n\
\t/singleton[=Milliseconds]\n\
\tOnly starts the target process if named /mutex is the only instance.\n\
\tIf Milliseconds is specified, the number of milliseconds to wait.\n\
\n\
\t/semaphore=MaxCount,SemaphoreName\n\
\tCreates a semaphore with the specified name and limit/count.\n\
\tUse the named semaphore with /multiton\n\
\tto limit the number of running processes.\n\
\n\
\t/multiton[=Milliseconds]\n\
\tChecks or waits for a named /semaphore.\n\
\tIf Milliseconds is specified, the number of milliseconds to wait.\n\
\n\
\t/f=PriorityClass\n\
\tSets the priority class of the new process.\n\
\tThere is only one priority class per process.\n\
\tThe 'PriorityClass' can be one of:\n\
\t\tABOVE_NORMAL_PRIORITY_CLASS\n\
\t\tBELOW_NORMAL_PRIORITY_CLASS\n\
\t\tHIGH_PRIORITY_CLASS\n\
\t\tIDLE_PRIORITY_CLASS\n\
\t\tNORMAL_PRIORITY_CLASS\n\
\t\tREALTIME_PRIORITY_CLASS\n\
\n\
\t/f=CreateFlag\n\
\tSets a creation flag for the new process.\n\
\tMultiple /f options can be specified.\n\
\tEach 'CreateFlag' can be one of:\n\
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
\tSets the starting directory of the new process.\n\
\n\
\t/desktop=Desktop\n\
\tSets the STARTUPINFO.lpDesktop member to target a specific desktop.\n\
\n\
\t/title=WindowTitle\n\
\tSets the STARTUPINFO.lpTitle member to a specific title.\n\
\n\
\t/x=XPositionInPixels\n\
\tSets the STARTUPINFO.dwX member to a specific x-axis position, in pixels.\n\
\n\
\t/y=YPositionInPixels\n\
\tSets the STARTUPINFO.dwY member to a specific y-axis position, in pixels.\n\
\n\
\t/width=WidthInPixels\n\
\tSets the STARTUPINFO.dwXSize member to a specific width, in pixels.\n\
\n\
\t/height=HeightInPixels\n\
\tSets the STARTUPINFO.dwYSize member to a specific height, in pixels.\n\
\n\
\t/xchars=BufferWidthInCharacters\n\
\tSets the STARTUPINFO.dwXCountChars member to buffer width, in characters.\n\
\n\
\t/ychars=BufferHeightInCharacters\n\
\tSets the STARTUPINFO.dwYCountChars member to buffer height, in characters.\n\
\n\
\t/f=FillAttribute\n\
\tSets the STARTUPINFO.dwFillAttribute member text and background colors.\n\
\tMultiple /f options can be specified.\n\
\tEach 'FillAttribute' can be one of:\n\
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
\tSets the STARTUPINFO.dwFlags flag for the new process.\n\
\tMultiple /f options can be specified.\n\
\tEach 'StartupFlag' can be one of:\n\
\t\tSTARTF_FORCEONFEEDBACK\n\
\t\tSTARTF_FORCEOFFFEEDBACK\n\
\t\tSTARTF_PREVENTPINNING\n\
\t\tSTARTF_RUNFULLSCREEN\n\
\t\tSTARTF_TITLEISAPPID\n\
\t\tSTARTF_TITLEISLINKNAME\n\
\n\
\t/f=ShowWindow\n\
\tSets the STARTUPINFO.wShowWindow flag for the new process.\n\
\tThere is only one show window option per process.\n\
\tThe 'ShowWindow' value can be one of:\n\
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
\tSets the STARTUPINFO.hStdInput handle for the new process.\n\
\tSpecifies the wParam member of a WM_SETHOKEY message to the new process.\n\
\n\
\t/socketip=IPAddress\n\
\tSpecifies the IP address to connect to over TCP/IP.\n\
\n\
\t/socketport=PortNumber\n\
\tSpecifies the port number to connect to over TCP/IP.\n\
\n\
\t/sockettoken=Token\n\
\tSpecifies the token to send to each socket.\n\
\tLess secure than using /sockettokenlen and stdin.\n\
\n\
\t/sockettokenlen=TokenLength\n\
\tSpecifies the length of the token to read from stdin.\n\
\tWhen specified, a token must be sent for each socket.\n\
\n\
\t/stdin=FileOrEmptyOrsocket\n\
\tSets the STARTUPINFO.hStdInput handle for the new process.\n\
\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n\
\tWhen this option is 'socket', the /socket IP and port are used.\n\
\tWhen this option is not specified, the current stdin is used.\n\
\n\
\t/stdout=FileOrEmptyOrsocket\n\
\tSets the STARTUPINFO.hStdOutput handle for the new process.\n\
\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n\
\tWhen this option is 'socket', the /socket IP and port are used.\n\
\tWhen this option is not specified, the current stdout is used.\n\
\n\
\t/stderr=FileOrEmptyOrstdoutOrsocket\n\
\tSets the STARTUPINFO.hStdError handle for the new process.\n\
\tWhen this option is empty, INVALID_HANDLE_VALUE is used.\n\
\tWhen this option is 'stdout', the value of stdout is used.\n\
\tWhen this option is 'socket', the /socket IP and port are used.\n\
\tWhen this option is not specified, the current stderr is used.\n\
\n\
\t/attach[=ProcessID]\n\
\tAttempt to attach to a parent OR a specific process' console.\n\
\tAlso resets standard handles back to defaults.\n\n"));
}

bool SetThreadProcessPrivilege(LPCWSTR PrivilegeName, bool Enable)
{
	HANDLE Token;
	TOKEN_PRIVILEGES TokenPrivs;
	LUID TempLuid;
	bool Result;

	if (!::LookupPrivilegeValueW(NULL, PrivilegeName, &TempLuid))  return false;

	if (!::OpenThreadToken(::GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &Token))
	{
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token))  return false;
	}

	TokenPrivs.PrivilegeCount = 1;
	TokenPrivs.Privileges[0].Luid = TempLuid;
	TokenPrivs.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);

	Result = (::AdjustTokenPrivileges(Token, FALSE, &TokenPrivs, 0, NULL, NULL) && ::GetLastError() == ERROR_SUCCESS);

	::CloseHandle(Token);

	return Result;
}

DWORD GxConnectPipePID = 0;
HANDLE GxMainCommPipeClient = INVALID_HANDLE_VALUE;

// Creates an event object to be able to timeout on for the named pipe.
HANDLE CreateCommPipeConnectEventObj(DWORD pid)
{
	CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;

	TempVar.SetStr(L"Global\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_CommEv_");
	TempVar.AppendUInt(pid);

	return ::CreateEventW(NULL, FALSE, FALSE, TempVar.GetStr());
}

// Attempt to connect to the named pipe.  Does not emit errors.
bool ConnectToMainCommPipe()
{
	if (GxMainCommPipeClient != INVALID_HANDLE_VALUE)  return true;

	if (!GxConnectPipePID)  return false;

	CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;
	HANDLE tempevent;

	tempevent = CreateCommPipeConnectEventObj(GxConnectPipePID);
	if (tempevent == NULL)  return false;

	TempVar.SetStr(L"\\\\.\\pipe\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_Comm_");
	TempVar.AppendUInt(GxConnectPipePID);

	do
	{
		// Notify the pipe server that a connection is incoming.
		if (!::SetEvent(tempevent))  break;

		// Attempt to connect.
		GxMainCommPipeClient = ::CreateFile(TempVar.GetStr(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (GxMainCommPipeClient != INVALID_HANDLE_VALUE)  return true;

		if (::GetLastError() != ERROR_PIPE_BUSY)  break;

		if (!::WaitNamedPipe(TempVar.GetStr(), 10000))  break;
	} while (1);

	::CloseHandle(tempevent);

	return false;
}

void DisconnectMainCommPipe()
{
	if (GxMainCommPipeClient == INVALID_HANDLE_VALUE)  return;

	::CloseHandle(GxMainCommPipeClient);

	GxMainCommPipeClient = INVALID_HANDLE_VALUE;
	GxConnectPipePID = 0;
}

HANDLE StartMainCommPipeServer()
{
	CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;

	TempVar.SetStr(L"\\\\.\\pipe\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_Comm_");
	TempVar.AppendUInt(::GetCurrentProcessId());

	return ::CreateNamedPipe(TempVar.GetStr(), PIPE_ACCESS_DUPLEX, 0, 1, 4096, 4096, 0, NULL);
}

void DumpErrorMsg(const char *errorstr, const char *errorcode, DWORD winerror)
{
#ifdef SUBSYSTEM_WINDOWS
	InitVerboseMode();
#endif

	CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
	CubicleSoft::StaticMixedVar<char[8192]> TempVar2;

	printf("%s (%s)\n", errorstr, errorcode);

	TempVar2.SetFormattedStr("[%lu] %s (%s)", ::GetCurrentProcessId(), errorstr, errorcode);

	if (winerror)
	{
		LPSTR errmsg = NULL;

		::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, winerror, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errmsg, 0, NULL);
		if (errmsg == NULL)
		{
			printf("%d - Unknown error\n", winerror);
			TempVar2.AppendFormattedStr("  (%lu - Unknown error)", winerror);
		}
		else
		{
			size_t y = strlen(errmsg);
			while (y && (errmsg[y - 1] == '\r' || errmsg[y - 1] == '\n'))  errmsg[--y] = '\0';

			printf("%d - %s\n", winerror, errmsg);
			TempVar2.AppendFormattedStr("  (%lu - %s)", winerror, errmsg);
			::LocalFree(errmsg);
		}
	}

	TempVar2.AppendStr("\n");

	// Send the error message to the main process' communication pipe.
	if (ConnectToMainCommPipe())
	{
		TempVar.SetStr(TempVar2.GetStr());

		DWORD tempsize = TempVar.GetSize() * sizeof(WCHAR);

		::WriteFile(GxMainCommPipeClient, "\x00", 1, NULL, NULL);
		::WriteFile(GxMainCommPipeClient, &tempsize, 4, NULL, NULL);
		::WriteFile(GxMainCommPipeClient, TempVar.GetStr(), tempsize, NULL, NULL);

		DisconnectMainCommPipe();
	}

	::Sleep(5000);
}

void DumpWideErrorMsg(const WCHAR *errorstr, const char *errorcode, DWORD winerror)
{
	char *errorstr2;
	BOOL useddefaultchar = FALSE;
	int retval = ::WideCharToMultiByte(CP_ACP, 0, errorstr, -1, NULL, 0, NULL, &useddefaultchar);
	if (!retval)
	{
		DumpErrorMsg("Unable to convert error message to multibyte.", "wide_char_to_multibyte_failed", ::GetLastError());

		return;
	}

	DWORD tempsize = (DWORD)retval;
	errorstr2 = (char *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tempsize);
	if (errorstr2 == NULL)
	{
		DumpErrorMsg("Error message buffer allocation failed.", "heap_alloc_failed", ::GetLastError());

		return;
	}

	useddefaultchar = FALSE;
	retval = ::WideCharToMultiByte(CP_ACP, 0, errorstr, -1, errorstr2, tempsize, NULL, &useddefaultchar);
	if (!retval)
	{
		DumpErrorMsg("Unable to convert error message to multibyte.", "wide_char_to_multibyte_failed", ::GetLastError());

		::HeapFree(::GetProcessHeap(), 0, errorstr2);

		return;
	}

	size_t y = strlen(errorstr2);
	while (y && (errorstr2[y - 1] == '\r' || errorstr2[y - 1] == '\n'))  errorstr2[--y] = '\0';

	// Display the error message.
	DumpErrorMsg(errorstr2, errorcode, winerror);

	::HeapFree(::GetProcessHeap(), 0, errorstr2);
}

// Waits for a client to connect and then validates the session the client is running in.
bool WaitForValidCommPipeClient(HANDLE &pipehandle, HANDLE eventhandle, DWORD timeout)
{
	while (1)
	{
		// Wait for the event object to fire (allows for timeout).
		if (::WaitForSingleObject(eventhandle, timeout) != WAIT_OBJECT_0)
		{
			DumpErrorMsg("An elevated process did not respond before the timeout expired.", "timeout", ::GetLastError());

			return false;
		}

		// Wait for the client to connect.
		if (!::ConnectNamedPipe(pipehandle, NULL) && ::GetLastError() != ERROR_PIPE_CONNECTED)
		{
			DumpErrorMsg("Main communication pipe failed.", "connect_named_pipe_failed", ::GetLastError());

			return false;
		}

		// Allow connections from session 0 (SYSTEM) and the current process' session.
		ULONG clientsession;
		DWORD currsessionid;
		if (::GetNamedPipeClientSessionId(pipehandle, &clientsession) && ::ProcessIdToSessionId(::GetCurrentProcessId(), &currsessionid) && (clientsession == 0 || clientsession == currsessionid))
		{
			return true;
		}

		// Restart the named pipe server.
		::CloseHandle(pipehandle);

		pipehandle = StartMainCommPipeServer();
		if (pipehandle == INVALID_HANDLE_VALUE)
		{
			DumpErrorMsg("Unable to create the main communication pipe.", "start_main_comm_pipe_failed", ::GetLastError());

			return false;
		}
	}
}

BOOL ReadFileExact(HANDLE filehandle, LPVOID buffer, DWORD numbytes)
{
	BOOL result;
	DWORD bytesread;

	do
	{
		result = ::ReadFile(filehandle, buffer, numbytes, &bytesread, NULL);
		if (!result)  return result;

		buffer = (char *)buffer + bytesread;
		numbytes -= bytesread;
	} while (numbytes);

	return result;
}

// Waits until a single status byte is received.
bool WaitForCommPipeStatus(HANDLE pipehandle)
{
	char status;

	if (!ReadFileExact(pipehandle, &status, 1))
	{
		DumpErrorMsg("The main communication pipe broke.", "read_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	if (status)  return true;

	// Read the error message.
	DWORD tempsize;
	WCHAR *msgbuffer;

	if (!ReadFileExact(pipehandle, &tempsize, 4))
	{
		DumpErrorMsg("Unable to read size from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	if (tempsize < 1 * sizeof(WCHAR) || tempsize % sizeof(WCHAR) != 0)
	{
		DumpErrorMsg("The read size from the main communication pipe is invalid.", "read_main_comm_pipe_invalid_size", ::GetLastError());

		return false;
	}

	msgbuffer = (WCHAR *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tempsize);
	if (msgbuffer == NULL)
	{
		DumpErrorMsg("Error message buffer allocation failed.", "heap_alloc_failed", ::GetLastError());

		return false;
	}

	if (!ReadFileExact(pipehandle, msgbuffer, tempsize))
	{
		DumpErrorMsg("Unable to read error message from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	tempsize /= sizeof(WCHAR);

	msgbuffer[tempsize - 1] = L'\0';

	// Convert to DumpErrorMsg() format.
	DumpWideErrorMsg(msgbuffer, "main_comm_pipe_client_sent_error", 0);

	::HeapFree(::GetProcessHeap(), 0, msgbuffer);

	return false;
}

bool ReadCommPipeString(HANDLE commpipehandle, WCHAR *&buffer, DWORD &tempsize, size_t minsize)
{
	if (commpipehandle == INVALID_HANDLE_VALUE)  return false;

	if (!ReadFileExact(commpipehandle, &tempsize, 4))
	{
		DumpErrorMsg("Unable to read size from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	if (tempsize < minsize * sizeof(WCHAR) || tempsize % sizeof(WCHAR) != 0)
	{
		DumpErrorMsg("The read size from the main communication pipe is invalid.", "read_main_comm_pipe_invalid_size", 0);

		return false;
	}

	buffer = (WCHAR *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tempsize);
	if (buffer == NULL)
	{
		DumpErrorMsg("Allocation failed.", "heap_alloc_failed", ::GetLastError());

		return false;
	}

	if (!ReadFileExact(commpipehandle, buffer, tempsize))
	{
		DumpErrorMsg("Unable to read buffer from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	tempsize /= sizeof(WCHAR);

	buffer[tempsize - 1] = L'\0';

	return true;
}

void FreeCommPipeString(WCHAR *&buffer)
{
	if (buffer != NULL)  ::HeapFree(::GetProcessHeap(), 0, (LPVOID)buffer);
}

bool MakeStringArrayFromString(WCHAR *buffer, DWORD buffersize, WCHAR **&arr, int &numitems)
{
	if (buffersize < 2)
	{
		DumpErrorMsg("Insufficient buffer size.", "invalid_buffer_size", ::GetLastError());

		return false;
	}

	buffer[buffersize - 2] = L'\0';
	buffer[buffersize - 1] = L'\0';

	// Parse the buffer into an array of zero-terminated WCHAR strings.
	int x, x2;
	numitems = 0;
	for (x = 0; x < (int)buffersize - 1; x++)
	{
		if (!buffer[x])  numitems++;
	}

	arr = (WCHAR **)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, (numitems + 1) * sizeof(WCHAR *));
	if (arr == NULL)
	{
		DumpErrorMsg("Pointer buffer allocation failed.", "heap_alloc_failed", ::GetLastError());

		return false;
	}

	arr[0] = buffer;
	x2 = 1;
	for (x = 0; x < (int)buffersize - 2; x++)
	{
		if (!buffer[x])  arr[x2++] = buffer + x + 1;
	}

	return true;
}

bool ReadCommPipeStringArray(HANDLE commpipehandle, WCHAR *&buffer, WCHAR **&arr, int &numitems)
{
	DWORD tempsize;

	if (!ReadCommPipeString(commpipehandle, buffer, tempsize, 2))  return false;

	return MakeStringArrayFromString(buffer, tempsize, arr, numitems);
}

void FreeCommPipeStringArray(WCHAR *&buffer, WCHAR **&arr)
{
	if (buffer != NULL)  ::HeapFree(::GetProcessHeap(), 0, (LPVOID)buffer);
	if (arr != NULL)  ::HeapFree(::GetProcessHeap(), 0, (LPVOID)arr);
}

inline void FreeTokenInformation(LPVOID tinfo)
{
	if (tinfo != NULL)  ::HeapFree(::GetProcessHeap(), 0, tinfo);
}

LPVOID AllocateAndGetTokenInformation(HANDLE token, TOKEN_INFORMATION_CLASS infoclass, DWORD sizehint)
{
	LPVOID tinfo;
	DWORD tinfosize = sizehint;
	bool success;

	tinfo = (LPVOID)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tinfosize);
	if (tinfo == NULL)  tinfosize = 0;

	// Resize until the buffer is big enough.
	while (!(success = ::GetTokenInformation(token, infoclass, tinfo, tinfosize, &tinfosize)) && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		if (tinfo != NULL)  FreeTokenInformation(tinfo);

		tinfo = (LPVOID)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tinfosize);
		if (tinfo == NULL)
		{
			tinfosize = 0;

			break;
		}
	}

	return tinfo;
}

// Get/Duplicate the primary token of the specified process ID as a primary or impersonation token.
// duptype is ignored if accessmode does not specify TOKEN_DUPLICATE.
HANDLE GetTokenFromPID(DWORD pid, TOKEN_TYPE duptype, DWORD accessmode = TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY)
{
	HANDLE tempproc;
	HANDLE tokenhandle, tokenhandle2;

	// Enable SeDebugPrivilege.
	SetThreadProcessPrivilege(L"SeDebugPrivilege", true);

	// Open a handle to the process.
	tempproc = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (tempproc == NULL)  tempproc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

	if (tempproc == NULL)
	{
		DumpErrorMsg("Unable to open a handle to the specified process.", "open_process_failed", ::GetLastError());

		return INVALID_HANDLE_VALUE;
	}

	if (!::OpenProcessToken(tempproc, accessmode, &tokenhandle) && (!(accessmode & TOKEN_QUERY_SOURCE) || !::OpenProcessToken(tempproc, accessmode & ~TOKEN_QUERY_SOURCE, &tokenhandle)))
	{
		DumpErrorMsg("Unable to open a handle to the specified process token.", "open_process_token_failed", ::GetLastError());

		::CloseHandle(tempproc);

		return INVALID_HANDLE_VALUE;
	}

	::CloseHandle(tempproc);

	if (!(accessmode & TOKEN_DUPLICATE))  tokenhandle2 = tokenhandle;
	else
	{
		SECURITY_ATTRIBUTES secattr = {0};
		secattr.nLength = sizeof(secattr);
		secattr.bInheritHandle = FALSE;
		secattr.lpSecurityDescriptor = NULL;

		if (!::DuplicateTokenEx(tokenhandle, MAXIMUM_ALLOWED, &secattr, SecurityImpersonation, duptype, &tokenhandle2))
		{
			DumpErrorMsg("Unable to duplicate the specified process token.", "duplicate_token_ex_failed", ::GetLastError());

			::CloseHandle(tokenhandle);

			return INVALID_HANDLE_VALUE;
		}

		::CloseHandle(tokenhandle);
	}

	return tokenhandle2;
}

void GetNumSIDsAndLUIDs(LPWSTR tokenopts, size_t &numsids, size_t &numluids)
{
	size_t x = 0;

	numsids = 0;
	numluids = 0;
	while (tokenopts[x] && tokenopts[x] != L';')
	{
		for (; tokenopts[x] && tokenopts[x] != L';' && tokenopts[x] != L'S'; x++);

		if (tokenopts[x] == L'S')
		{
			if (tokenopts[x + 1] == L'-')  numsids++;
			else if (tokenopts[x + 1] == L'e')  numluids++;
		}

		for (; tokenopts[x] && tokenopts[x] != L';' && tokenopts[x] != L','; x++);

		if (tokenopts[x] == L',')  x++;
	}
}

bool GetNextTokenOptsSID(LPWSTR tokenopts, size_t &x, PSID &sidbuffer)
{
	size_t x2;
	WCHAR tempchr;

	for (x2 = x; tokenopts[x2] && tokenopts[x2] != L';' && tokenopts[x2] != L':' && tokenopts[x2] != L','; x2++);
	tempchr = tokenopts[x2];
	tokenopts[x2] = L'\0';

	bool result = ::ConvertStringSidToSidW(tokenopts + x, &sidbuffer);
	if (!result)
	{
		DWORD winerror = ::GetLastError();
		CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;

		TempVar.SetFormattedStr(L"The specified SID '%ls' in the token options is invalid.", tokenopts + x);

		DumpWideErrorMsg(TempVar.GetStr(), "invalid_sid", winerror);
	}

	tokenopts[x2] = tempchr;
	x = x2;

	return result;
}

bool GetNextTokenOptsLUID(LPWSTR tokenopts, size_t &x, LUID &luidbuffer)
{
	size_t x2;
	WCHAR tempchr;

	for (x2 = x; tokenopts[x2] && tokenopts[x2] != L';' && tokenopts[x2] != L':' && tokenopts[x2] != L','; x2++);
	tempchr = tokenopts[x2];
	tokenopts[x2] = L'\0';

	bool result = ::LookupPrivilegeValueW(NULL, tokenopts + x, &luidbuffer);
	if (!result)
	{
		DWORD winerror = ::GetLastError();
		CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;

		TempVar.SetFormattedStr(L"The specified privilege '%ls' in the token options is invalid.", tokenopts + x);

		DumpWideErrorMsg(TempVar.GetStr(), "invalid_privilege", winerror);
	}

	tokenopts[x2] = tempchr;
	x = x2;

	return result;
}

DWORD GetNextTokenOptsAttrs(LPWSTR tokenopts, size_t &x)
{
	size_t x2;
	WCHAR tempchr;

	if (tokenopts[x] != L':')  return 0;
	x++;

	for (x2 = x; tokenopts[x2] && tokenopts[x2] != L';' && tokenopts[x2] != L','; x2++);
	tempchr = tokenopts[x2];
	tokenopts[x2] = L'\0';

	DWORD result = (DWORD)_wtoi(tokenopts + x);

	tokenopts[x2] = tempchr;
	x = x2;

	return result;
}

// Attempts to locate an existing token that matches the input option string.
// duptype is ignored if accessmode does not specify TOKEN_DUPLICATE.
// Example:  FindExistingTokenFromOpts(L"S-1-16-16384,SeDebugPrivilege,SeAssignPrimaryTokenPrivilege", true)
HANDLE FindExistingTokenFromOpts(LPWSTR tokenopts, TOKEN_TYPE duptype, DWORD accessmode = TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY)
{
	if (tokenopts[0] != L'S')  return GetTokenFromPID(_wtoi(tokenopts), duptype, accessmode);

	// Split and convert the options into SIDs and privilege LUIDs.
	CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
	size_t x, x2, numsids, numluids;
	PSID *sids = NULL;
	LUID *luids = NULL;

	TempVar.SetStr(tokenopts);
	WCHAR *tokenopts2 = TempVar.GetStr();

	GetNumSIDsAndLUIDs(tokenopts2, numsids, numluids);

	if (numsids)  sids = (PSID *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, numsids * sizeof(PSID));
	if (numluids)  luids = (LUID *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, numluids * sizeof(LUID));

	bool valid = true;
	x = 0;
	numsids = 0;
	numluids = 0;
	while (tokenopts2[x])
	{
		for (; tokenopts2[x] && tokenopts2[x] != L'S'; x++);

		if (tokenopts2[x] == L'S')
		{
			if (tokenopts2[x + 1] == L'-')
			{
				valid = GetNextTokenOptsSID(tokenopts2, x, sids[numsids]);
				if (!valid)  break;

				numsids++;
			}
			else if (tokenopts2[x + 1] == L'e')
			{
				valid = GetNextTokenOptsLUID(tokenopts2, x, luids[numluids]);
				if (!valid)  break;

				numluids++;
			}
		}

		for (; tokenopts2[x] && tokenopts2[x] != L','; x++);

		if (tokenopts2[x] == L',')  x++;
	}

	// Find a process that has matching SIDs and LUIDs.
	HANDLE result = INVALID_HANDLE_VALUE;
	if (valid)
	{
		// Enable SeDebugPrivilege.
		SetThreadProcessPrivilege(L"SeDebugPrivilege", true);

		// Get the list of currently running processes.
		HANDLE snaphandle, tempproc, proctoken, duptoken;
		PTOKEN_USER user = NULL;
		PTOKEN_GROUPS groups = NULL;
		PTOKEN_PRIVILEGES privs = NULL;
		BOOL result2;

		snaphandle = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snaphandle == INVALID_HANDLE_VALUE)  DumpErrorMsg("Unable to retrieve the list of running processes.", "create_toolhelp32_snapshot_failed", ::GetLastError());
		else
		{
			PROCESSENTRY32 pe32;
			pe32.dwSize = sizeof(PROCESSENTRY32);

			if (::Process32First(snaphandle, &pe32))
			{
				do
				{
					// Open a handle to the process.
					tempproc = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
					if (tempproc == NULL)  tempproc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);

					if (tempproc != NULL)
					{
						result2 = ::OpenProcessToken(tempproc, accessmode, &proctoken);
						if (result2 == NULL && (accessmode & TOKEN_QUERY_SOURCE))  result2 = ::OpenProcessToken(tempproc, accessmode & ~TOKEN_QUERY_SOURCE, &proctoken);
//						DWORD pid = ::GetProcessId(tempproc);

						::CloseHandle(tempproc);

						if (result2)
						{
							// Load token user, groups, and privileges.
							user = (PTOKEN_USER)AllocateAndGetTokenInformation(proctoken, TokenUser, 4096);
							groups = (PTOKEN_GROUPS)AllocateAndGetTokenInformation(proctoken, TokenGroups, 4096);
							privs = (PTOKEN_PRIVILEGES)AllocateAndGetTokenInformation(proctoken, TokenPrivileges, 4096);

							if (user != NULL && groups != NULL && privs != NULL)
							{
								// Match SIDs.
								for (x = 0; x < numsids; x++)
								{
									if (!::EqualSid(user->User.Sid, sids[x]))
									{
										for (x2 = 0; x2 < groups->GroupCount && !::EqualSid(groups->Groups[x2].Sid, sids[x]); x2++);

										if (x2 >= groups->GroupCount)  break;
									}
								}

								if (x >= numsids)
								{
									// Match privileges.
									for (x = 0; x < numluids; x++)
									{
										for (x2 = 0; x2 < privs->PrivilegeCount && (privs->Privileges[x2].Luid.LowPart != luids[x].LowPart || privs->Privileges[x2].Luid.HighPart != luids[x].HighPart); x2++);

										if (x2 >= privs->PrivilegeCount)  break;
									}

									if (x >= numluids)
									{
										// Everything matches.  Duplicate the token.
										if (accessmode & TOKEN_DUPLICATE)
										{
											SECURITY_ATTRIBUTES secattr = {0};
											secattr.nLength = sizeof(secattr);
											secattr.bInheritHandle = FALSE;
											secattr.lpSecurityDescriptor = NULL;

											if (::DuplicateTokenEx(proctoken, MAXIMUM_ALLOWED, &secattr, SecurityImpersonation, duptype, &duptoken))
											{
												result = duptoken;
											}
										}
										else
										{
											result = proctoken;
											proctoken = NULL;
										}
									}
								}
							}

							FreeTokenInformation((LPVOID)privs);
							FreeTokenInformation((LPVOID)groups);
							FreeTokenInformation((LPVOID)user);

							if (proctoken != NULL)  ::CloseHandle(proctoken);
						}
					}
				} while (result == INVALID_HANDLE_VALUE && ::Process32Next(snaphandle, &pe32));
			}

			::CloseHandle(snaphandle);

			if (result == INVALID_HANDLE_VALUE)  DumpErrorMsg("Unable to find a matching process for the token options.", "no_match_found", ::GetLastError());
		}
	}

	for (x = 0; x < numsids; x++)  ::FreeSid(sids[x]);

	::HeapFree(::GetProcessHeap(), 0, (LPVOID)sids);
	::HeapFree(::GetProcessHeap(), 0, (LPVOID)luids);

	return result;
}

typedef enum _KERNEL_PROCESS_INFORMATION_CLASS {
	NtProcessBasicInformation,
	NtProcessQuotaLimits,
	NtProcessIoCounters,
	NtProcessVmCounters,
	NtProcessTimes,
	NtProcessBasePriority,
	NtProcessRaisePriority,
	NtProcessDebugPort,
	NtProcessExceptionPort,
	NtProcessAccessToken,
	NtProcessLdtInformation,
	NtProcessLdtSize,
	NtProcessDefaultHardErrorMode,
	NtProcessIoPortHandlers,
	NtProcessPooledUsageAndLimits,
	NtProcessWorkingSetWatch,
	NtProcessUserModeIOPL,
	NtProcessEnableAlignmentFaultFixup,
	NtProcessPriorityClass,
	NtProcessWx86Information,
	NtProcessHandleCount,
	NtProcessAffinityMask,
	NtProcessPriorityBoost,
	NtMaxProcessInfoClass
} KERNEL_PROCESS_INFORMATION_CLASS, *KERNEL_PPROCESS_INFORMATION_CLASS;

typedef struct _KERNEL_PROCESS_ACCESS_TOKEN {
	HANDLE Token;
	HANDLE Thread;
} KERNEL_PROCESS_ACCESS_TOKEN, *KERNEL_PPROCESS_ACCESS_TOKEN;

CubicleSoft::SharedLib::ModuleUtil GxNTDLL("ntdll.dll");
CubicleSoft::SharedLib::FunctionUtil GxNtCreateToken(GxNTDLL, "NtCreateToken");
CubicleSoft::SharedLib::FunctionUtil GxRtlNtStatusToDosError(GxNTDLL, "RtlNtStatusToDosError");
CubicleSoft::SharedLib::FunctionUtil GxNtSetInformationProcess(GxNTDLL, "NtSetInformationProcess");

// Attempts to create a new token based on the input string.
HANDLE CreateTokenFromOpts(LPWSTR tokenopts, bool primary, HANDLE wildcardprocesshandle = ::GetCurrentProcess(), DWORD desiredaccess = TOKEN_ALL_ACCESS)
{
	// Enable or obtain and then enable SeCreateTokenPrivilege.
	if (!SetThreadProcessPrivilege(L"SeCreateTokenPrivilege", true))
	{
		// Obtain an impersonation token containing the privilege.
		HANDLE tokenhandle = FindExistingTokenFromOpts((LPWSTR)L"SeCreateTokenPrivilege", TokenImpersonation);

		if (tokenhandle == INVALID_HANDLE_VALUE)  return INVALID_HANDLE_VALUE;

		// Assign the impersonation token to the current thread.
		if (!::SetThreadToken(NULL, tokenhandle))
		{
			DumpErrorMsg("Unable to assign the token to the thread.", "set_thread_token_failed", ::GetLastError());

			return INVALID_HANDLE_VALUE;
		}

		// Enable the privilege.
		if (!SetThreadProcessPrivilege(L"SeCreateTokenPrivilege", true))
		{
			DumpErrorMsg("Unable to enable SeCreateTokenPrivilege.", "enable_privilege_failed", ::GetLastError());

			return INVALID_HANDLE_VALUE;
		}
	}

	// Split and convert the options.
	CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
	WCHAR tempchr;
	size_t x, x2, numsids, numluids;
	bool valid = true, freeusersid = false, freegroupsids = false, freeownersid = false, freeprimarygroupsid = false;
	HANDLE currtoken, result = INVALID_HANDLE_VALUE;
	PTOKEN_USER user = NULL;
	PTOKEN_GROUPS groups = NULL;
	PTOKEN_PRIVILEGES privs = NULL;
	PTOKEN_OWNER owner = NULL;
	PTOKEN_PRIMARY_GROUP primarygroup = NULL;
	PSECURITY_DESCRIPTOR sd = NULL;
	PTOKEN_DEFAULT_DACL defaultdacl = NULL;
	PTOKEN_SOURCE source = NULL;

	TempVar.SetStr(tokenopts);
	WCHAR *tokenopts2 = TempVar.GetStr();

	// For wildcard options (*), use the specified process token.
	if (!::OpenProcessToken(wildcardprocesshandle, TOKEN_QUERY | TOKEN_QUERY_SOURCE, &currtoken) && !::OpenProcessToken(wildcardprocesshandle, TOKEN_QUERY, &currtoken))
	{
		DumpErrorMsg("Unable to obtain a handle to the process token.", "open_process_token_failed", ::GetLastError());

		return INVALID_HANDLE_VALUE;
	}

	// Get the user SID.
	x = 0;
	for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

	if (tokenopts2[x] == L'*')
	{
		user = (PTOKEN_USER)AllocateAndGetTokenInformation(currtoken, TokenUser, 4096);
		if (user == NULL)
		{
			DumpErrorMsg("Unable to retrieve user for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

			valid = false;
		}
	}
	else
	{
		user = (PTOKEN_USER)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_USER));
		if (user != NULL)
		{
			valid = GetNextTokenOptsSID(tokenopts2, x, user->User.Sid);
			if (valid)  freeusersid = true;

			// Attributes are always ignored for the user and must be 0.
			user->User.Attributes = 0;
		}
	}

	for (; tokenopts2[x] && tokenopts2[x] != L';'; x++);
	if (tokenopts2[x] == L';')  x++;

	// Get group SIDs and attributes.
	if (valid)
	{
		for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

		if (tokenopts2[x] == L'*')
		{
			groups = (PTOKEN_GROUPS)AllocateAndGetTokenInformation(currtoken, TokenGroups, 4096);
			if (groups == NULL)
			{
				DumpErrorMsg("Unable to retrieve group SIDs and attributes for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

				valid = false;
			}
		}
		else
		{
			GetNumSIDsAndLUIDs(tokenopts2 + x, numsids, numluids);

			groups = (PTOKEN_GROUPS)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_GROUPS) + sizeof(SID_AND_ATTRIBUTES) * numsids);
			if (groups != NULL)
			{
				freegroupsids = true;

				groups->GroupCount = (DWORD)numsids;

				for (x2 = 0; valid && x2 < numsids; x2++)
				{
					valid = GetNextTokenOptsSID(tokenopts2, x, groups->Groups[x2].Sid);

					groups->Groups[x2].Attributes = GetNextTokenOptsAttrs(tokenopts2, x);

					if (tokenopts2[x] == L',')  x++;
				}

				if (!valid)  numsids = x2;
			}
		}

		for (; tokenopts2[x] && tokenopts2[x] != L';'; x++);
		if (tokenopts2[x] == L';')  x++;
	}

	// Get privileges and attributes.
	if (valid)
	{
		for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

		if (tokenopts2[x] == L'*')
		{
			privs = (PTOKEN_PRIVILEGES)AllocateAndGetTokenInformation(currtoken, TokenPrivileges, 4096);
			if (privs == NULL)
			{
				DumpErrorMsg("Unable to retrieve privileges and attributes for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

				valid = false;
			}
		}
		else
		{
			GetNumSIDsAndLUIDs(tokenopts2 + x, numsids, numluids);

			privs = (PTOKEN_PRIVILEGES)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES) * numluids);
			if (privs != NULL)
			{
				privs->PrivilegeCount = (DWORD)numluids;

				for (x2 = 0; valid && x2 < numluids; x2++)
				{
					valid = GetNextTokenOptsLUID(tokenopts2, x, privs->Privileges[x2].Luid);

					privs->Privileges[x2].Attributes = GetNextTokenOptsAttrs(tokenopts2, x);

					if (tokenopts2[x] == L',')  x++;
				}

				if (!valid)  numluids = x2;
			}
		}

		for (; tokenopts2[x] && tokenopts2[x] != L';'; x++);
		if (tokenopts2[x] == L';')  x++;
	}

	// Get the owner SID.
	if (valid)
	{
		for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

		if (tokenopts2[x] == L'*')
		{
			owner = (PTOKEN_OWNER)AllocateAndGetTokenInformation(currtoken, TokenOwner, 4096);
			if (owner == NULL)
			{
				DumpErrorMsg("Unable to retrieve the owner for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

				valid = false;
			}
		}
		else
		{
			owner = (PTOKEN_OWNER)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_OWNER));
			if (owner != NULL)
			{
				valid = GetNextTokenOptsSID(tokenopts2, x, owner->Owner);
				if (valid)  freeownersid = true;
			}
		}

		for (; tokenopts2[x] && tokenopts2[x] != L';'; x++);
		if (tokenopts2[x] == L';')  x++;
	}

	// Get the primary group SID.
	if (valid)
	{
		for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

		if (tokenopts2[x] == L'*')
		{
			primarygroup = (PTOKEN_PRIMARY_GROUP)AllocateAndGetTokenInformation(currtoken, TokenPrimaryGroup, 4096);
			if (primarygroup == NULL)
			{
				DumpErrorMsg("Unable to retrieve the primary group for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

				valid = false;
			}
		}
		else
		{
			primarygroup = (PTOKEN_PRIMARY_GROUP)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_PRIMARY_GROUP));
			if (primarygroup != NULL)
			{
				valid = GetNextTokenOptsSID(tokenopts2, x, primarygroup->PrimaryGroup);
				if (valid)  freeprimarygroupsid = true;
			}
		}

		for (; tokenopts2[x] && tokenopts2[x] != L';'; x++);
		if (tokenopts2[x] == L';')  x++;
	}

	// Get the default DACL.
	if (valid)
	{
		for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

		if (tokenopts2[x] == L'*')
		{
			defaultdacl = (PTOKEN_DEFAULT_DACL)AllocateAndGetTokenInformation(currtoken, TokenDefaultDacl, 4096);
			if (defaultdacl == NULL)
			{
				DumpErrorMsg("Unable to retrieve the default DACL for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

				valid = false;
			}
		}
		else
		{
			for (x2 = x; tokenopts2[x2] && tokenopts2[x2] != L';'; x2++)
			{
				if (tokenopts2[x2] == L'(')
				{
					for (; tokenopts2[x2] && tokenopts2[x2] != L')'; x2++);
				}
			}
			tempchr = tokenopts2[x2];
			tokenopts2[x2] = L'\0';

			defaultdacl = (PTOKEN_DEFAULT_DACL)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_DEFAULT_DACL));
			if (defaultdacl != NULL)
			{
				BOOL daclpresent, dacldefaulted;

				valid = ::ConvertStringSecurityDescriptorToSecurityDescriptorW(tokenopts2 + x, SDDL_REVISION_1, &sd, NULL) && ::GetSecurityDescriptorDacl(sd, &daclpresent, &defaultdacl->DefaultDacl, &dacldefaulted) && daclpresent;
				if (!valid)  DumpErrorMsg("The specified default DACL in the token options is invalid.", "invalid_default_dacl", ::GetLastError());
			}

			tokenopts2[x2] = tempchr;
			x = x2;
		}

		for (; tokenopts2[x] && tokenopts2[x] != L';'; x++);
		if (tokenopts2[x] == L';')  x++;
	}

	// Get the source and source LUID.
	if (valid)
	{
		for (; tokenopts2[x] && tokenopts2[x] == L' '; x++);

		if (tokenopts2[x] == L'*')
		{
			source = (PTOKEN_SOURCE)AllocateAndGetTokenInformation(currtoken, TokenSource, sizeof(TOKEN_SOURCE));
			if (source == NULL)
			{
				DumpErrorMsg("Unable to retrieve the source for the current token.", "allocate_and_get_token_info_failed", ::GetLastError());

				valid = false;
			}
		}
		else
		{
			for (x2 = x; tokenopts2[x2] && x2 - x != TOKEN_SOURCE_LENGTH * 2; x2++);

			if (tokenopts2[x2] != L':')  DumpErrorMsg("The specified source in the token options is invalid.", "invalid_source", ::GetLastError());
			else
			{
				source = (PTOKEN_SOURCE)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOKEN_SOURCE));

				// Convert from hex.
				for (x2 = x; tokenopts2[x2] && x2 - x != TOKEN_SOURCE_LENGTH * 2; x2 += 2)
				{
					if (tokenopts2[x2] >= L'0' && tokenopts2[x2] <= L'9')  tempchr = tokenopts2[x2] - L'0';
					else if (tokenopts2[x2] >= L'A' && tokenopts2[x2] <= L'F')  tempchr = tokenopts2[x2] - L'A';
					else if (tokenopts2[x2] >= L'a' && tokenopts2[x2] <= L'f')  tempchr = tokenopts2[x2] - L'a';
					else  tempchr = 0;

					tempchr <<= 4;

					if (tokenopts2[x2 + 1] >= L'0' && tokenopts2[x2 + 1] <= L'9')  tempchr |= tokenopts2[x2 + 1] - L'0';
					else if (tokenopts2[x2 + 1] >= L'A' && tokenopts2[x2 + 1] <= L'F')  tempchr |= tokenopts2[x2 + 1] - L'A';
					else if (tokenopts2[x2 + 1] >= L'a' && tokenopts2[x2 + 1] <= L'f')  tempchr |= tokenopts2[x2 + 1] - L'a';

					source->SourceName[(x2 - x) / 2] = (CHAR)tempchr;
				}

				std::uint64_t tempid = _wtoi64(tokenopts2 + x2 + 1);

				source->SourceIdentifier.HighPart = tempid >> 32;
				source->SourceIdentifier.LowPart = tempid & 0xFFFFFFFF;
			}
		}
	}

	// Create the token.
	if (valid && user != NULL && groups != NULL && privs != NULL && owner != NULL && primarygroup != NULL && defaultdacl != NULL && source != NULL)
	{
		NTSTATUS status;
		HANDLE temphandle;
		TOKEN_STATISTICS tokenstats;
		DWORD templen;
		DWORD winerror;

		if (!::GetTokenInformation(currtoken, TokenStatistics, &tokenstats, sizeof(tokenstats), &templen))  DumpErrorMsg("Unable to get statistics for the current token.", "get_token_info_failed", ::GetLastError());
		else
		{
			SECURITY_QUALITY_OF_SERVICE tempsqos = {
				sizeof(tempsqos), SecurityImpersonation, SECURITY_DYNAMIC_TRACKING
			};

			OBJECT_ATTRIBUTES tempoa = {
				sizeof(tempoa), 0, 0, 0, 0, &tempsqos
			};

			if (CubicleSoft::SharedLib::Stdcall<NTSTATUS, PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, TOKEN_TYPE, PLUID, PLARGE_INTEGER, PTOKEN_USER, PTOKEN_GROUPS, PTOKEN_PRIVILEGES, PTOKEN_OWNER, PTOKEN_PRIMARY_GROUP, PTOKEN_DEFAULT_DACL, PTOKEN_SOURCE>(GxNtCreateToken, status, &temphandle, desiredaccess, &tempoa, (primary ? TokenPrimary : TokenImpersonation), &tokenstats.AuthenticationId, &tokenstats.ExpirationTime, user, groups, privs, owner, primarygroup, defaultdacl, source) && status >= 0)
			{
				result = temphandle;
			}
			else
			{
				if (CubicleSoft::SharedLib::Stdcall<ULONG, NTSTATUS>(GxRtlNtStatusToDosError, winerror, status) && winerror != ERROR_MR_MID_NOT_FOUND)  DumpErrorMsg("Failed to create a token.", "nt_create_token_failed", winerror);
				else  DumpErrorMsg("Failed to create a token and failed to retrieve a Windows error code mapping.", "nt_create_token_failed", status);
			}
		}
	}

	::CloseHandle(currtoken);

	// Cleanup.
	if (user != NULL)
	{
		if (freeusersid)  ::FreeSid(user->User.Sid);

		FreeTokenInformation((LPVOID)user);
	}

	if (groups != NULL)
	{
		for (x = 0; x < numsids; x++)  ::FreeSid(groups->Groups[x].Sid);

		FreeTokenInformation((LPVOID)groups);
	}

	if (privs != NULL)  FreeTokenInformation((LPVOID)privs);

	if (owner != NULL)
	{
		if (freeownersid)  ::FreeSid(owner->Owner);

		FreeTokenInformation((LPVOID)owner);
	}

	if (primarygroup != NULL)
	{
		if (freeprimarygroupsid)  ::FreeSid(primarygroup->PrimaryGroup);

		FreeTokenInformation((LPVOID)primarygroup);
	}

	if (sd != NULL)  ::LocalFree(sd);
	if (defaultdacl != NULL)  FreeTokenInformation((LPVOID)defaultdacl);
	if (source != NULL)  FreeTokenInformation((LPVOID)source);

	return result;
}


// Determines whether or not the current process is elevated.
int IsElevatedProcess()
{
	HANDLE temptoken;
	PTOKEN_ELEVATION teinfo;
	int result = -1;

	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &temptoken))  DumpErrorMsg("Unable to open a handle to the process token.", "open_process_token_failed", ::GetLastError());
	else
	{
		teinfo = (PTOKEN_ELEVATION)AllocateAndGetTokenInformation(temptoken, TokenElevation, sizeof(TOKEN_ELEVATION));

		if (teinfo == NULL)  DumpErrorMsg("Unable to get elevation status.", "get_token_information_failed", ::GetLastError());
		else
		{
			result = (int)(teinfo->TokenIsElevated ? 1 : 0);

			FreeTokenInformation((LPVOID)teinfo);
		}

		::CloseHandle(temptoken);
	}

	return result;
}

// Determines whether or not the current process is running as SYSTEM.
int IsSystemProcess()
{
	HANDLE temptoken;
	PTOKEN_USER tuinfo = NULL;
	PSID systemsid;
	int result = -1;

	// NT AUTHORITY\SYSTEM
	::ConvertStringSidToSid(_T("S-1-5-18"), &systemsid);

	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &temptoken))  DumpErrorMsg("Unable to open a handle to the process token.", "open_process_token_failed", ::GetLastError());
	else
	{
		tuinfo = (PTOKEN_USER)AllocateAndGetTokenInformation(temptoken, TokenUser, 4096);

		if (tuinfo == NULL)  DumpErrorMsg("Unable to get user token for the process.", "get_token_information_failed", ::GetLastError());
		else
		{
			result = (::EqualSid(systemsid, tuinfo->User.Sid) ? 1 : 0);

			FreeTokenInformation((LPVOID)tuinfo);
		}

		::CloseHandle(temptoken);
	}

	::FreeSid(systemsid);

	return result;
}


// Attempts to self-elevate.
bool ElevateSelf(bool createservice)
{
	// Retrieve the full EXE path and filename for this process.
	WCHAR procfilename[8192], procdir[8192];
	CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
	size_t y = (size_t)::GetModuleFileNameW(NULL, procfilename, sizeof(procfilename) / sizeof(WCHAR));
	if (y >= sizeof(procfilename) / sizeof(WCHAR) - 50)
	{
		DumpErrorMsg("Unable to retrieve EXE path and filename.", "get_module_filename_failed", ::GetLastError());

		return false;
	}
	procfilename[y] = L'\0';

	// Remove Zone.Identifier.
	if (y < TempVar.GetMaxSize() - 18)
	{
		TempVar.SetStr(procfilename);
		TempVar.AppendStr(L":Zone.Identifier");
		::DeleteFileW(TempVar.GetStr());
	}

	memcpy(procdir, procfilename, (y + 1) * sizeof(WCHAR));
	while (y && procdir[y - 1] != L'\\' && procdir[y - 1] != L'/')  y--;
	procdir[y] = L'\0';

	// If this process has a console, make the new process connect to the console.
	if (::GetConsoleWindow() == NULL)  TempVar.SetStr(L"");
	else
	{
		TempVar.SetStr(L"/attach=");
		TempVar.AppendUInt(::GetCurrentProcessId());
		TempVar.AppendChar(L' ');
	}

	// The new process will either start a system service or run elevated.
	if (createservice)
	{
		TempVar.AppendStr(L"/connectpipe=");
		TempVar.AppendUInt(::GetCurrentProcessId());
		TempVar.AppendStr(L" /createservice");
	}
	else
	{
		TempVar.AppendStr(L"/runelevated=");
		TempVar.AppendUInt(::GetCurrentProcessId());
	}

	// Trigger the elevation prompt.
	SHELLEXECUTEINFOW seinfo = {0};

	seinfo.cbSize = sizeof(seinfo);
	seinfo.fMask = 0;
	seinfo.hwnd = NULL;
	seinfo.lpVerb = L"runas";
	seinfo.lpFile = (LPWSTR)procfilename;
	seinfo.lpParameters = (LPWSTR)TempVar.GetStr();
	seinfo.lpDirectory = (LPWSTR)procdir;
	seinfo.nShow = SW_NORMAL;

	// ShellExecuteExW() does not return until the elevation prompt is handled.
	if (!::ShellExecuteExW(&seinfo))
	{
		DumpErrorMsg("Unable to create elevated process.", "shell_execute_ex_failed", ::GetLastError());

		return false;
	}

	return true;
}

// Attempts to start a system service.
bool SystemSelf(DWORD connectpid)
{
	// Retrieve the full EXE path and filename for this process.
	WCHAR procfilename[8192];
	size_t y = (size_t)::GetModuleFileNameW(NULL, procfilename, sizeof(procfilename) / sizeof(WCHAR));
	if (y >= sizeof(procfilename) / sizeof(WCHAR) - 1)
	{
		DumpErrorMsg("Unable to retrieve EXE path and filename.", "get_module_filename_failed", ::GetLastError());

		return false;
	}
	procfilename[y] = L'\0';

	// Prepare an event synchronization object.
	CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;
	HANDLE tempevent;
	DWORD pid = ::GetCurrentProcessId();

	TempVar.SetStr(L"Global\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_SrvEv_");
	TempVar.AppendUInt(pid);

	tempevent = ::CreateEventW(NULL, FALSE, FALSE, TempVar.GetStr());
	if (tempevent == NULL)
	{
		DumpErrorMsg("Unable to create service notification event object.", "create_event_failed", ::GetLastError());

		return false;
	}

	// Get a handle to the service control manager.
	SC_HANDLE scmhandle = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE | SERVICE_START | DELETE);
	if (scmhandle == NULL)
	{
		DumpErrorMsg("Unable to open a handle to the service control manager.", "open_sc_manager_failed", ::GetLastError());

		::CloseHandle(tempevent);

		return false;
	}

	// Create the service.
	CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar2;

	TempVar.SetStr(L"z_createprocess-");
	TempVar.AppendUInt(pid);

	TempVar2.SetStr(L"\"");
	TempVar2.AppendStr(procfilename);
	TempVar2.AppendStr(L"\" /connectpipe=");
	TempVar2.AppendUInt(connectpid);
	TempVar2.AppendStr(L" /runservice=");
	TempVar2.AppendUInt(pid);

	SC_HANDLE servicehandle = ::CreateServiceW(scmhandle, TempVar.GetStr(), TempVar.GetStr(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, TempVar2.GetStr(), NULL, NULL, NULL, NULL, NULL);
	if (servicehandle == NULL)
	{
		DumpErrorMsg("Unable to create the system service.", "create_service_failed", ::GetLastError());

		::CloseServiceHandle(scmhandle);

		::CloseHandle(tempevent);

		return false;
	}

	// Start the system service.
	::StartService(servicehandle, 0, NULL);

	// Wait for up to 15 seconds for the process to fire the event.
#ifdef _DEBUG
	if (::WaitForSingleObject(tempevent, INFINITE) != WAIT_OBJECT_0)
#else
	if (::WaitForSingleObject(tempevent, 15000) != WAIT_OBJECT_0)
#endif
	{
		DumpErrorMsg("System service process did not respond within 15 seconds.", "timeout", ::GetLastError());

		::DeleteService(servicehandle);

		::CloseServiceHandle(servicehandle);
		::CloseServiceHandle(scmhandle);

		::CloseHandle(tempevent);

		return false;
	}

	::DeleteService(servicehandle);

	::CloseServiceHandle(servicehandle);
	::CloseServiceHandle(scmhandle);

	::CloseHandle(tempevent);

	return true;
}


struct SidInfo {
	PSID MxSid;
	TCHAR MxDomainName[1024];
	TCHAR MxAccountName[1024];
	SID_NAME_USE MxSidType;
};

// Enable SeBackupPrivilege, SeRestorePrivilege, SeIncreaseQuotaPrivilege, and SeAssignPrimaryTokenPrivilege on the current process token.
// Only the SYSTEM user has all of these by default.
bool EnableSystemPrivileges()
{
	if (!::SetThreadProcessPrivilege(L"SeBackupPrivilege", true))
	{
		DumpErrorMsg("Failed to enable SeBackupPrivilege.", "enable_privilege_failed", ::GetLastError());

		return false;
	}

	if (!::SetThreadProcessPrivilege(L"SeRestorePrivilege", true))
	{
		DumpErrorMsg("Failed to enable SeRestorePrivilege.", "enable_privilege_failed", ::GetLastError());

		return false;
	}

	if (!::SetThreadProcessPrivilege(L"SeIncreaseQuotaPrivilege", true))
	{
		DumpErrorMsg("Failed to enable SeIncreaseQuotaPrivilege.", "enable_privilege_failed", ::GetLastError());

		return false;
	}

	if (!::SetThreadProcessPrivilege(L"SeAssignPrimaryTokenPrivilege", true))
	{
		DumpErrorMsg("Failed to enable SeAssignPrimaryTokenPrivilege.", "enable_privilege_failed", ::GetLastError());

		return false;
	}

	return true;
}

// Loads the user profile associated with the token.
// Note that some user profiles are roaming, so call ::NetUserGetInfo() to get the correct profile path.
bool LoadRealUserProfile(HANDLE maintoken, PROFILEINFO &profile)
{
	::ZeroMemory(&profile, sizeof(profile));

	PTOKEN_USER tuinfo = (PTOKEN_USER)AllocateAndGetTokenInformation(maintoken, TokenUser, 4096);
	if (tuinfo == NULL)
	{
		DumpErrorMsg("User token was retrieved/created but retrieving user information failed.", "get_token_information_failed", ::GetLastError());

		return false;
	}

	SidInfo TempSidInfo;
	DWORD acctbuffersize, domainbuffersize;
	NET_API_STATUS netstatus;
	LPUSER_INFO_4 userinfo4 = NULL;

	TempSidInfo.MxSid = tuinfo->User.Sid;

	acctbuffersize = sizeof(TempSidInfo.MxAccountName) / sizeof(TCHAR);
	domainbuffersize = sizeof(TempSidInfo.MxDomainName) / sizeof(TCHAR);

	if (!::LookupAccountSid(NULL, TempSidInfo.MxSid, TempSidInfo.MxAccountName, &acctbuffersize, TempSidInfo.MxDomainName, &domainbuffersize, &TempSidInfo.MxSidType))
	{
		DumpErrorMsg("User token was retrieved/created but retrieving SID information failed.", "lookup_account_sid_failed", ::GetLastError());

		FreeTokenInformation((LPVOID)tuinfo);

		return false;
	}

	FreeTokenInformation((LPVOID)tuinfo);

	profile.dwSize = sizeof(profile);
	profile.lpUserName = TempSidInfo.MxAccountName;

	if (TempSidInfo.MxSidType == SidTypeUser || TempSidInfo.MxSidType == SidTypeDeletedAccount)
	{
		netstatus = ::NetUserGetInfo(TempSidInfo.MxDomainName, TempSidInfo.MxAccountName, 4, (LPBYTE *)&userinfo4);

		if (netstatus == NERR_Success && userinfo4 != NULL)  profile.lpProfilePath = userinfo4->usri4_profile;
	}

	// LoadUserProfile() requires SeBackupPrivilege and SeRestorePrivilege and also running as a system service.
	if (!::LoadUserProfile(maintoken, &profile))
	{
		DumpErrorMsg("Loading the user profile failed.", "load_user_profile_failed", ::GetLastError());

		if (userinfo4 != NULL)  ::NetApiBufferFree(userinfo4);

		return false;
	}

	if (userinfo4 != NULL)  ::NetApiBufferFree(userinfo4);

	return true;
}

// Assigns the primary token to the process.
// The process must have just been created as a suspended process for this to work.
// The undocumented NtSetInformationProcess() API supposedly requires the SeAssignPrimaryTokenPrivilege (preferably) or possibly SeDebugPrivilege.
bool AssignPrimaryTokenToProcess(HANDLE prochandle, HANDLE maintoken)
{
	KERNEL_PROCESS_ACCESS_TOKEN pat;
	NTSTATUS status;
	DWORD winerror;

	pat.Token = maintoken;
	pat.Thread = (HANDLE)0;

	if (!CubicleSoft::SharedLib::Stdcall<NTSTATUS, HANDLE, KERNEL_PROCESS_INFORMATION_CLASS, PVOID, ULONG>(GxNtSetInformationProcess, status, prochandle, NtProcessAccessToken, &pat, sizeof(pat)) || status < 0)
	{
		if (CubicleSoft::SharedLib::Stdcall<ULONG, NTSTATUS>(GxRtlNtStatusToDosError, winerror, status) && winerror != ERROR_MR_MID_NOT_FOUND)  DumpErrorMsg("Failed to assign the primary token to the target process.", "nt_set_information_process_failed", winerror);
		else  DumpErrorMsg("Failed to assign the primary token to the target process and failed to retrieve a Windows error code mapping.", "nt_set_information_process_failed", status);

		return false;
	}

	return true;
}


struct ServiceInitState {
	SERVICE_STATUS MxServiceStatus;
	SERVICE_STATUS_HANDLE MxStatusHandle;
	DWORD MxCreatorPID;
	LPTSTR MxServiceName;
	DWORD MxExitCode;
};

ServiceInitState GxService;

// Notify the elevated process that started this service that it can clean up and terminate.
bool NotifyServiceCreatorProcess()
{
	if (!GxService.MxCreatorPID)  return true;

	CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;
	HANDLE tempevent;

	TempVar.SetStr(L"Global\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_SrvEv_");
	TempVar.AppendUInt(GxService.MxCreatorPID);

	GxService.MxCreatorPID = 0;

	tempevent = ::CreateEventW(NULL, FALSE, FALSE, TempVar.GetStr());
	if (tempevent == NULL)
	{
		DumpErrorMsg("Unable to create service notification event object.", "create_event_failed", ::GetLastError());

		return false;
	}

	::SetEvent(tempevent);

	::CloseHandle(tempevent);

	return true;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlType)
{
	if (CtrlType == SERVICE_CONTROL_STOP && GxService.MxServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		// Let the service control manager know that the service will be stopping shortly.
		GxService.MxServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		GxService.MxServiceStatus.dwControlsAccepted = 0;
		GxService.MxServiceStatus.dwWin32ExitCode = NO_ERROR;
		GxService.MxServiceStatus.dwCheckPoint = 1;
		::SetServiceStatus(GxService.MxStatusHandle, &GxService.MxServiceStatus);
	}
}

bool ServiceMainInternal()
{
	if (!ConnectToMainCommPipe())
	{
		DumpErrorMsg("Unable to connect to the main communication pipe.", "connect_to_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	// Enable necessary privileges.
	if (!EnableSystemPrivileges())  return false;

	// Notify that all is well so far.
	::WriteFile(GxMainCommPipeClient, "\x01", 1, NULL, NULL);

	// Receive the token type.
	char assigntoken;
	if (!ReadFileExact(GxMainCommPipeClient, &assigntoken, 1))
	{
		DumpErrorMsg("Unable to read token type from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

		return false;
	}

	// Load the primary token.  Requires SeDebugPrivilege.
	// The undocumented NtCreateToken() API requires SeCreateTokenPrivilege.
	HANDLE maintoken;
	if (assigntoken == 0)  maintoken = GetTokenFromPID(::GetCurrentProcessId(), TokenPrimary);
	else if (assigntoken == 1)  maintoken = GetTokenFromPID(GxService.MxCreatorPID, TokenPrimary);
	else if (assigntoken == 2 || assigntoken == 3)
	{
		// Read token options.
		DWORD tempsize;
		LPWSTR tokenopts = NULL;

		if (!ReadCommPipeString(GxMainCommPipeClient, tokenopts, tempsize, 1))  return false;

		if (assigntoken == 2)  maintoken = FindExistingTokenFromOpts(tokenopts, TokenPrimary);
		else  maintoken = CreateTokenFromOpts(tokenopts, true);

		FreeCommPipeString(tokenopts);
	}
	else
	{
		DumpErrorMsg("Invalid token type sent on the main communication pipe.", "invalid_token_type", ::GetLastError());

		return false;
	}

	if (maintoken == INVALID_HANDLE_VALUE)  return false;

	// Notify that this process is done with the creator process and can remove the NT service and continue.
	if (!NotifyServiceCreatorProcess())  return false;

	// Load the user profile associated with the token.
	PROFILEINFO tempprofile = {0};
	if (!LoadRealUserProfile(maintoken, tempprofile))
	{
		::CloseHandle(maintoken);

		return false;
	}

	// Generate an environment block for the user.
	LPVOID tempenv = NULL;

	if (!::CreateEnvironmentBlock(&tempenv, maintoken, FALSE))
	{
		DumpErrorMsg("Creating the environment block for the user token failed.", "create_environment_block_failed", ::GetLastError());

		if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

		::CloseHandle(maintoken);

		return false;
	}

	// Notify that all is well so far.
	::WriteFile(GxMainCommPipeClient, "\x01", 1, NULL, NULL);

	// Send the environment block.
	WCHAR *envp = (WCHAR *)tempenv;
	CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
	DWORD tempsize = sizeof(WCHAR);
	int x = 0;
	while (envp[x])
	{
		TempVar.SetStr(envp + x);
		tempsize += (TempVar.GetSize() + 1) * sizeof(WCHAR);

		x += TempVar.GetSize() + 1;
	}

	::WriteFile(GxMainCommPipeClient, &tempsize, 4, NULL, NULL);
	x = 0;
	while (envp[x])
	{
		TempVar.SetStr(envp + x);
		tempsize = (TempVar.GetSize() + 1) * sizeof(WCHAR);
		::WriteFile(GxMainCommPipeClient, TempVar.GetStr(), tempsize, NULL, NULL);

		x += TempVar.GetSize() + 1;
	}

	::WriteFile(GxMainCommPipeClient, L"\0", sizeof(WCHAR), NULL, NULL);

	::DestroyEnvironmentBlock(tempenv);

	// Receive the process ID to modify.
	DWORD pid;
	if (!ReadFileExact(GxMainCommPipeClient, &pid, sizeof(DWORD)))
	{
		DumpErrorMsg("Unable to read process ID from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

		if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

		::CloseHandle(maintoken);

		return false;
	}

	// Open the process specified by the process ID.
	HANDLE tempproc = ::OpenProcess(SYNCHRONIZE | PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (tempproc == NULL)  tempproc = ::OpenProcess(SYNCHRONIZE | PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

	if (tempproc == NULL)
	{
		DumpErrorMsg("Unable to open the specified process.", "open_process_failed", ::GetLastError());

		if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

		::CloseHandle(maintoken);

		return false;
	}

	// Assign the primary token to the process.
	if (!AssignPrimaryTokenToProcess(tempproc, maintoken))
	{
		::CloseHandle(tempproc);

		if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

		::CloseHandle(maintoken);

		return false;
	}

	// Notify that all is well so far.
	::WriteFile(GxMainCommPipeClient, "\x01", 1, NULL, NULL);

	// Wait for the process to terminate.
	::WaitForSingleObject(tempproc, INFINITE);

	::CloseHandle(tempproc);

	DisconnectMainCommPipe();

	// Unload the user profile.
	if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

	::CloseHandle(maintoken);

	return true;
}

VOID WINAPI ServiceMain(DWORD argc, TCHAR **argv)
{
	// Register the service control handler.
	GxService.MxStatusHandle = ::RegisterServiceCtrlHandler(GxService.MxServiceName, ServiceCtrlHandler);
	if (GxService.MxStatusHandle == NULL)  return;

	// Let the service control manager know that the application is started.
	GxService.MxServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	GxService.MxServiceStatus.dwCurrentState = SERVICE_RUNNING;
	GxService.MxServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	GxService.MxServiceStatus.dwWin32ExitCode = NO_ERROR;
	GxService.MxServiceStatus.dwServiceSpecificExitCode = 0;
	GxService.MxServiceStatus.dwCheckPoint = 0;
	if (!::SetServiceStatus(GxService.MxStatusHandle, &GxService.MxServiceStatus))  return;

	// A quick debugger hook.
#ifdef _DEBUG
	while (!::IsDebuggerPresent())  ::Sleep(500);
	::Sleep(2000);
#endif

	// Get past the event object portion of SystemSelf() if the named pipe and the creator process are the same.
	if (GxConnectPipePID == GxService.MxCreatorPID)  NotifyServiceCreatorProcess();

	GxService.MxExitCode = (ServiceMainInternal() ? 0 : 1);

	// Notify that this process is done with the creator process and can remove the NT service and continue.
	NotifyServiceCreatorProcess();

	// Stop the service.
	GxService.MxServiceStatus.dwCurrentState = SERVICE_STOPPED;
	GxService.MxServiceStatus.dwControlsAccepted = 0;
	GxService.MxServiceStatus.dwWin32ExitCode = GxService.MxExitCode;
	GxService.MxServiceStatus.dwCheckPoint = 0;
	::SetServiceStatus(GxService.MxStatusHandle, &GxService.MxServiceStatus);
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

		// There's no particularly good way to handle errors here without completely rewriting the function.
		if (!ReadFileExact(::GetStdHandle(STD_INPUT_HANDLE), tokenbuf, tokenlen) || ::send(s, tokenbuf, (int)tokenlen, 0) != tokenlen)
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

// Moved these out of the main function into the global namespace so the thread entry functions have relatively clean access to them.
SOCKET Gx_stdinsocket = INVALID_SOCKET;
HANDLE Gx_stdinwritehandle = NULL;

SOCKET Gx_stdoutsocket = INVALID_SOCKET;
HANDLE Gx_stdoutreadhandle = NULL;

SOCKET Gx_stderrsocket = INVALID_SOCKET;
HANDLE Gx_stderrreadhandle = NULL;

DWORD WINAPI StdinSocketHandler(LPVOID)
{
	char buffer[65536];
	int bufferlen;

	do
	{
		// Stdin socket -> stdin pipe.
		bufferlen = ::recv(Gx_stdinsocket, buffer, sizeof(buffer), 0);
		if (!bufferlen || bufferlen == SOCKET_ERROR)  break;

		if (!::WriteFile(Gx_stdinwritehandle, buffer, (DWORD)bufferlen, NULL, NULL))  break;

	} while (1);

	::closesocket(Gx_stdinsocket);
	::CloseHandle(Gx_stdinwritehandle);

	Gx_stdinsocket = INVALID_SOCKET;
	Gx_stdinwritehandle = NULL;

	return 0;
}

DWORD WINAPI StdoutSocketHandler(LPVOID)
{
	char buffer[65536];
	DWORD bufferlen;

	do
	{
		// Stdout pipe -> stdout socket.
		if (!::ReadFile(Gx_stdoutreadhandle, buffer, sizeof(buffer), &bufferlen, NULL))  break;

		if (bufferlen && ::send(Gx_stdoutsocket, buffer, (int)bufferlen, 0) != bufferlen)  break;

	} while (1);

	::shutdown(Gx_stdoutsocket, SD_SEND);
	::recv(Gx_stdoutsocket, buffer, sizeof(buffer), 0);

	::closesocket(Gx_stdoutsocket);
	::CloseHandle(Gx_stdoutreadhandle);

	Gx_stdoutsocket = INVALID_SOCKET;
	Gx_stdoutreadhandle = NULL;

	return 0;
}

DWORD WINAPI StderrSocketHandler(LPVOID)
{
	char buffer[65536];
	DWORD bufferlen;

	do
	{
		// Stderr pipe -> stderr socket.
		if (!::ReadFile(Gx_stderrreadhandle, buffer, sizeof(buffer), &bufferlen, NULL))  break;

		if (bufferlen && ::send(Gx_stderrsocket, buffer, (int)bufferlen, 0) != bufferlen)  break;

	} while (1);

	::shutdown(Gx_stderrsocket, SD_SEND);
	::recv(Gx_stderrsocket, buffer, sizeof(buffer), 0);

	::closesocket(Gx_stderrsocket);
	::CloseHandle(Gx_stderrreadhandle);

	Gx_stderrsocket = INVALID_SOCKET;
	Gx_stderrreadhandle = NULL;

	return 0;
}


int _tmain(int argc, TCHAR **argv, TCHAR **envp)
{
	bool verbose = false;
	bool wait = false;
	bool terminate = false;
	DWORD waitamount = INFINITE;
	LPTSTR pidfile = NULL;
	char assigntoken = -1;
	LPTSTR tokenopts = NULL;
	bool mergeenv = false;
	HANDLE connectpipe = INVALID_HANDLE_VALUE;
	HANDLE mutexhandle = NULL, semaphorehandle = NULL;
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
	TCHAR *stdinstr = (TCHAR *)_T(":stdin");
	TCHAR *stdoutstr = (TCHAR *)_T(":stdout");
	TCHAR *stderrstr = (TCHAR *)_T(":stderr");
	TCHAR *socketip = NULL;
	unsigned short socketport = 0;
	TCHAR *sockettoken = NULL;
	unsigned short sockettokenlen = 0;
	HANDLE temphandle = NULL, stdinread = NULL, stdoutwrite = NULL, stderrwrite = NULL;
	HANDLE stdinthread = NULL, stdoutthread = NULL, stderrthread = NULL;
	PROFILEINFO tempprofile = {0};
	PROCESS_INFORMATION procinfo = {0};
	DWORD exitcode = 0;
	DWORD winerror = 0;

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
	int x, x2;
	for (x = 1; x < argc; x++)
	{
		if (!_tcsicmp(argv[x], _T("/?")))
		{
			x = argc;

			break;
		}
		else if (!_tcsicmp(argv[x], _T("/v")))  verbose = true;
		else if (!_tcsicmp(argv[x], _T("/w")))  wait = true;
		else if (!_tcsncicmp(argv[x], _T("/w="), 3))
		{
			wait = true;
			waitamount = _tstoi(argv[x] + 3);
		}
		else if (!_tcsncicmp(argv[x], _T("/pid="), 5))  pidfile = argv[x] + 5;
		else if (!_tcsicmp(argv[x], _T("/term")))  terminate = true;
		else if (!_tcsicmp(argv[x], _T("/runelevated")))
		{
			// Send this process' command-line and environment variables to an elevated process.
			x2 = IsElevatedProcess();
			if (x2 < 0)  return 1;

			if (x2 == 0)
			{
				// Set up an event object to wait for the timeout period.
				CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
				HANDLE tempevent = INVALID_HANDLE_VALUE;

				HANDLE commpipeevent = CreateCommPipeConnectEventObj(::GetCurrentProcessId());
				if (commpipeevent == NULL)
				{
					DumpErrorMsg("Unable to create pipe server event object.", "create_event_failed", ::GetLastError());

					return false;
				}

				if (wait)
				{
					TempVar.SetStr(L"Global\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_CommEv2_");
					TempVar.AppendUInt(::GetCurrentProcessId());

					tempevent = ::CreateEventW(NULL, FALSE, FALSE, TempVar.GetStr());
					if (tempevent == NULL)
					{
						DumpErrorMsg("Unable to create notification event object.", "create_event_failed", ::GetLastError());

						return false;
					}
				}

				// Start the named pipe server.
				HANDLE commpipehandle = StartMainCommPipeServer();
				if (commpipehandle == INVALID_HANDLE_VALUE)
				{
					DumpErrorMsg("Unable to create the main communication pipe.", "start_main_comm_pipe_failed", ::GetLastError());

					return 1;
				}

				// Start an elevated process.
				if (!ElevateSelf(false))  return 1;

				// Wait for the client to connect.
#ifdef _DEBUG
				if (!WaitForValidCommPipeClient(commpipehandle, commpipeevent, INFINITE))  return 1;
#else
				if (!WaitForValidCommPipeClient(commpipehandle, commpipeevent, 10000))  return 1;
#endif

				// Process the status.
				if (!WaitForCommPipeStatus(commpipehandle))  return 1;

				DWORD tempsize;

				// Send command-line options.
				tempsize = sizeof(WCHAR);
				for (x2 = 0; x2 < argc; x2++)
				{
					if (x2 != x)
					{
						TempVar.SetStr(argv[x2]);
						tempsize += (TempVar.GetSize() + 1) * sizeof(WCHAR);
					}
				}

				::WriteFile(commpipehandle, &tempsize, 4, NULL, NULL);

				for (x2 = 0; x2 < argc; x2++)
				{
					if (x2 != x)
					{
						TempVar.SetStr(argv[x2]);
						tempsize = (TempVar.GetSize() + 1) * sizeof(WCHAR);
						::WriteFile(commpipehandle, TempVar.GetStr(), tempsize, NULL, NULL);
					}
				}

				::WriteFile(commpipehandle, L"\0", sizeof(WCHAR), NULL, NULL);

				// Send environment.
				tempsize = sizeof(WCHAR);
				for (x2 = 0; envp[x2]; x2++)
				{
					TempVar.SetStr(envp[x2]);
					tempsize += (TempVar.GetSize() + 1) * sizeof(WCHAR);
				}

				::WriteFile(commpipehandle, &tempsize, 4, NULL, NULL);
				for (x2 = 0; envp[x2]; x2++)
				{
					TempVar.SetStr(envp[x2]);
					tempsize = (TempVar.GetSize() + 1) * sizeof(WCHAR);
					::WriteFile(commpipehandle, TempVar.GetStr(), tempsize, NULL, NULL);
				}

				::WriteFile(commpipehandle, L"\0", sizeof(WCHAR), NULL, NULL);

				// Get the status/result.
				if (!WaitForCommPipeStatus(commpipehandle))  return 1;

				int retval = 0;

				// Wait for the event object to fire.
				if (wait)
				{
					if (::WaitForSingleObject(tempevent, waitamount) != WAIT_OBJECT_0)
					{
						DumpErrorMsg("The elevated process did not respond before the timeout expired.", "timeout", ::GetLastError());

						::CloseHandle(tempevent);

						return false;
					}

					::CloseHandle(tempevent);

					// Get the status/result.
					if (!WaitForCommPipeStatus(commpipehandle))  return 1;

					// Receive the return value.
					if (!ReadFileExact(commpipehandle, &retval, sizeof(retval)))
					{
						DumpErrorMsg("Unable to read return value from the main communication pipe.", "read_main_comm_pipe_failed", ::GetLastError());

						return 1;
					}
				}

				::CloseHandle(commpipehandle);

				return retval;
			}
		}
		else if (!_tcsncicmp(argv[x], _T("/runelevated="), 13))
		{
			// Should only be called by ElevateSelf(false).
			GxConnectPipePID = _tstoi(argv[x] + 13);

			// A quick debugger hook.
#ifdef _DEBUG
			while (!::IsDebuggerPresent())  ::Sleep(500);
			::Sleep(2000);
#endif

			x2 = IsSystemProcess();
			if (x2 < 0)  return 1;

			if (x2 == 1)
			{
				DumpErrorMsg("Process is running as SYSTEM.  Expected elevation only.", "elevation_check_failed", ::GetLastError());

				return 1;
			}

			x2 = IsElevatedProcess();
			if (x2 < 0)  return 1;

			if (x2 == 0)
			{
				DumpErrorMsg("Process is not elevated.  Expected elevation.", "elevation_check_failed", ::GetLastError());

				return 1;
			}

			if (!ConnectToMainCommPipe())
			{
				DumpErrorMsg("Unable to connect to the main communication pipe.", "connect_to_main_comm_pipe_failed", ::GetLastError());

				return 1;
			}

			CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;
			HANDLE tempevent;

			TempVar.SetStr(L"Global\\CreateProcess_21F9597D-1A55-41AD-BCE0-0DB5CA53BDF8_CommEv2_");
			TempVar.AppendUInt(GxConnectPipePID);

			tempevent = ::CreateEventW(NULL, FALSE, FALSE, TempVar.GetStr());
			if (tempevent == NULL)
			{
				DumpErrorMsg("Unable to create notification event object.", "create_event_failed", ::GetLastError());

				return 1;
			}

			// Notify that all is well so far.
			::WriteFile(GxMainCommPipeClient, "\x01", 1, NULL, NULL);

			// Get the command-line and environment.
			int argc2 = 0, envc2 = 0;
			WCHAR *argvbuffer, *envpbuffer;
			WCHAR **argv2, **envp2;

			if (!ReadCommPipeStringArray(GxMainCommPipeClient, argvbuffer, argv2, argc2))  return 1;
			if (!ReadCommPipeStringArray(GxMainCommPipeClient, envpbuffer, envp2, envc2))  return 1;

			// Notify that all is well so far.
			::WriteFile(GxMainCommPipeClient, "\x01", 1, NULL, NULL);

			// Run this function again with the new information.
			int retval = _tmain(argc2, argv2, envp2);

			// Trigger the notification event that this process is done.
			if (!::SetEvent(tempevent))
			{
				DumpErrorMsg("Unable to trigger the notification event.", "set_event_failed", ::GetLastError());

				::CloseHandle(tempevent);

				return 1;
			}

			::CloseHandle(tempevent);

			// Notify that the process is done and send the return value.
			if (GxMainCommPipeClient != INVALID_HANDLE_VALUE)
			{
				::WriteFile(GxMainCommPipeClient, "\x01", 1, NULL, NULL);
				::WriteFile(GxMainCommPipeClient, &retval, sizeof(retval), NULL, NULL);

				DisconnectMainCommPipe();
			}

			FreeCommPipeStringArray(argvbuffer, argv2);
			FreeCommPipeStringArray(envpbuffer, envp2);

			return retval;
		}
		else if (!_tcsicmp(argv[x], _T("/systemtoken")))  assigntoken = 0;
		else if (!_tcsicmp(argv[x], _T("/elevatedtoken")))  assigntoken = 1;
		else if (!_tcsncicmp(argv[x], _T("/usetoken="), 10))
		{
			assigntoken = 2;
			tokenopts = argv[x] + 10;
		}
		else if (!_tcsncicmp(argv[x], _T("/createtoken="), 13))
		{
			assigntoken = 3;
			tokenopts = argv[x] + 13;
		}
		else if (!_tcsicmp(argv[x], _T("/mergeenv")))  mergeenv = true;
		else if (!_tcsncicmp(argv[x], _T("/connectpipe="), 13))  GxConnectPipePID = _tstoi(argv[x] + 13);
		else if (!_tcsicmp(argv[x], _T("/createservice")))
		{
			// Should only be called by ElevateSelf(true).
			if (!GxConnectPipePID)
			{
				DumpErrorMsg("Missing expected /connectpipe.", "pipe_check_failed", ::GetLastError());

				return 1;
			}

			x2 = IsSystemProcess();
			if (x2 < 0)  return 1;

			if (x2 == 1)
			{
				DumpErrorMsg("Process is running as SYSTEM.  Expected elevation only.", "elevation_check_failed", ::GetLastError());

				return 1;
			}

			x2 = IsElevatedProcess();
			if (x2 < 0)  return 1;

			if (x2 == 0)
			{
				DumpErrorMsg("Process is not elevated.  Expected elevation.", "elevation_check_failed", ::GetLastError());

				return 1;
			}

			if (!SystemSelf(GxConnectPipePID))  return 1;

			return 0;
		}
		else if (!_tcsncicmp(argv[x], _T("/runservice="), 12))
		{
			// Called by SystemSelf().
			GxService.MxServiceStatus = {0};
			GxService.MxStatusHandle = NULL;
			GxService.MxCreatorPID = _tstoi(argv[x] + 12);

			CubicleSoft::StaticWCMixedVar<WCHAR[256]> TempVar;

			TempVar.SetStr(L"z_createprocess-");
			TempVar.AppendUInt(GxService.MxCreatorPID);

			GxService.MxServiceName = TempVar.GetStr();
			GxService.MxExitCode = 0;

			// Start the NT service.
			SERVICE_TABLE_ENTRY servicetable[] = {
				{GxService.MxServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
				{NULL, NULL}
			};

			if (!::StartServiceCtrlDispatcher(servicetable))
			{
				DumpErrorMsg("Error starting service.", "start_service_ctrl_dispatcher_failed", ::GetLastError());

				GxService.MxExitCode = 1;
			}

			return GxService.MxExitCode;
		}
		else if (!_tcsncicmp(argv[x], _T("/mutex="), 7))
		{
			if (mutexhandle != NULL)  ::CloseHandle(mutexhandle);

			mutexhandle = ::CreateMutex(NULL, FALSE, argv[x] + 7);
			if (mutexhandle == NULL)
			{
				DumpErrorMsg("Unable to create mutex.", "create_mutex_failed", ::GetLastError());

				return 1;
			}
		}
		else if (!_tcsicmp(argv[x], _T("/singleton")))
		{
			if (mutexhandle == NULL)
			{
				DumpErrorMsg("The /mutex option was not used.", "mutex_check_failed", ::GetLastError());

				return 1;
			}

			if (::WaitForSingleObject(mutexhandle, 0) != WAIT_OBJECT_0)
			{
				DumpErrorMsg("The mutex is in use.", "mutex_in_use", 0);

				return 1;
			}
		}
		else if (!_tcsncicmp(argv[x], _T("/singleton="), 11))
		{
			if (mutexhandle == NULL)
			{
				DumpErrorMsg("The /mutex option was not used.", "mutex_check_failed", ::GetLastError());

				return 1;
			}

			DWORD temptimeout = (DWORD)_tstoi(argv[x] + 11);

			if (::WaitForSingleObject(mutexhandle, temptimeout) != WAIT_OBJECT_0)
			{
				DumpErrorMsg("The mutex is in use.", "mutex_in_use", 0);

				return 1;
			}
		}
		else if (!_tcsncicmp(argv[x], _T("/semaphore="), 11))
		{
			if (semaphorehandle != NULL)  ::CloseHandle(semaphorehandle);

			for (x2 = 7; argv[x][x2] && argv[x][x2] != _T(','); x2++);
			if (!argv[x][x2])
			{
				DumpErrorMsg("Invalid /semaphore option.", "invalid_semaphore_option", 0);
				DumpSyntax(argv[0]);

				return 1;
			}

			LONG maxcount = (LONG)_tstoi(argv[x] + 11);

			semaphorehandle = ::CreateSemaphoreW(NULL, maxcount, maxcount, argv[x] + x2 + 1);
			if (semaphorehandle == NULL)
			{
				DumpErrorMsg("Unable to create semaphore.", "create_semaphore_failed", ::GetLastError());

				return 1;
			}
		}
		else if (!_tcsicmp(argv[x], _T("/multiton")))
		{
			if (semaphorehandle == NULL)
			{
				DumpErrorMsg("The /semaphore option was not used.", "semaphore_check_failed", ::GetLastError());

				return 1;
			}

			if (::WaitForSingleObject(semaphorehandle, 0) != WAIT_OBJECT_0)
			{
				DumpErrorMsg("The semaphore is in use.", "semaphore_in_use", 0);

				return 1;
			}
		}
		else if (!_tcsncicmp(argv[x], _T("/multiton="), 10))
		{
			if (semaphorehandle == NULL)
			{
				DumpErrorMsg("The /semaphore option was not used.", "semaphore_check_failed", ::GetLastError());

				return 1;
			}

			DWORD temptimeout = (DWORD)_tstoi(argv[x] + 11);

			if (::WaitForSingleObject(semaphorehandle, temptimeout) != WAIT_OBJECT_0)
			{
				DumpErrorMsg("The semaphore is in use.", "semaphore_in_use", 0);

				return 1;
			}
		}
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
			stdinstr = (TCHAR *)_T(":hotkey");

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
				stdinstr = (TCHAR *)_T(":null");
			}
			else if (!_tcsicmp(argv[x] + 7, _T("socket")))
			{
				Gx_stdinsocket = ConnectSocketHandle(socketip, socketport, '\x00', sockettoken, sockettokenlen);
				temphandle = NULL;

				if (Gx_stdinsocket != INVALID_SOCKET && stdinread == NULL && Gx_stdinwritehandle == NULL && ::CreatePipe(&stdinread, &temphandle, &secattr, 0))
				{
					wait = true;

					::SetHandleInformation(temphandle, HANDLE_FLAG_INHERIT, 0);
					Gx_stdinwritehandle = temphandle;

					startinfo.hStdInput = stdinread;
					stdinstr = (TCHAR *)_T(":socket");
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
				stdoutstr = (TCHAR *)_T(":null");
			}
			else if (!_tcsicmp(argv[x] + 8, _T("socket")))
			{
				Gx_stdoutsocket = ConnectSocketHandle(socketip, socketport, '\x01', sockettoken, sockettokenlen);
				temphandle = NULL;

				if (Gx_stdoutsocket != INVALID_SOCKET && stdoutwrite == NULL && Gx_stdoutreadhandle == NULL && ::CreatePipe(&temphandle, &stdoutwrite, &secattr, 0))
				{
					wait = true;

					::SetHandleInformation(temphandle, HANDLE_FLAG_INHERIT, 0);
					Gx_stdoutreadhandle = temphandle;

					startinfo.hStdOutput = stdoutwrite;
					stdoutstr = (TCHAR *)_T(":socket");
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
				stderrstr = (TCHAR *)_T(":null");
			}
			else if (!_tcsicmp(argv[x] + 8, _T("socket")))
			{
				Gx_stderrsocket = ConnectSocketHandle(socketip, socketport, '\x02', sockettoken, sockettokenlen);
				temphandle = NULL;

				if (Gx_stderrsocket != INVALID_SOCKET && stderrwrite == NULL && Gx_stderrreadhandle == NULL && ::CreatePipe(&temphandle, &stderrwrite, &secattr, 0))
				{
					wait = true;

					::SetHandleInformation(temphandle, HANDLE_FLAG_INHERIT, 0);
					Gx_stderrreadhandle = temphandle;

					startinfo.hStdError = stderrwrite;
					stderrstr = (TCHAR *)_T(":socket");
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

			stdinstr = (TCHAR *)_T(":stdin");
			stdoutstr = (TCHAR *)_T(":stdout");
			stderrstr = (TCHAR *)_T(":stderr");

			startinfo.dwFlags &= ~STARTF_USEHOTKEY;
			startinfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		else if (!_tcsncicmp(argv[x], _T("/attach="), 8))
		{
			::CloseHandle(startinfo.hStdInput);
			::CloseHandle(startinfo.hStdOutput);
			::CloseHandle(startinfo.hStdError);
			::FreeConsole();

			DWORD pid = _tstoi(argv[x] + 8);

			// A quick debugger hook.
#ifdef _DEBUG
			while (!::IsDebuggerPresent())  ::Sleep(500);
			::Sleep(2000);
#endif

			if (::AttachConsole(pid))
			{
				// Reset handles.
				startinfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
				startinfo.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
				startinfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);

				stdinstr = (TCHAR *)_T(":stdin");
				stdoutstr = (TCHAR *)_T(":stdout");
				stderrstr = (TCHAR *)_T(":stderr");

				// Reset C library stdin/stdout/stderr.
				if (startinfo.hStdInput != INVALID_HANDLE_VALUE)
				{
					freopen("CONIN$", "r", stdin);
					setvbuf(stdin, NULL, _IONBF, 0);
				}

				if (startinfo.hStdOutput != INVALID_HANDLE_VALUE)
				{
					freopen("CONOUT$", "w", stdout);
					setvbuf(stdout, NULL, _IONBF, 0);
				}

				if (startinfo.hStdError != INVALID_HANDLE_VALUE)
				{
					freopen("CONOUT$", "w", stderr);
					setvbuf(stderr, NULL, _IONBF, 0);
				}

				startinfo.dwFlags &= ~STARTF_USEHOTKEY;
				startinfo.dwFlags |= STARTF_USESTDHANDLES;
			}
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
		if (!z3 || z2 < z3)
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
		stdinstr = (TCHAR *)_T(":null");
	}
	if (startinfo.hStdOutput == INVALID_HANDLE_VALUE)
	{
		startinfo.hStdOutput = NULL;
		stdoutstr = (TCHAR *)_T(":null");
	}
	if (startinfo.hStdError == INVALID_HANDLE_VALUE)
	{
		startinfo.hStdError = NULL;
		stderrstr = (TCHAR *)_T(":null");
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
		if (assigntoken == 0)  _tprintf(_T("CreateProcessAsSYSTEM(\n"));
		else if (assigntoken == 1)  _tprintf(_T("CreateProcessAsElevated(\n"));
		else if (assigntoken == 2)  _tprintf(_T("CreateProcessWithCopiedToken(\n\ttokenopts = %s\n"), tokenopts);
		else if (assigntoken == 3)  _tprintf(_T("CreateProcessWithCustomToken(\n\ttokenopts = %s\n"), tokenopts);
		else if (GxConnectPipePID)  _tprintf(_T("CreateProcessElevated(\n"));
		else  _tprintf(_T("CreateProcess(\n"));
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

	// Prepare to assign a user token to the child process.
	HANDLE commpipehandle = INVALID_HANDLE_VALUE;
	HANDLE maintoken = INVALID_HANDLE_VALUE;
	int envc2 = 0;
	WCHAR *envpbuffer = NULL;
	WCHAR **envp2 = NULL;
	if (assigntoken > -1)
	{
		// The process must be started suspended in order for the specific user token to be assigned.
		// After the process resumes, the process token is locked and can't be changed.
		createflags |= CREATE_SUSPENDED;

		// If this is already SYSTEM, then handle directly.
		x2 = IsSystemProcess();
		if (x2 < 0)  return 1;

		if (x2 == 1)
		{
			// A quick debugger hook.
#ifdef _DEBUG
			while (!::IsDebuggerPresent())  ::Sleep(500);
			::Sleep(2000);
#endif

			// Enable necessary privileges.
			if (!EnableSystemPrivileges())  return 1;

			// Load the primary token.  Requires SeDebugPrivilege.
			// The undocumented NtCreateToken() API requires SeCreateTokenPrivilege.
			if (assigntoken == 0 || assigntoken == 1)  maintoken = GetTokenFromPID(::GetCurrentProcessId(), TokenPrimary);
			else if (assigntoken == 2)  maintoken = FindExistingTokenFromOpts(tokenopts, TokenPrimary);
			else if (assigntoken == 3)  maintoken = CreateTokenFromOpts(tokenopts, true);

			if (maintoken == INVALID_HANDLE_VALUE)  return 1;

			// Load the user profile associated with the token.
			if (!LoadRealUserProfile(maintoken, tempprofile))
			{
				::CloseHandle(maintoken);

				return 1;
			}

			// Unfortunately, since we are now responsible for refcounting the user profile within this process,
			// we may need to leak the profile handle if the process is being started/run in the background.
			// There are no workarounds.  User profile management should be done within the target process.  Way to go, Microsoft!
			if (!wait || (waitamount != INFINITE && !terminate))  tempprofile.hProfile = NULL;

			// Generate an environment block for the user.
			LPVOID tempenv = NULL;

			if (!::CreateEnvironmentBlock(&tempenv, maintoken, FALSE))
			{
				DumpErrorMsg("Creating the environment block for the user token failed.", "create_environment_block_failed", ::GetLastError());

				if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

				::CloseHandle(maintoken);

				return 1;
			}

			// Convert to envp2.
			envpbuffer = (WCHAR *)tempenv;
			for (x2 = 0; envpbuffer[x2]; x2++)
			{
				while (envpbuffer[x2])  x2++;
			}

			envpbuffer = (WCHAR *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, (x2 + 1) * sizeof(WCHAR));
			if (envpbuffer == NULL)
			{
				DumpErrorMsg("Pointer buffer allocation failed.", "heap_alloc_failed", ::GetLastError());

				::DestroyEnvironmentBlock(tempenv);

				if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

				::CloseHandle(maintoken);

				return 1;
			}

			memcpy(envpbuffer, tempenv, (x2 + 1) * sizeof(WCHAR));

			::DestroyEnvironmentBlock(tempenv);

			if (!MakeStringArrayFromString(envpbuffer, x2 + 1, envp2, envc2))  return 1;
		}
		else
		{
			// Start the named pipe server.
			commpipehandle = StartMainCommPipeServer();
			if (commpipehandle == INVALID_HANDLE_VALUE)
			{
				DumpErrorMsg("Unable to create the main communication pipe.", "start_main_comm_pipe_failed", ::GetLastError());

				return 1;
			}

			HANDLE commpipeevent = CreateCommPipeConnectEventObj(::GetCurrentProcessId());
			if (commpipeevent == NULL)
			{
				DumpErrorMsg("Unable to create pipe server event object.", "create_event_failed", ::GetLastError());

				return 1;
			}

			// Elevate (if not already) and create the system service.
			x2 = IsElevatedProcess();
			if (x2 < 0)  return 1;

			if (x2 == 0)
			{
				if (!ElevateSelf(true))  return 1;
			}
			else
			{
				if (!SystemSelf(::GetCurrentProcessId()))  return 1;
			}

			// Wait for the client to connect.
#ifdef _DEBUG
			if (!WaitForValidCommPipeClient(commpipehandle, commpipeevent, INFINITE))  return 1;
#else
			if (!WaitForValidCommPipeClient(commpipehandle, commpipeevent, 35000))  return 1;
#endif

			// Wait for the system service to initialize.
			if (!WaitForCommPipeStatus(commpipehandle))  return 1;

			// Send the token type.
			::WriteFile(commpipehandle, &assigntoken, 1, NULL, NULL);

			// Send token options (if any).
			if (assigntoken == 2 || assigntoken == 3)
			{
				CubicleSoft::StaticWCMixedVar<WCHAR[8192]> TempVar;
				DWORD tempsize;

				TempVar.SetStr(tokenopts);
				tempsize = (TempVar.GetSize() + 1) * sizeof(WCHAR);
				::WriteFile(commpipehandle, &tempsize, 4, NULL, NULL);
				::WriteFile(commpipehandle, TempVar.GetStr(), tempsize, NULL, NULL);
			}

			// Wait for the system service to select or create the token, load the user profile, and get an environment for the user.
			if (!WaitForCommPipeStatus(commpipehandle))  return 1;

			// Get the environment.
			if (!ReadCommPipeStringArray(commpipehandle, envpbuffer, envp2, envc2))  return 1;
		}

		// Merge the current environment with the user environment.
		// User environment variables take precedence.
		if (mergeenv)
		{
			CubicleSoft::PackedOrderedHash<WCHAR *> TempHash(32);
			CubicleSoft::PackedOrderedHashNode<WCHAR *> *HashNode;

			// Copy local process variables.
			for (x = 0; envp[x]; x++)
			{
				for (x2 = 0; envp[x][x2] && envp[x][x2] != L'='; x2++);

				// Keep special cmd.exe pseudo-variables (current directory path per drive).
				if (!x2 && envp[x][0] == L'=' && envp[x][1] >= L'A' && envp[x][1] <= L'Z' && envp[x][2] == L':' && envp[x][3] == L'=')  x2 = 3;

				TempHash.Set((char *)envp[x], x2 * sizeof(WCHAR), envp[x] + x2);
			}

			// Copy user variables.  Replaces existing values.
			for (x = 0; envp2[x]; x++)
			{
				for (x2 = 0; envp2[x][x2] && envp2[x][x2] != L'='; x2++);

				// Keep special cmd.exe pseudo-variables (current directory path per drive).
				if (!x2 && envp2[x][0] == L'=' && envp2[x][1] >= L'A' && envp2[x][1] <= L'Z' && envp2[x][2] == L':' && envp2[x][3] == L'=')  x2 = 3;

				TempHash.Set((char *)envp2[x], x2 * sizeof(WCHAR), envp2[x] + x2);
			}

			// Calculate the size of the buffer needed.
			DWORD tempsize = sizeof(WCHAR);
			size_t hashpos = TempHash.GetNextPos();
			while ((HashNode = TempHash.Next(hashpos)) != NULL)
			{
				tempsize += HashNode->GetStrLen() + ((wcslen(HashNode->Value) + 1) * sizeof(WCHAR));
			}

			WCHAR *tempenvpbuffer;

			tempenvpbuffer = (WCHAR *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, tempsize);
			if (tempenvpbuffer == NULL)
			{
				DumpErrorMsg("Pointer buffer allocation failed.", "heap_alloc_failed", ::GetLastError());

				if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

				::CloseHandle(maintoken);

				return 1;
			}

			// Copy the variables to the buffer.
			x = 0;
			hashpos = TempHash.GetNextPos();
			while ((HashNode = TempHash.Next(hashpos)) != NULL)
			{
				x2 = (int)HashNode->GetStrLen();
				memcpy(tempenvpbuffer + (x / sizeof(WCHAR)), HashNode->GetStrKey(), x2);
				x += x2;

				x2 = (int)((wcslen(HashNode->Value) + 1) * sizeof(WCHAR));
				memcpy(tempenvpbuffer + (x / sizeof(WCHAR)), HashNode->Value, x2);
				x += x2;
			}

			// Replace envp2.
			FreeCommPipeStringArray(envpbuffer, envp2);

			envpbuffer = tempenvpbuffer;
			if (!MakeStringArrayFromString(envpbuffer, tempsize / sizeof(WCHAR), envp2, envc2))  return 1;
		}
	}

	// Execute CreateProcess().
	BOOL result = ::CreateProcess(appname, commandline, &secattr, &secattr, (startinfo.dwFlags & STARTF_USESTDHANDLES ? TRUE : FALSE), createflags, (envp2 != NULL ? *envp2 : *envp), startdir, &startinfo, &procinfo);

	FreeCommPipeStringArray(envpbuffer, envp2);

	if (!result)
	{
		winerror = ::GetLastError();

		if (winerror == ERROR_ELEVATION_REQUIRED)  _tprintf(_T("'%s' must be run as Administrator.\n\n"), appname);

		_tprintf(_T("lpApplicationName = %s\n"), appname);
		_tprintf(_T("lpCommandLine = %s\n\n"), commandline);

		DumpErrorMsg("An error occurred while attempting to start the process.", "create_process_failed", winerror);

		exitcode = 1;
	}
	else
	{
		// Assign user token.
		if (assigntoken > -1)
		{
			// This process only has a token if it is running as SYSTEM.
			if (maintoken != INVALID_HANDLE_VALUE)
			{
				// Assign the primary token to the process.
				if (!AssignPrimaryTokenToProcess(procinfo.hProcess, maintoken))
				{
					::CloseHandle(procinfo.hThread);

					::TerminateProcess(procinfo.hProcess, 1);
					::CloseHandle(procinfo.hProcess);

					// Unload the user profile.
					if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

					::CloseHandle(maintoken);

					return 1;
				}
			}
			else
			{
				// Send the system service the process ID of the newly created, suspended process.
				::WriteFile(commpipehandle, &procinfo.dwProcessId, 4, NULL, NULL);

				// Wait for the system service to assign the token to the process.
				if (!WaitForCommPipeStatus(commpipehandle))
				{
					::CloseHandle(procinfo.hThread);

					::TerminateProcess(procinfo.hProcess, 1);
					::CloseHandle(procinfo.hProcess);

					return 1;
				}

				::CloseHandle(commpipehandle);
			}

			// Start the main thread.
			::ResumeThread(procinfo.hThread);
		}

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

			// If socket handles are used, start relevant threads to pass the data around.
			if (Gx_stdinsocket != INVALID_SOCKET && stdinread != NULL)
			{
				::CloseHandle(stdinread);
				stdinread = NULL;

				stdinthread = ::CreateThread(NULL, 0, StdinSocketHandler, &startinfo, 0, NULL);

				if (stdinthread == NULL)
				{
#ifdef SUBSYSTEM_WINDOWS
					InitVerboseMode();
#endif

					_tprintf(_T("The 'stdin' socket handler thread failed to start.\n"));
				}
			}

			if (Gx_stdoutsocket != INVALID_SOCKET && stdoutwrite != NULL)
			{
				::CloseHandle(stdoutwrite);
				stdoutwrite = NULL;

				stdoutthread = ::CreateThread(NULL, 0, StdoutSocketHandler, &startinfo, 0, NULL);

				if (stdoutthread == NULL)
				{
#ifdef SUBSYSTEM_WINDOWS
					InitVerboseMode();
#endif

					_tprintf(_T("The 'stdout' socket handler thread failed to start.\n"));
				}
			}

			if (Gx_stderrsocket != INVALID_SOCKET && stderrwrite != NULL)
			{
				::CloseHandle(stderrwrite);
				stderrwrite = NULL;

				stderrthread = ::CreateThread(NULL, 0, StderrSocketHandler, &startinfo, 0, NULL);

				if (stderrthread == NULL)
				{
#ifdef SUBSYSTEM_WINDOWS
					InitVerboseMode();
#endif

					_tprintf(_T("The 'stderr' socket handler thread failed to start.\n"));
				}
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
			else
			{
				// The child process is still running but this process should exit.  Get all threads to terminate.
				if (Gx_stdinsocket != INVALID_SOCKET)  ::closesocket(Gx_stdinsocket);
				if (Gx_stdoutsocket != INVALID_SOCKET)  ::closesocket(Gx_stdoutsocket);
				if (Gx_stderrsocket != INVALID_SOCKET)  ::closesocket(Gx_stderrsocket);

				Gx_stdinsocket = INVALID_SOCKET;
				Gx_stdoutsocket = INVALID_SOCKET;
				Gx_stderrsocket = INVALID_SOCKET;

				if (Gx_stdinwritehandle != NULL)  ::CloseHandle(Gx_stdinwritehandle);
				if (Gx_stdoutreadhandle != NULL)  ::CloseHandle(Gx_stdoutreadhandle);
				if (Gx_stderrreadhandle != NULL)  ::CloseHandle(Gx_stderrreadhandle);

				Gx_stdinwritehandle = NULL;
				Gx_stdoutreadhandle = NULL;
				Gx_stderrreadhandle = NULL;
			}
		}

		::CloseHandle(procinfo.hProcess);
	}

	delete[] commandline;

	if (GxNetworkStarted)
	{
		// Wait for the threads to finish.
		if (stdinthread != NULL && ::WaitForSingleObject(stdinthread, 0) != WAIT_OBJECT_0)  ::TerminateThread(stdinthread, INFINITE);
		if (stdoutthread != NULL)  ::WaitForSingleObject(stdoutthread, INFINITE);
		if (stderrthread != NULL)  ::WaitForSingleObject(stderrthread, INFINITE);

		if (Gx_stdinsocket != INVALID_SOCKET)  ::closesocket(Gx_stdinsocket);
		if (Gx_stdoutsocket != INVALID_SOCKET)  ::closesocket(Gx_stdoutsocket);
		if (Gx_stderrsocket != INVALID_SOCKET)  ::closesocket(Gx_stderrsocket);

		::WSACleanup();
	}

	if (maintoken != INVALID_HANDLE_VALUE)
	{
		// Unload the user profile.
		if (tempprofile.hProfile != NULL)  ::UnloadUserProfile(maintoken, tempprofile.hProfile);

		::CloseHandle(maintoken);
	}

	if (mutexhandle != NULL)
	{
		::ReleaseMutex(mutexhandle);
		::CloseHandle(mutexhandle);
	}

	if (semaphorehandle != NULL)
	{
		::ReleaseSemaphore(semaphorehandle, 1, NULL);
		::CloseHandle(semaphorehandle);
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
	LPWSTR *args;
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
		::LocalFree(args);

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
	int argc, envc;
	TCHAR **argv, **envp;
	int result;

#ifdef UNICODE
	argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
#else
	argv = CommandLineToArgvA(lpCmdLine, &argc);
#endif

	if (argv == NULL)  return 1;

	LPWCH tempenv = ::GetEnvironmentStrings();
	int x2;

	// Convert to envp.
	WCHAR *envpbuffer = (WCHAR *)tempenv;
	for (x2 = 0; envpbuffer[x2]; x2++)
	{
		while (envpbuffer[x2])  x2++;
	}

	envpbuffer = (WCHAR *)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, (x2 + 1) * sizeof(WCHAR));
	if (envpbuffer == NULL)  return 1;

	memcpy(envpbuffer, tempenv, (x2 + 1) * sizeof(WCHAR));

	::FreeEnvironmentStrings(tempenv);

	if (!MakeStringArrayFromString(envpbuffer, x2 + 1, envp, envc))  return 1;

	result = _tmain(argc, argv, envp);

	::LocalFree(argv);

	FreeCommPipeStringArray(envpbuffer, envp);

	return result;
}
#endif
