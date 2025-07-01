# MindSploit 项目结构说明

## 📁 整体架构

```
MindSploit/
├── 📄 项目配置文件
│   ├── CMakeLists.txt              # CMake构建配置
│   ├── MindSploit.pro             # Qt项目文件
│   ├── .cursor-rules.yaml         # Cursor IDE规则
│   └── .gitignore                 # Git忽略文件
├── 📄 文档文件
│   ├── README.md                  # 项目说明
│   ├── DEVELOPMENT_PLAN.md        # 开发计划
│   ├── DATA_STORAGE_GUIDE.md      # 数据存储指南
│   ├── PACKAGING_GUIDE.md         # 打包发布指南
│   └── PROJECT_STRUCTURE.md       # 项目结构说明
├── 🔧 构建脚本
│   ├── build.bat                  # 一键构建脚本
│   ├── clean.bat                  # 清理脚本
│   ├── package.bat                # 打包脚本
│   ├── create_installer.bat       # 安装包制作
│   └── test_run.bat               # 测试运行脚本
├── 📁 src/                        # 源代码目录
│   ├── main.cpp                   # 程序入口
│   ├── 📁 core/                   # 核心模块
│   ├── 📁 gui/                    # 图形界面
│   ├── 📁 ai/                     # AI集成
│   ├── 📁 modules/                # 渗透测试模块
│   └── 📁 utils/                  # 工具类
├── 📁 tests/                      # 测试代码
├── 📁 release/                    # 发布版本输出
└── 📁 debug/                      # 调试版本输出
```

## 🏗️ 核心架构设计

### 1. 模块化架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (GUI)                          │
├─────────────────────────────────────────────────────────┤
│                    核心层 (Core)                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │
│  │ 模块管理器   │ │ 命令系统    │ │ 数据存储    │        │
│  └─────────────┘ └─────────────┘ └─────────────┘        │
├─────────────────────────────────────────────────────────┤
│                    模块层 (Modules)                      │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │
│  │    Nmap     │ │ Searchsploit│ │    Curl     │        │
│  └─────────────┘ └─────────────┘ └─────────────┘        │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │
│  │  PortScan   │ │    AI助手   │ │   自定义    │        │
│  └─────────────┘ └─────────────┘ └─────────────┘        │
├─────────────────────────────────────────────────────────┤
│                    工具层 (Utils)                        │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │
│  │ 进程管理    │ │ 网络工具    │ │ 文件处理    │        │
│  └─────────────┘ └─────────────┘ └─────────────┘        │
└─────────────────────────────────────────────────────────┘
```

## 📂 详细目录结构

### src/core/ - 核心模块

```
src/core/
├── module_interface.h/cpp      # 模块接口基类
├── module_manager.h/cpp        # 模块管理器
├── command.h/cpp               # 命令系统
├── database.h/cpp              # 数据库管理
└── config_manager.h/cpp        # 配置管理
```

**功能说明:**
- `ModuleInterface`: 所有渗透测试模块的统一接口
- `ModuleManager`: 模块注册、发现、执行和管理
- `Command`: 命令解析、历史记录、自动补全
- `Database`: SQLite数据存储，命令历史、扫描结果
- `ConfigManager`: 用户配置、API密钥、工具路径

### src/gui/ - 图形界面

```
src/gui/
├── mainwindow.h/cpp           # 主窗口
└── terminal_widget.h/cpp      # 专业终端组件
```

**功能说明:**
- `MainWindow`: 主界面，上下分屏布局，菜单栏
- `TerminalWidget`: 类似msfconsole的专业终端界面

### src/modules/ - 渗透测试模块

```
src/modules/
├── 📁 nmap/                   # Nmap网络扫描
│   ├── nmap_module.h
│   └── nmap_module.cpp
├── 📁 searchsploit/           # 漏洞搜索
│   ├── searchsploit_module.h
│   └── searchsploit_module.cpp
├── 📁 curl/                   # HTTP请求工具
│   ├── curl_module.h
│   └── curl_module.cpp
├── 📁 portscan/               # 内置端口扫描
│   ├── portscan_module.h
│   └── portscan_module.cpp
└── 📁 [future_modules]/       # 未来扩展模块
```

**设计原则:**
- 每个工具独立文件夹
- 统一的模块接口
- 支持外部工具和内置实现
- 模块间解耦，便于扩展

### src/ai/ - AI集成

```
src/ai/
├── ai_manager.h/cpp           # AI管理器
└── [future_ai_modules]/       # 未来AI模块
```

### src/utils/ - 工具类

```
src/utils/
├── process.h/cpp              # 进程管理
└── [future_utils]/            # 未来工具类
```

## 🔧 模块接口设计

### ModuleInterface 基类

所有渗透测试模块都继承自 `ModuleInterface`，提供统一的接口：

```cpp
class ModuleInterface : public QObject {
    Q_OBJECT
public:
    // 基本信息
    virtual QString getName() const = 0;
    virtual QString getDescription() const = 0;
    virtual ModuleType getType() const = 0;
    
    // 参数配置
    virtual QJsonObject getDefaultOptions() const = 0;
    virtual bool setOption(const QString& key, const QVariant& value) = 0;
    virtual bool validateOptions() const = 0;
    
    // 执行控制
    virtual bool isAvailable() const = 0;
    virtual void execute() = 0;
    virtual void stop() = 0;
    
    // 结果处理
    virtual QJsonObject getResults() const = 0;
    virtual QString getHelp() const = 0;
    
signals:
    void outputReceived(const QString& output);
    void executionCompleted(const QJsonObject& results);
    void executionFailed(const QString& error);
};
```

## 🎯 模块类型分类

### 1. 扫描器类型 (SCANNER)
- **Nmap**: 网络发现和端口扫描
- **PortScan**: 内置TCP/UDP端口扫描器
- **ZMap**: 高速网络扫描器 (计划)
- **XMap**: 多协议扫描器 (计划)

### 2. 辅助工具类型 (AUXILIARY)
- **Searchsploit**: 漏洞数据库搜索
- **Curl**: HTTP/HTTPS请求工具
- **Ping**: 网络连通性测试 (计划)
- **Traceroute**: 路由跟踪 (计划)

### 3. 漏洞利用类型 (EXPLOIT)
- **Metasploit模块**: 漏洞利用框架集成 (计划)
- **自定义Exploit**: 自开发漏洞利用 (计划)

### 4. 后渗透类型 (POST)
- **文件传输**: 文件上传下载 (计划)
- **权限提升**: 提权工具集成 (计划)

## 🔄 命令执行流程

```
用户输入命令
    ↓
TerminalWidget 接收
    ↓
MainWindow 处理
    ↓
ModuleManager 路由
    ↓
具体Module 执行
    ↓
结果返回显示
    ↓
数据库存储
```

## 📊 数据流设计

### 1. 命令历史
```
用户命令 → Database → 历史记录 → 终端显示
```

### 2. 扫描结果
```
模块执行 → JSON结果 → Database → 可视化显示
```

### 3. 配置管理
```
用户设置 → ConfigManager → QSettings/JSON → 持久化存储
```

## 🚀 扩展指南

### 添加新模块

1. **创建模块目录**
```bash
mkdir src/modules/new_tool/
```

2. **实现模块类**
```cpp
class NewToolModule : public ModuleInterface {
    // 实现所有虚函数
};
```

3. **注册模块**
```cpp
// 在 ModuleManager::initializeBuiltinModules() 中
registerModule("newtool", new NewToolModule(this));
```

4. **更新项目文件**
```pro
# 在 MindSploit.pro 中添加
SOURCES += src/modules/new_tool/new_tool_module.cpp
HEADERS += src/modules/new_tool/new_tool_module.h
```

### 添加外部工具支持

1. **检查工具可用性**
```cpp
bool isAvailable() const override {
    return checkDependency("tool_name");
}
```

2. **构建命令行**
```cpp
QString buildCommand() const {
    QString toolPath = getToolPath("tool_name");
    // 构建完整命令
}
```

3. **解析输出结果**
```cpp
void parseOutput(const QString& output) {
    // 解析工具输出为JSON格式
}
```

## 🎨 界面设计原则

### 1. 分离设计
- **可视化区域**: 显示结果、图表、报告
- **终端区域**: 命令交互、日志输出
- **菜单栏**: 功能快捷入口

### 2. 响应式布局
- 支持窗口大小调整
- 分割器可拖拽调节
- 适配不同分辨率

### 3. 主题支持
- 暗色主题为主
- 支持自定义配色
- 终端样式可配置

---

这个模块化架构确保了MindSploit的可扩展性、可维护性和跨平台兼容性。
