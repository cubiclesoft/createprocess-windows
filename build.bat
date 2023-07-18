@echo off

echo Compiling createprocess.exe...
cl ^
    /Ox ^
    createprocess.cpp ^
    templates/*.cpp ^
    -D_USING_V110_SDK71_ ^
    -DSUBSYSTEM_CONSOLE ^
    /link ^
    /FILEALIGN:512 ^
    /OPT:REF ^
    /OPT:ICF ^
    /INCREMENTAL:NO ^
    /subsystem:console,5.02 ^
    ws2_32.lib ^
    shell32.lib ^
    netapi32.lib ^
    advapi32.lib ^
    userenv.lib ^
    /out:createprocess.exe
if %errorlevel% neq 0 (
    echo Compilation failed. Exit code: %errorlevel%
    exit /b %errorlevel%
)

echo Showing the createprocess.exe dependencies...
dumpbin /DEPENDENTS createprocess.exe

echo Compiling createprocess-win.exe...
cl ^
    /Ox ^
    createprocess.cpp ^
    templates/*.cpp ^
    -D_USING_V110_SDK71_ ^
    -DSUBSYSTEM_WINDOWS ^
    /link ^
    /FILEALIGN:512 ^
    /OPT:REF ^
    /OPT:ICF ^
    /INCREMENTAL:NO ^
    /subsystem:windows,5.02 ^
    ws2_32.lib ^
    shell32.lib ^
    netapi32.lib ^
    advapi32.lib ^
    userenv.lib ^
    /out:createprocess-win.exe
if %errorlevel% neq 0 (
    echo Compilation failed. Exit code: %errorlevel%
    exit /b %errorlevel%
)

echo Showing the createprocess-win.exe dependencies...
dumpbin /DEPENDENTS createprocess-win.exe
