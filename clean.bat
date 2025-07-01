@echo off
echo Cleaning build files...

REM Remove build directories
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
if exist build rmdir /s /q build

REM Remove Makefiles
if exist Makefile del Makefile
if exist Makefile.Debug del Makefile.Debug
if exist Makefile.Release del Makefile.Release

REM Remove Qt generated files
if exist .qmake.stash del .qmake.stash

echo Clean completed!
pause
