@echo off
echo ========================================
echo         MindSploit 测试运行脚本
echo ========================================

REM 设置Qt环境
set QTDIR=E:\Qt\6.9.1\mingw_64
set PATH=%QTDIR%\bin;E:\Qt\Tools\mingw1310_64\bin;%PATH%

echo 正在启动 MindSploit...
echo.
echo 测试功能：
echo 1. 在终端中输入 'help' 查看帮助
echo 2. 输入 'banner' 显示横幅
echo 3. 输入 'nmap 192.168.1.1' 测试扫描
echo 4. 输入 'history' 查看历史记录
echo 5. 使用 ↑↓ 键浏览命令历史
echo 6. 测试菜单栏功能
echo.

start "" "release\MindSploit.exe"

echo MindSploit 已启动！
echo 如果程序无法启动，请检查Qt库是否正确安装。
pause
