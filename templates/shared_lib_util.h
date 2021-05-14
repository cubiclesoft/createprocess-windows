// Shared library (DLL, so, etc.) loader and function call templates.
// (C) 2021 CubicleSoft.  All Rights Reserved.

// NOTE:  This file is intended to be included from 'shared_lib.h'.

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)

template<class RetType, class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX(FunctionUtil &Func, RetType &ReturnVal, Args... FuncArgs)
{
	FARPROC FuncAddressPtr;

	if ((FuncAddressPtr = Func.GetFuncPtr()) == NULL && (FuncAddressPtr = Func.LoadFuncPtr()) == NULL)  return false;

	ReturnVal = (*((RetType (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

template<class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID(FunctionUtil &Func, Args... FuncArgs)
{
	FARPROC FuncAddressPtr;

	if ((FuncAddressPtr = Func.GetFuncPtr()) == NULL || (FuncAddressPtr = Func.LoadFuncPtr()) == NULL)  return false;

	(*((void (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

// Doesn't call FreeLibrary() to avoid issues with multiple uses of the same DLL.
template<class RetType, class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE(const char *Filename, const char *FuncName, RetType &ReturnVal, Args... FuncArgs)
{
	HMODULE ModulePtr;
	FARPROC FuncAddressPtr;

	ModulePtr = ::LoadLibraryA(Filename);
	if (ModulePtr == NULL)  return false;

	FuncAddressPtr = ::GetProcAddress(ModulePtr, FuncName);
	if (FuncAddressPtr == NULL)  return false;

	ReturnVal = (*((RetType (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

// Doesn't call FreeLibrary() to avoid issues with multiple uses of the same DLL.
template<class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID(const char *Filename, const char *FuncName, Args... FuncArgs)
{
	HMODULE ModulePtr;
	FARPROC FuncAddressPtr;

	ModulePtr = ::LoadLibraryA(Filename);
	if (ModulePtr == NULL)  return false;

	FuncAddressPtr = ::GetProcAddress(ModulePtr, FuncName);
	if (FuncAddressPtr == NULL)  return false;

	(*((void (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

#else

template<class RetType, class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX(FunctionUtil &Func, RetType &ReturnVal, Args... FuncArgs)
{
	void *FuncAddressPtr;

	if ((FuncAddressPtr = Func.GetFuncPtr()) == NULL && (FuncAddressPtr = Func.LoadFuncPtr()) == NULL)  return false;

	ReturnVal = (*((RetType (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

template<class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX_VOID(FunctionUtil &Func, Args... FuncArgs)
{
	void *FuncAddressPtr;

	if ((FuncAddressPtr = Func.GetFuncPtr()) == NULL || (FuncAddressPtr = Func.LoadFuncPtr()) == NULL)  return false;

	(*((void (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

// Doesn't call dlclose() to avoid issues with multiple uses of the same library.
template<class RetType, class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE(const char *Filename, const char *FuncName, RetType &ReturnVal, Args... FuncArgs)
{
	void *ModulePtr;
	void *FuncAddressPtr;

	ModulePtr = dlopen(Filename, RTLD_LAZY);
	if (ModulePtr == NULL)  return false;

	FuncAddressPtr = dlsym(ModulePtr, FuncName);
	if (FuncAddressPtr == NULL)  return false;

	ReturnVal = (*((RetType (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

// Doesn't call dlclose() to avoid issues with multiple uses of the same library.
template<class... Args>
bool CUBICLESOFT_SHARED_LIBRARY_PREFIX_ONCE_VOID(const char *Filename, const char *FuncName, Args... FuncArgs)
{
	void *ModulePtr;
	void *FuncAddressPtr;

	ModulePtr = dlopen(Filename, RTLD_LAZY);
	if (ModulePtr == NULL)  return false;

	FuncAddressPtr = dlsym(ModulePtr, FuncName);
	if (FuncAddressPtr == NULL)  return false;

	(*((void (CUBICLESOFT_SHARED_LIBRARY_CALLCONV *)(Args...))FuncAddressPtr))(FuncArgs...);

	return true;
}

#endif
