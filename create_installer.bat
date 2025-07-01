@echo off
echo ========================================
echo      MindSploit 安装包制作脚本
echo ========================================

REM 检查NSIS是否安装
where makensis >nul 2>nul
if %errorlevel% neq 0 (
    echo 错误: 未找到NSIS，请先安装NSIS
    echo 下载地址: https://nsis.sourceforge.io/Download
    echo 安装后请将NSIS添加到系统PATH环境变量
    pause
    exit /b 1
)

REM 检查打包文件是否存在
if not exist "MindSploit_Package" (
    echo 错误: 找不到打包文件夹，请先运行 package.bat
    pause
    exit /b 1
)

echo 正在创建安装包...
makensis installer.nsi

if %errorlevel% equ 0 (
    echo ========================================
    echo 安装包创建成功！
    echo 输出文件: MindSploit_Setup_v1.0.0.exe
    echo ========================================
) else (
    echo 错误: 安装包创建失败
)

pause
