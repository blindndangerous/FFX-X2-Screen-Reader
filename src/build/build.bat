@echo off
rem Compatibility entry point for the native build.

setlocal
if /I "%~1"=="--prototype" (
    echo The retired Frida prototype and its build script are available in Git history.
    exit /b 2
)
call "%~dp0..\proxy\build_dinput8.bat" %*
exit /b %ERRORLEVEL%
