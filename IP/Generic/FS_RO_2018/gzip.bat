@ECHO OFF
Echo gzip.bat       Compressing %3.%4

IF NOT EXIST %1\gzip.exe (
  ECHO %1\gzip.exe missing, please download it from http://gnuwin32.sourceforge.net/packages/gzip.htm
  PAUSE
  EXIT
)

%1\gzip.exe -k %2\%3.%4
ren %2\%3.%4.gz %3.gz 
IF EXIST %2\%3.%4.gz (
  del %2\%3.%4.gz
)