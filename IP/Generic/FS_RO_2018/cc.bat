@ECHO OFF
Echo CC.bat       Converting %3.%4
%1\Bin2C.exe %2\%3.%4 %3_2018
XCopy %3_2018.* Generated\%3_2018.* /Q/Y
del %3_2018.c
del %3_2018.h
