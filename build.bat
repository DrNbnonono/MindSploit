@echo off
echo Building MindSploit...

REM Set Qt environment
set QTDIR=E:\Qt\6.9.1\mingw_64
set PATH=%QTDIR%\bin;E:\Qt\Tools\mingw1310_64\bin;%PATH%

REM Clean previous build
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
if exist Makefile del Makefile
if exist Makefile.Debug del Makefile.Debug
if exist Makefile.Release del Makefile.Release

REM Generate Makefile
echo Running qmake...
qmake.exe MindSploit.pro
if %errorlevel% neq 0 (
    echo Error: qmake failed
    pause
    exit /b 1
)

REM Build the project
echo Building project...
mingw32-make.exe
if %errorlevel% neq 0 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable: release\MindSploit.exe
pause
