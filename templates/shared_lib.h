// Shared library (DLL, so, etc.) loader and function call templates.
// (C) 2021 CubicleSoft.  All Rights Reserved.

#ifndef CUBICLESOFT_SHARED_LIBRARY
#define CUBICLESOFT_SHARED_LIBRARY

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

namespace CubicleSoft
{
	namespace SharedLib
	{
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)

		class ModuleUtil
		{
		public:
			inline ModuleUtil(const char *Filename) : MxFilename(Filename), MxModulePtr(NULL)
			{
			}

			inline ~ModuleUtil()
			{
				if (MxModulePtr != NULL)  ::FreeLibrary(MxModulePtr);
			}

			inline HMODULE Load()
			{
				if (MxModulePtr != NULL)  return MxModulePtr;
				if (MxFilename == NULL)  return NULL;

				MxModulePtr = ::LoadLibraryA(MxFilename);

				MxFilename = NULL;

				return MxModulePtr;
			}

		private:
			const char *MxFilename;
			HMODULE MxModulePtr;
		};

		class FunctionUtil
		{
		public:
			inline FunctionUtil(ModuleUtil &ModuleInst, const char *FuncName) : MxModule(&ModuleInst), MxFuncName(FuncName), MxFuncPtr(NULL)
			{
			}

			inline FARPROC GetFuncPtr()  { return MxFuncPtr; }

			inline FARPROC LoadFuncPtr()
			{
				if (MxFuncName == NULL)  return NULL;

				const char *FuncName = MxFuncName;
				MxFuncName = NULL;

				HMODULE ModulePtr = MxModule->Load();

				return (ModulePtr == NULL ? NULL : (MxFuncPtr = ::GetProcAddress(ModulePtr, FuncName)));
			}

		private:
			ModuleUtil *MxModule;
			const char *MxFuncName;
			FARPROC MxFuncPtr;
		};

		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX  Call
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID  CallVoid
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE  CallOnce
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID  CallOnceVoid
		#define CUBICLESOFT_SHARED_LIBRARY_CALLCONV
		#include "shared_lib_util.h"
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_CALLCONV

		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX  Stdcall
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID  StdcallVoid
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE  StdcallOnce
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID  StdcallOnceVoid
		#define CUBICLESOFT_SHARED_LIBRARY_CALLCONV  __stdcall
		#include "shared_lib_util.h"
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_CALLCONV

		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX  Cdecl
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID  CdeclVoid
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE  CdeclOnce
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID  CdeclOnceVoid
		#define CUBICLESOFT_SHARED_LIBRARY_CALLCONV  __cdecl
		#include "shared_lib_util.h"
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_CALLCONV

		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX  Fastcall
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID  FastcallVoid
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE  FastcallOnce
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID  FastcallOnceVoid
		#define CUBICLESOFT_SHARED_LIBRARY_CALLCONV  __fastcall
		#include "shared_lib_util.h"
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_CALLCONV

#else

		class ModuleUtil
		{
		public:
			inline ModuleUtil(const char *Filename) : MxFilename(Filename), MxModulePtr(NULL)
			{
			}

			inline ~ModuleUtil()
			{
				if (MxModulePtr != NULL)  dlclose(MxModulePtr);
			}

			inline void *Load()
			{
				if (MxModulePtr != NULL)  return MxModulePtr;
				if (MxFilename == NULL)  return NULL;

				MxModulePtr = dlopen(MxFilename, RTLD_LAZY);

				MxFilename = NULL;

				return MxModulePtr;
			}

		private:
			const char *MxFilename;
			void *MxModulePtr;
		};

		class FunctionUtil
		{
		public:
			inline FunctionUtil(ModuleUtil &ModuleInst, const char *FuncName) : MxModule(&ModuleInst), MxFuncName(FuncName), MxFuncPtr(NULL)
			{
			}

			inline void *GetFuncPtr()  { return MxFuncPtr; }

			inline void *LoadFuncPtr()
			{
				if (MxFuncName == NULL)  return NULL;

				const char *FuncName = MxFuncName;
				MxFuncName = NULL;

				void *ModulePtr = MxModule->Load();

				return (ModulePtr == NULL ? NULL : (MxFuncPtr = dlsym(ModulePtr, FuncName)));
			}

		private:
			ModuleUtil *MxModule;
			const char *MxFuncName;
			void *MxFuncPtr;
		};

		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX  Call
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID  CallVoid
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE  CallOnce
		#define CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID  CallOnceVoid
		#define CUBICLESOFT_SHARED_LIBRARY_CALLCONV
		#include "shared_lib_util.h"
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE
		#undef CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID
		#undef CUBICLESOFT_SHARED_LIBRARY_CALLCONV

#endif
	}
}

#endif
