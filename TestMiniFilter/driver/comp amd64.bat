@echo off
set ddkpath=C:\WinDDK\7600.16385.1\bin\x86\amd64
set incpath=C:\WinDDK\7600.16385.1\inc
set libpath=C:\WinDDK\7600.16385.1\lib
set binpath=.\comp_win7fre\amd64
set objpath=.\comp_win7fre\amd64\Intermediate

echo ===========Start Generating===========
echo Project:HelloDrv

echo ===========Start  Compiling===========
%ddkpath%\cl.exe .\driver.c /I"%incpath%\api" /I"%incpath%\crt" /I"%incpath%\ddk" /Zi /nologo /W3 /WX /Od /Oy- /D"_AMD64_" /D"_WIN64" /D "_NDEBUG" /D"_UNICODE" /D "UNICODE" /Zc:wchar_t /Zc:forScope /Fa"%objpath%\driver.asm" /Fo"%objpath%\driver.obj" /Fd"%objpath%\vc90.pdb" /GS- /Gr /TC /c /ERRORREPORT:QUEUE

%ddkpath%\cl.exe .\MiniFilter.c /I"%incpath%\api" /I"%incpath%\crt" /I"%incpath%\ddk" /Zi /nologo /W3 /WX /Od /Oy- /D"_AMD64_" /D"_WIN64" /D "_NDEBUG" /D"_UNICODE" /D "UNICODE" /Zc:wchar_t /Zc:forScope /Fa"%objpath%\MiniFilter.asm" /Fo"%objpath%\MiniFilter.obj" /Fd"%objpath%\vc90.pdb" /GS- /Gr /TC /c /ERRORREPORT:QUEUE
echo Completed!

echo ============Start  Linking============
%ddkpath%\link.exe "%objpath%\driver.obj" "%objpath%\MiniFilter.obj" ".\version.res" /LIBPATH:"%libpath%\win7\amd64" /LIBPATH:"%libpath%\Crt\amd64" "ntoskrnl.lib" "hal.lib" "FltMgr.lib" /NOLOGO /DEBUG /PDB:"%objpath%\driver.pdb" /OUT:"%binpath%\driver.sys" /SUBSYSTEM:NATIVE /Driver /ENTRY:"DriverEntry" /Machine:AMD64 /ERRORREPORT:QUEUE
echo Completed!

pause