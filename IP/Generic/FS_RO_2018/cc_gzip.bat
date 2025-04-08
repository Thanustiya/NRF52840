@ECHO OFF
Echo CC.bat       Converting %2\%3.%4
%1\Bin2C.exe %2\%3.%4 %3_2018
XCopy %3_2018.* Generated\%3_2018.* /Y /Q
del %3_2018.c
del %3_2018.h
IF EXIST %2\%3.%4 (
  del %2\%3.%4
)

