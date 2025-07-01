@echo off
echo ========================================
echo      MindSploit 模块功能测试指南
echo ========================================

echo.
echo 🎯 测试新的模块化功能：
echo.
echo 1. 基础命令测试：
echo    - help          显示帮助信息
echo    - banner        显示项目横幅
echo    - history       查看命令历史
echo    - projects      查看项目列表
echo.
echo 2. 内置端口扫描模块测试：
echo    - portscan 127.0.0.1
echo    - portscan 127.0.0.1 80 443
echo    - scan 127.0.0.1
echo.
echo 3. Nmap模块测试（需要安装nmap）：
echo    - nmap 127.0.0.1
echo    - nmap -sS 127.0.0.1
echo    - nmap -p 80,443 127.0.0.1
echo.
echo 4. Curl模块测试（需要安装curl）：
echo    - curl https://httpbin.org/get
echo    - curl -X POST https://httpbin.org/post
echo.
echo 5. Searchsploit模块测试（需要安装searchsploit）：
echo    - searchsploit apache
echo    - searchsploit CVE-2021-44228
echo.
echo 6. 界面功能测试：
echo    - 使用 ↑↓ 键浏览命令历史
echo    - 使用 Tab 键自动补全命令
echo    - 查看可视化区域的结果显示
echo    - 测试菜单栏功能
echo.
echo 🚀 启动 MindSploit 进行测试...
echo.

start "" "release\MindSploit.exe"

echo ✅ MindSploit 已启动！
echo.
echo 💡 测试提示：
echo - 所有命令都会自动保存到数据库
echo - 扫描结果会在可视化区域显示
echo - 模块会自动检查依赖工具是否可用
echo - 内置端口扫描器无需外部依赖
echo.
pause
