@echo off
cls

cl /Ox createprocess.cpp /link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /out:createprocess.exe
