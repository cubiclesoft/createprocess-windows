@echo off
cls

cl /Ox createprocess.cpp -D_USING_V110_SDK71_ /link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /subsystem:console,5.01 /out:createprocess.exe
