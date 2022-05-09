@ECHO OFF

SET CYGWIN_DIR=D:\cygwin\bin\

SET GCC_GNU_DIR=C:\gcc\bin\

SET PATH=%GCC_GNU_DIR%;%CYGWIN_DIR%;%PATH%

cmd /K cd %~dp0

