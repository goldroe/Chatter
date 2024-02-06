@echo off

set includes=-Iext -Iext\SDL2\include -Iext\glad\include -IC:\vclib\imgui -IC:\vclib\imgui\backends
SET src=src\chatter.cpp ext\glad\src\glad.c

SET warning_flags=-W4 -wd4100 -wd4101 -wd4189 -wd4996 -wd4530 -wd4201 -wd4505
SET compiler_flags=-nologo -FC -Zi -MDd %warning_flags% %includes% /Fe:Chatter.exe

SET linker_libs=imgui.lib SDL2main.lib SDL2.lib opengl32.lib  shell32.lib user32.lib
SET linker_flags=-SUBSYSTEM:CONSOLE -opt:ref -incremental:no /LIBPATH:ext\ /LIBPATH:ext/SDL2/lib/x64/ -NODEFAULTLIB:LIBCMT %linker_libs%

CL %compiler_flags% %src% -link %linker_flags%

del *.obj > nul
