@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo
if errorlevel 1 exit /b 1
cd /d C:\Users\god\Desktop\Onyx
cmake --build build\windows-debug -j 8
