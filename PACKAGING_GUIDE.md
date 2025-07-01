# MindSploit 打包发布指南

## 📦 打包方案概述

MindSploit提供两种主要的分发方式：

1. **绿色版** - 免安装压缩包，解压即用
2. **安装版** - 标准Windows安装程序，集成系统

## 🚀 快速打包流程

### 步骤1: 构建项目

```bash
# 清理并构建发布版本
clean.bat
build.bat
```

### 步骤2: 创建绿色版

```bash
# 自动打包绿色版
package.bat
```

输出文件：
- `MindSploit_Package/` - 绿色版文件夹
- `MindSploit_v1.0.0_Windows.zip` - 绿色版压缩包

### 步骤3: 创建安装版（可选）

```bash
# 创建Windows安装程序
create_installer.bat
```

输出文件：
- `MindSploit_Setup_v1.0.0.exe` - Windows安装程序

## 📁 绿色版详细说明

### 优势
- ✅ 无需安装，解压即用
- ✅ 便携性强，可放在U盘
- ✅ 不修改系统注册表
- ✅ 卸载简单，删除文件夹即可

### 包含文件
```
MindSploit_Package/
├── MindSploit.exe          # 主程序
├── start.bat               # 启动脚本
├── uninstall.bat          # 卸载脚本
├── VERSION.txt             # 版本信息
├── *.dll                   # Qt和MinGW运行时库
├── platforms/              # Qt平台插件
├── styles/                 # Qt样式插件
├── docs/                   # 文档文件
│   └── README.md
├── tools/                  # 外部工具目录
└── config/                 # 配置文件目录
```

### 使用方法
1. 解压 `MindSploit_v1.0.0_Windows.zip`
2. 双击 `start.bat` 或 `MindSploit.exe` 启动
3. 首次运行会自动创建配置文件

## 🔧 安装版详细说明

### 优势
- ✅ 标准Windows安装体验
- ✅ 自动创建开始菜单和桌面快捷方式
- ✅ 集成到系统程序列表
- ✅ 支持静默安装

### 安装特性
- **安装位置**: `C:\Program Files\MindSploit\`
- **开始菜单**: 创建程序组和快捷方式
- **桌面快捷方式**: 可选创建
- **注册表**: 记录安装信息，支持控制面板卸载
- **卸载程序**: 完整清理所有文件和注册表项

### 静默安装
```bash
# 静默安装
MindSploit_Setup_v1.0.0.exe /S

# 静默卸载
"C:\Program Files\MindSploit\Uninstall.exe" /S
```

## 🛠️ 自定义打包

### 修改版本号

编辑以下文件中的版本信息：
- `package.bat` - 修改 `VERSION` 变量
- `installer.nsi` - 修改 `APP_VERSION` 定义

### 添加额外文件

在 `package.bat` 中添加复制命令：
```batch
echo 正在复制额外文件...
copy "extra_file.txt" "%PACKAGE_DIR%\"
mkdir "%PACKAGE_DIR%\extra_folder"
copy "extra_folder\*" "%PACKAGE_DIR%\extra_folder\"
```

### 自定义安装程序

编辑 `installer.nsi` 文件：
- 修改安装程序图标
- 添加自定义页面
- 修改安装选项
- 添加环境变量设置

## 📋 依赖库管理

### Qt库依赖

`windeployqt` 自动处理以下Qt库：
- Qt6Core.dll
- Qt6Gui.dll  
- Qt6Widgets.dll
- platforms/qwindows.dll
- styles/qwindowsvistastyle.dll

### MinGW运行时

手动复制的MinGW库：
- libgcc_s_seh-1.dll
- libstdc++-6.dll
- libwinpthread-1.dll

### 第三方工具

如需集成外部工具（如nmap），在打包脚本中添加：
```batch
echo 正在复制第三方工具...
mkdir "%PACKAGE_DIR%\tools\nmap"
copy "third_party\nmap\*" "%PACKAGE_DIR%\tools\nmap\"
```

## 🔍 测试打包结果

### 绿色版测试
1. 在干净的Windows系统上测试
2. 确保无Qt开发环境的机器能正常运行
3. 测试所有功能模块
4. 验证配置文件创建

### 安装版测试
1. 测试安装过程
2. 验证快捷方式创建
3. 测试程序功能
4. 验证卸载过程

### 自动化测试脚本

```batch
@echo off
echo 正在测试打包结果...

REM 测试绿色版
echo 测试绿色版...
cd MindSploit_Package
MindSploit.exe --version
if %errorlevel% neq 0 echo 绿色版测试失败

REM 测试安装版
echo 测试安装版...
MindSploit_Setup_v1.0.0.exe /S
timeout /t 10
"C:\Program Files\MindSploit\MindSploit.exe" --version
if %errorlevel% neq 0 echo 安装版测试失败

echo 测试完成
```

## 📤 发布流程

### 1. 版本准备
- [ ] 更新版本号
- [ ] 更新CHANGELOG
- [ ] 完成所有测试

### 2. 构建打包
- [ ] 执行 `build.bat`
- [ ] 执行 `package.bat`
- [ ] 执行 `create_installer.bat`

### 3. 质量检查
- [ ] 在多台机器测试
- [ ] 验证所有功能
- [ ] 检查文件完整性

### 4. 发布上传
- [ ] 上传到GitHub Releases
- [ ] 更新下载链接
- [ ] 发布更新公告

## 🚨 常见问题

### Q: windeployqt找不到
**A**: 确保Qt bin目录在PATH中，或使用完整路径：
```batch
E:\Qt\6.9.1\mingw_64\bin\windeployqt.exe
```

### Q: 缺少DLL文件
**A**: 使用Dependency Walker检查依赖，手动复制缺失的库文件

### Q: 安装程序创建失败
**A**: 
1. 检查NSIS是否正确安装
2. 确保所有文件路径正确
3. 检查installer.nsi语法

### Q: 程序在其他机器无法运行
**A**:
1. 检查Visual C++ Redistributable
2. 确保所有Qt插件已复制
3. 验证MinGW运行时库

## 📊 打包清单

发布前检查清单：

- [ ] 主程序可执行文件
- [ ] 所有Qt依赖库
- [ ] MinGW运行时库
- [ ] Qt平台插件
- [ ] 配置文件模板
- [ ] 文档文件
- [ ] 版本信息文件
- [ ] 启动脚本
- [ ] 卸载脚本
- [ ] 许可证文件

---

*遵循此指南可确保MindSploit的正确打包和分发*
