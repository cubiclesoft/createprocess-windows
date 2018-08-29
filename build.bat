@echo off
cls

cl /Ox createprocess.cpp -D_USING_V110_SDK71_ -DSUBSYSTEM_CONSOLE /link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /subsystem:console,5.01 ws2_32.lib /out:createprocess.exe
cl /Ox createprocess.cpp -D_USING_V110_SDK71_ -DSUBSYSTEM_WINDOWS /link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /subsystem:windows,5.01 ws2_32.lib shell32.lib /out:createprocess-win.exe
