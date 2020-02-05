@echo off
set ddkpath=C:\WinDDK\7600.16385.1\bin\x86\x86
set incpath=C:\WinDDK\7600.16385.1\inc
set libpath=C:\WinDDK\7600.16385.1\lib
set binpath=.\comp_win7fre\intel_x86
set objpath=.\comp_win7fre\intel_x86\Intermediate

echo ===========Start Generating===========
echo Project:HelloDrv

echo ===========Start  Compiling===========
%ddkpath%\cl.exe .\driver.c /I"%incpath%\api" /I"%incpath%\crt" /I"%incpath%\ddk" /Zi /nologo /W3 /WX /Od /Oy- /D"_X86_" /D"_WIN32" /D "_NDEBUG" /D"_UNICODE" /D "UNICODE" /Zc:wchar_t /Zc:forScope /Fa"%objpath%\driver.asm" /Fo"%objpath%\driver.obj" /Fd"%objpath%\vc90.pdb" /GS- /Gz /TC /c /ERRORREPORT:QUEUE

%ddkpath%\cl.exe .\MiniFilter.c /I"%incpath%\api" /I"%incpath%\crt" /I"%incpath%\ddk" /Zi /nologo /W3 /WX /Od /Oy- /D"_X86_" /D"_WIN32" /D "_NDEBUG" /D"_UNICODE" /D "UNICODE" /Zc:wchar_t /Zc:forScope /Fa"%objpath%\MiniFilter.asm" /Fo"%objpath%\MiniFilter.obj" /Fd"%objpath%\vc90.pdb" /GS- /Gz /TC /c /ERRORREPORT:QUEUE
echo Completed!

echo ============Start  Linking============
%ddkpath%\link.exe "%objpath%\driver.obj" "%objpath%\MiniFilter.obj" ".\version.res" /LIBPATH:"%libpath%\win7\i386" /LIBPATH:"%libpath%\Crt\i386" "ntoskrnl.lib" "hal.lib" "FltMgr.lib" /NOLOGO /DEBUG /PDB:"%objpath%\driver.pdb" /OUT:"%binpath%\driver.sys" /SUBSYSTEM:NATIVE /Driver /ENTRY:"DriverEntry" /Machine:X86 /ERRORREPORT:QUEUE
echo Completed!

pause