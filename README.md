# MindSploit

<div align="center">

![MindSploit Logo](https://img.shields.io/badge/MindSploit-v2.0-blue?style=for-the-badge)
![Qt](https://img.shields.io/badge/Qt-6.9.1-green?style=for-the-badge&logo=qt)
![C++](https://img.shields.io/badge/C++-17-blue?style=for-the-badge&logo=cplusplus)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

**🚀 AI驱动的自主渗透测试框架**

*全自主开发内核，无外部工具依赖的下一代安全测试平台*

</div>

---

## 📖 项目简介

**MindSploit** 是一个基于C++17开发的现代化渗透测试框架，采用全自主开发的内核工具，无需依赖外部工具。为安全研究人员和渗透测试工程师提供一站式、智能化、高效的安全测试平台。

### 🎯 核心特性

- 🧠 **AI智能辅助** - 集成大模型API，支持命令解析、结果分析、自动化测试
- ⚡ **自主内核** - 全自研网络扫描、漏洞检测、协议分析等核心功能
- 🔧 **模块化架构** - 插件式设计，轻松扩展新工具和功能
- 💻 **命令行优先** - 专业终端交互，类似Metasploit的使用体验
- 📊 **智能分析** - AI驱动的漏洞分析和攻击路径规划
- 🖥️ **跨平台支持** - 优先支持Windows，计划兼容Linux

### 🏗️ 技术架构

```
┌─────────────────────────────────────────────────────────┐
│                    MindSploit 架构                        │
├─────────────────────────────────────────────────────────┤
│  Terminal Layer      │  命令解析  │  交互界面   │  历史管理 │
├─────────────────────────────────────────────────────────┤
│  Core Layer          │  任务调度  │  模块管理   │  会话管理 │
├─────────────────────────────────────────────────────────┤
│  AI Layer            │  API集成   │  结果分析   │  智能建议 │
├─────────────────────────────────────────────────────────┤
│  Engine Layer        │  网络扫描  │  漏洞检测   │  协议分析 │
├─────────────────────────────────────────────────────────┤
│  Network Layer       │  原始套接字 │  协议栈    │  数据包处理│
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
├── � LICENSE                 # 开源许可证
├── 📁 src/                    # 源代码目录
│   ├── 📄 main.cpp            # 程序入口点
│   ├── 📁 core/               # 核心逻辑模块
│   │   ├── terminal_interface.h/cpp    # 终端交互界面
│   │   ├── engine_manager.h/cpp        # 引擎管理器
│   │   ├── session_manager.h/cpp       # 会话管理器
│   │   ├── command_parser.h/cpp        # 命令解析器
│   │   ├── database.h/cpp              # 数据库管理
│   │   └── config_manager.h/cpp        # 配置管理
│   ├── 📁 engines/            # 渗透测试引擎
│   │   ├── engine_interface.h          # 引擎接口定义
│   │   └── network/                    # 网络扫描引擎
│   │       ├── network_engine.h
│   │       └── network_engine.cpp
│   ├── 📁 ai/                 # AI集成模块
│   │   ├── ai_manager.h       # AI管理器头文件
│   │   └── ai_manager.cpp     # AI管理器实现
│   └── 📁 utils/              # 工具类模块
│       ├── network_utils.h    # 网络工具类头文件
│       └── network_utils.cpp  # 网络工具类实现
├── 📁 docs/                   # 项目文档
│   ├── NEW_DEVELOPMENT_PLAN.md # 新开发计划
│   └── ...                    # 其他文档
├── 📁 tests/                  # 测试代码
│   └── test_main.cpp          # 主测试文件
└── 📁 release/                # 发布版本输出
    └── MindSploit.exe         # 可执行文件
```

## 🎮 使用指南

### 界面布局

```
┌─────────────────────────────────────────────────────────┐
│  MindSploit v1.0.0 - 菜单栏 [文件 视图 工具 帮助]         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  📊 可视化区域 (Visual Area)                            │
│  - 扫描结果表格展示                                      │
│  - 网络拓扑图                                            │
│  - 命令历史记录                                          │
│  - 项目管理界面                                          │
│                                                         │
├─────────────────────────────────────────────────────────┤
│  💻 专业终端 (MindSploit Terminal)                      │
│  ┌─────────────────────────────────────────────────────┐│
│  │ Welcome to MindSploit - AI-Powered Framework       │ │
│  │ Type 'help' for available commands                 │ │
│  │                                                    │ │
│  │ MindSploit> help                                   │ │
│  │ MindSploit> nmap -sS 192.168.1.1                   │ │
│  │ MindSploit> history                                 │ │
│  │ ┌─────────────────────────────────────────────────┐ │ │
│  │ │ MindSploit> _                                   │ │ │
│  │ └─────────────────────────────────────────────────┘ │ │
│  └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 🎮 终端特性

- **专业提示符**: `MindSploit>` 类似msfconsole的专业界面
- **命令历史**: 使用 ↑↓ 键浏览历史命令
- **自动补全**: Tab键补全命令和参数
- **多彩输出**: 不同类型信息用不同颜色显示
- **实时交互**: 命令与可视化区域实时联动

### 核心模块说明

| 模块 | 功能描述 | 状态 |
|------|----------|------|
| **GUI模块** | Qt6界面，专业终端，分屏显示 | 🚧 未实现 |
| **终端系统** | MindSploit>命令行，历史记录，补全 | 🚧 未实现 |
| **Core模块** | 命令解析、任务调度、模块管理 | 🚧 开发中 |
| **AI模块** | 大模型集成、智能分析、自动化 | 🚧 开发中 |
| **Modules模块** | Nmap、Curl等工具适配器 | 🚧 开发中 |
| **Utils模块** | 进程管理、文件处理、网络工具 | 🚧 开发中 |
| **数据存储** | SQLite数据库、配置管理 | 🚧 未实现 |

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
- [x] 数据存储系统
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
