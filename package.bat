@echo off
echo ========================================
echo         MindSploit 打包脚本
echo ========================================

REM 设置变量
set QTDIR=E:\Qt\6.9.1\mingw_64
set MINGW_DIR=E:\Qt\Tools\mingw1310_64
set PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%PATH%
set PACKAGE_DIR=MindSploit_Package
set VERSION=1.0.0

echo 正在准备打包环境...

REM 清理旧的打包目录
if exist %PACKAGE_DIR% rmdir /s /q %PACKAGE_DIR%
mkdir %PACKAGE_DIR%

REM 检查可执行文件是否存在
if not exist "release\MindSploit.exe" (
    echo 错误: 找不到可执行文件，请先构建项目
    echo 运行: build.bat
    pause
    exit /b 1
)

echo 正在复制可执行文件...
copy "release\MindSploit.exe" "%PACKAGE_DIR%\"

echo 正在复制Qt依赖库...
REM 使用windeployqt自动复制Qt依赖
windeployqt.exe --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%PACKAGE_DIR%\MindSploit.exe"

echo 正在复制MinGW运行时库...
REM 复制必要的MinGW运行时库
copy "%MINGW_DIR%\bin\libgcc_s_seh-1.dll" "%PACKAGE_DIR%\" 2>nul
copy "%MINGW_DIR%\bin\libstdc++-6.dll" "%PACKAGE_DIR%\" 2>nul
copy "%MINGW_DIR%\bin\libwinpthread-1.dll" "%PACKAGE_DIR%\" 2>nul

echo 正在创建目录结构...
mkdir "%PACKAGE_DIR%\docs"
mkdir "%PACKAGE_DIR%\tools"
mkdir "%PACKAGE_DIR%\config"

echo 正在复制文档和配置文件...
copy "README.md" "%PACKAGE_DIR%\docs\"
copy "LICENSE" "%PACKAGE_DIR%\docs\" 2>nul

echo 正在创建启动脚本...
echo @echo off > "%PACKAGE_DIR%\start.bat"
echo echo Starting MindSploit... >> "%PACKAGE_DIR%\start.bat"
echo MindSploit.exe >> "%PACKAGE_DIR%\start.bat"
echo pause >> "%PACKAGE_DIR%\start.bat"

echo 正在创建卸载脚本...
echo @echo off > "%PACKAGE_DIR%\uninstall.bat"
echo echo Uninstalling MindSploit... >> "%PACKAGE_DIR%\uninstall.bat"
echo rmdir /s /q "%%~dp0" >> "%PACKAGE_DIR%\uninstall.bat"

echo 正在创建版本信息文件...
echo MindSploit v%VERSION% > "%PACKAGE_DIR%\VERSION.txt"
echo Build Date: %date% %time% >> "%PACKAGE_DIR%\VERSION.txt"
echo Platform: Windows x64 >> "%PACKAGE_DIR%\VERSION.txt"

echo 正在创建压缩包...
if exist "MindSploit_v%VERSION%_Windows.zip" del "MindSploit_v%VERSION%_Windows.zip"
powershell -command "Compress-Archive -Path '%PACKAGE_DIR%\*' -DestinationPath 'MindSploit_v%VERSION%_Windows.zip'"

echo ========================================
echo 打包完成！
echo.
echo 输出文件:
echo - 文件夹: %PACKAGE_DIR%\
echo - 压缩包: MindSploit_v%VERSION%_Windows.zip
echo.
echo 可以直接分发压缩包或整个文件夹
echo ========================================
pause
