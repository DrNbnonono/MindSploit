# MindSploit 数据存储方案

## 📊 存储架构概述

MindSploit采用**轻量级混合存储方案**，无需安装额外数据库服务：

```
📁 数据存储架构
├── 🗄️ SQLite数据库 (结构化数据)
│   ├── 命令历史记录
│   ├── 扫描结果数据
│   ├── AI对话记录
│   ├── 项目会话管理
│   └── 报告数据
├── ⚙️ QSettings配置 (用户设置)
│   ├── 界面配置
│   ├── 工具路径
│   ├── AI API配置
│   └── 安全设置
└── 📄 JSON文件 (复杂配置)
    ├── 扫描模板
    ├── 报告模板
    └── 插件配置
```

## 🎯 为什么选择这个方案？

### ✅ **优势**
- **零依赖** - 无需安装MySQL/PostgreSQL等数据库服务
- **便携性** - 所有数据存储在应用目录，支持绿色版
- **轻量级** - SQLite文件小，性能优秀
- **简单性** - 开发和维护成本低
- **跨平台** - Qt内置支持，Windows/Linux通用

### 🚫 **不适用场景**
- 大规模并发访问（本项目是单用户工具）
- 复杂关系查询（数据结构相对简单）
- 分布式部署（本地工具无此需求）

## 📁 数据存储位置

### Windows
```
C:\Users\{用户名}\AppData\Roaming\MindSploit\
├── mindsploit.db          # SQLite数据库
├── settings.ini           # QSettings配置
├── config.json            # JSON配置
├── logs\                  # 日志文件
└── reports\               # 导出报告
```

### Linux (未来支持)
```
~/.local/share/MindSploit/
├── mindsploit.db
├── settings.ini
├── config.json
├── logs/
└── reports/
```

## 🛠️ 使用方法

### 1. 数据库操作示例

```cpp
#include "core/database.h"

// 获取数据库实例
Database& db = Database::instance();

// 初始化数据库
if (!db.initialize()) {
    qDebug() << "Database initialization failed";
    return;
}

// 添加命令历史
db.addCommandHistory("nmap -sS 192.168.1.1", "扫描结果...", "project1");

// 获取命令历史
QList<QJsonObject> history = db.getCommandHistory("project1", 10);
for (const auto& cmd : history) {
    qDebug() << cmd["command"].toString();
}

// 保存扫描结果
QJsonObject scanResult;
scanResult["target"] = "192.168.1.1";
scanResult["ports"] = QJsonArray{80, 443, 22};
db.addScanResult("192.168.1.1", "nmap", scanResult, "project1");

// 创建新项目
db.createProject("WebApp_Test", "Web应用渗透测试项目");
```

### 2. 配置管理示例

```cpp
#include "core/config_manager.h"

// 获取配置管理器实例
ConfigManager& config = ConfigManager::instance();
config.initialize();

// 设置AI API密钥
config.setAIApiKey("openai", "sk-your-api-key");
config.setAIModel("openai", "gpt-4");

// 设置工具路径
config.setToolPath("nmap", "C:\\Program Files\\Nmap\\nmap.exe");
config.setToolPath("curl", "C:\\Windows\\System32\\curl.exe");

// 获取配置
QString apiKey = config.getAIApiKey("openai");
QString nmapPath = config.getToolPath("nmap");

// 界面配置
config.setTheme("dark");
config.setAutoSaveInterval(300); // 5分钟自动保存
```

### 3. 项目会话管理

```cpp
// 创建新项目
db.createProject("Company_PenTest", "某公司渗透测试项目");

// 切换当前项目
db.setCurrentProject("Company_PenTest");

// 在当前项目下执行操作
db.addCommandHistory("nmap -sS target.com", "扫描结果...");
db.addAIConversation("如何扫描这个目标？", "建议使用nmap进行端口扫描...");

// 获取项目列表
QList<QJsonObject> projects = db.getProjects();
```

## 📊 数据库表结构

### projects (项目表)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER | 主键 |
| name | TEXT | 项目名称 |
| description | TEXT | 项目描述 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

### command_history (命令历史表)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER | 主键 |
| project | TEXT | 所属项目 |
| command | TEXT | 执行的命令 |
| output | TEXT | 命令输出 |
| execution_time | REAL | 执行时间(秒) |
| created_at | DATETIME | 执行时间 |

### scan_results (扫描结果表)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER | 主键 |
| project | TEXT | 所属项目 |
| target | TEXT | 扫描目标 |
| scan_type | TEXT | 扫描类型 |
| result_data | TEXT | JSON格式结果 |
| created_at | DATETIME | 扫描时间 |

### ai_conversations (AI对话表)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER | 主键 |
| project | TEXT | 所属项目 |
| user_input | TEXT | 用户输入 |
| ai_response | TEXT | AI回复 |
| created_at | DATETIME | 对话时间 |

### reports (报告表)
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER | 主键 |
| project | TEXT | 所属项目 |
| report_name | TEXT | 报告名称 |
| report_data | TEXT | JSON格式报告数据 |
| created_at | DATETIME | 创建时间 |

## 🔧 集成到主程序

### 在MainWindow中初始化

```cpp
// src/gui/mainwindow.cpp
#include "core/database.h"
#include "core/config_manager.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 初始化数据存储
    if (!Database::instance().initialize()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败");
        return;
    }
    
    ConfigManager::instance().initialize();
    
    // 恢复窗口状态
    restoreGeometry(ConfigManager::instance().getWindowGeometry());
    restoreState(ConfigManager::instance().getWindowState());
    
    // 设置当前项目
    QString lastProject = ConfigManager::instance().getLastProject();
    Database::instance().setCurrentProject(lastProject);
    
    setupUI();
}

MainWindow::~MainWindow() {
    // 保存窗口状态
    ConfigManager::instance().setWindowGeometry(saveGeometry());
    ConfigManager::instance().setWindowState(saveState());
}
```

## 📈 性能考虑

### SQLite优化
- 使用事务批量插入数据
- 创建适当的索引
- 定期执行VACUUM清理

### 内存管理
- 大量数据分页加载
- 及时释放查询结果
- 使用连接池（如需要）

### 文件大小控制
- 定期清理旧数据
- 压缩历史记录
- 分离归档数据

## 🔒 数据安全

### 敏感信息加密
- API密钥加密存储
- 可选的数据库加密
- 安全的密码存储

### 备份策略
- 自动备份数据库文件
- 配置文件同步
- 导出/导入功能

## 🚀 未来扩展

### 可能的增强
- 数据同步功能
- 云端备份
- 多用户支持
- 数据分析功能

---

这个存储方案为MindSploit提供了完整、轻量、易用的数据管理能力，满足渗透测试框架的所有数据存储需求。
