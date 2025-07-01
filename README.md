# MindSploit

<div align="center">

![MindSploit Logo](https://img.shields.io/badge/MindSploit-v1.0-blue?style=for-the-badge)
![Qt](https://img.shields.io/badge/Qt-6.9.1-green?style=for-the-badge&logo=qt)
![C++](https://img.shields.io/badge/C++-11-blue?style=for-the-badge&logo=cplusplus)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

**🚀 AI驱动的现代化渗透测试框架**

*参考Metasploit设计理念，集成AI辅助能力的下一代安全测试平台*

</div>

---

## 📖 项目简介

**MindSploit** 是一个基于C++和Qt6开发的现代化渗透测试框架，旨在为安全研究人员和渗透测试工程师提供一站式、智能化、易用的安全测试平台。

### 🎯 核心特性

- 🧠 **AI智能辅助** - 集成大模型API，支持命令解析、结果分析、自动化测试
- 🎨 **现代化界面** - 基于Qt6的直观GUI，上下分屏设计（可视化区+终端区）
- 🔧 **模块化架构** - 插件式设计，轻松扩展新工具和功能
- 🛠️ **工具集成** - 内置nmap、curl等常用安全工具的适配器
- 📊 **可视化展示** - 网络拓扑图、扫描结果、报告生成
- 🖥️ **跨平台支持** - 优先支持Windows，计划兼容Linux

### 🏗️ 技术架构

```
┌─────────────────────────────────────────────────────────┐
│                    MindSploit 架构                        │
├─────────────────────────────────────────────────────────┤
│  GUI Layer (Qt6)     │  可视化区  │  终端交互区  │  菜单栏  │
├─────────────────────────────────────────────────────────┤
│  Core Layer          │  命令解析  │  任务调度   │  模块管理 │
├─────────────────────────────────────────────────────────┤
│  AI Layer            │  API集成   │  结果分析   │  智能建议 │
├─────────────────────────────────────────────────────────┤
│  Modules Layer       │  Nmap     │  Curl      │  自定义   │
├─────────────────────────────────────────────────────────┤
│  Utils Layer         │  进程管理  │  文件处理   │  网络工具 │
└─────────────────────────────────────────────────────────┘
```

## 🚀 快速开始

### 环境要求

- **Qt 6.9.1+** - GUI框架
- **MinGW 编译器** - C++编译环境
- **CMake 3.15+** - 构建系统
- **Windows 10+** - 操作系统（优先支持）

### 🔨 构建方式

#### 方式一：使用构建脚本（推荐）

```bash
# 一键构建
build.bat

# 清理构建文件
clean.bat
```

#### 方式二：手动构建

```bash
# 1. 生成Makefile
E:\Qt\6.9.1\mingw_64\bin\qmake.exe MindSploit.pro

# 2. 编译项目
E:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe

# 3. 运行程序
.\release\MindSploit.exe
```

#### 方式三：CMake构建

```bash
# 配置项目
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=E:/Qt/6.9.1/mingw_64

# 构建项目
cmake --build build --config Release

# 运行程序
.\build\MindSploit.exe
```

## 📁 项目结构

```
MindSploit/
├── 📄 CMakeLists.txt           # CMake构建配置
├── 📄 MindSploit.pro          # Qt项目文件
├── 📄 README.md               # 项目文档
├── 🔧 build.bat               # 一键构建脚本
├── 🔧 clean.bat               # 清理脚本
├── 📁 src/                    # 源代码目录
│   ├── 📄 main.cpp            # 程序入口点
│   ├── 📁 gui/                # 图形界面模块
│   │   ├── mainwindow.h       # 主窗口头文件
│   │   └── mainwindow.cpp     # 主窗口实现
│   ├── 📁 core/               # 核心逻辑模块
│   │   ├── command.h          # 命令系统头文件
│   │   └── command.cpp        # 命令系统实现
│   ├── 📁 ai/                 # AI集成模块
│   │   ├── ai_manager.h       # AI管理器头文件
│   │   └── ai_manager.cpp     # AI管理器实现
│   ├── 📁 modules/            # 渗透测试模块
│   │   ├── nmap_module.h      # Nmap模块头文件
│   │   └── nmap_module.cpp    # Nmap模块实现
│   └── 📁 utils/              # 工具类模块
│       ├── process.h          # 进程管理头文件
│       └── process.cpp        # 进程管理实现
├── 📁 tests/                  # 测试代码
│   └── test_main.cpp          # 主测试文件
├── 📁 release/                # 发布版本输出
│   └── MindSploit.exe         # 可执行文件
└── 📁 debug/                  # 调试版本输出
```

## 🎮 使用指南

### 界面布局

```
┌─────────────────────────────────────────────────────────┐
│  MindSploit - 菜单栏                                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  📊 可视化区域 (Visual Area)                              │
│  - 网络拓扑图                                             │
│  - 扫描结果展示                                           │
│  - 报告生成                                              │
│                                                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  💻 终端交互区 (Terminal Area)                            │
│  - 命令输入                                              │
│  - 日志输出                                              │
│  - AI对话                                               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 核心模块说明

| 模块 | 功能描述 | 状态 |
|------|----------|------|
| **GUI模块** | Qt6界面，分屏显示，用户交互 | ✅ 已实现 |
| **Core模块** | 命令解析、任务调度、模块管理 | 🚧 开发中 |
| **AI模块** | 大模型集成、智能分析、自动化 | 🚧 开发中 |
| **Modules模块** | Nmap、Curl等工具适配器 | 🚧 开发中 |
| **Utils模块** | 进程管理、文件处理、网络工具 | 🚧 开发中 |

## 🔧 开发配置

### VS Code 配置

项目已配置好 VS Code 的调试和构建任务：

1. **构建**: `Ctrl+Shift+P` → "Tasks: Run Task" → "build"
2. **配置**: `Ctrl+Shift+P` → "Tasks: Run Task" → "configure"
3. **调试**: `F5` 或 `Ctrl+Shift+P` → "Debug: Start Debugging"

### 故障排除

#### Qt相关错误

如果遇到 `QApplication: No such file or directory` 错误：

1. 确保Qt路径正确：检查 `CMakeLists.txt` 中的 `CMAKE_PREFIX_PATH`
2. 使用qmake构建：`qmake MindSploit.pro && mingw32-make`
3. 检查环境变量：确保PATH包含Qt和MinGW的bin目录
4. 重启开发环境：重新加载配置

#### 编译错误

```bash
# 清理并重新构建
clean.bat
build.bat

# 或手动清理
del Makefile* && rmdir /s release debug
```

## 🚀 开发路线图

### Phase 1: 基础框架 (当前阶段)
- [x] Qt6 GUI基础界面
- [x] 项目结构搭建
- [x] 构建系统配置
- [ ] 核心命令系统
- [ ] 模块管理框架

### Phase 2: 核心功能
- [ ] AI集成与API对接
- [ ] Nmap模块完整实现
- [ ] 进程管理优化
- [ ] 配置文件系统

### Phase 3: 高级特性
- [ ] 可视化图表
- [ ] 报告生成系统
- [ ] 插件扩展机制
- [ ] 多线程优化

### Phase 4: 完善与发布
- [ ] 单元测试覆盖
- [ ] 文档完善
- [ ] 安装包制作
- [ ] Linux平台支持

## 🤝 贡献指南

欢迎参与MindSploit的开发！请遵循以下规范：

1. **代码规范**: 遵循项目的命名约定和代码风格
2. **提交规范**: 使用清晰的commit message
3. **测试**: 确保新功能有对应的测试用例
4. **文档**: 更新相关文档和注释

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 📞 联系方式

- **项目地址**: [GitHub Repository]
- **问题反馈**: [Issues]
- **讨论交流**: [Discussions]

---

<div align="center">

**⭐ 如果这个项目对您有帮助，请给我们一个Star！**

Made with ❤️ by MindSploit Team

</div>
