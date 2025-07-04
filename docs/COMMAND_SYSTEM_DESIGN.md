# MindSploit 命令系统设计文档

## 🎯 设计目标

MindSploit的命令系统参考Metasploit Framework，提供专业、直观、强大的命令行界面。支持模块化操作、智能补全、历史记录和AI辅助。

## 🎮 命令分类体系

### 1. 全局命令 (Global Commands)
**作用域**: 在任何上下文中都可用的基础命令

```bash
# === 系统控制命令 ===
help [command/module]         # 显示帮助信息
version                       # 显示版本和构建信息
exit / quit                   # 退出程序
clear / cls                   # 清空终端屏幕
banner                        # 显示程序横幅
history [count]               # 显示命令历史
alias <name> <command>        # 创建命令别名
unalias <name>               # 删除命令别名

# === 模块管理命令 ===
use <module_path>            # 使用指定模块
back                         # 返回上一级上下文
info [module]               # 显示模块详细信息
search <keyword>            # 搜索模块
show <type>                 # 显示信息
reload [module]             # 重新加载模块
unload <module>             # 卸载模块

# === 配置管理命令 ===
set <option> <value>        # 设置当前上下文选项
unset <option>              # 取消设置选项
setg <option> <value>       # 设置全局选项
unsetg <option>             # 取消全局选项
save [filename]             # 保存当前配置
load <filename>             # 加载配置文件
reset                       # 重置所有设置

# === 会话管理命令 ===
sessions                    # 显示所有活动会话
session <id>               # 切换到指定会话
session -i <id>            # 交互指定会话
session -k <id>            # 终止指定会话
session -K                 # 终止所有会话
background / bg            # 将当前会话放到后台

# === 任务管理命令 ===
jobs                       # 显示后台任务
job <id>                   # 查看任务详情
kill <job_id>              # 终止指定任务
killall                    # 终止所有任务
```

### 2. 上下文命令 (Context Commands)
**作用域**: 在特定模块上下文中可用的命令

```bash
# === 模块操作命令 ===
show options               # 显示模块选项
show advanced             # 显示高级选项
show targets              # 显示可用目标
show payloads             # 显示兼容载荷
show info                 # 显示模块信息
show missing              # 显示缺失的必需选项

# === 模块执行命令 ===
check                     # 检查目标是否易受攻击
run                       # 执行模块 (通用)
exploit                   # 执行漏洞利用模块
scan                      # 执行扫描模块
auxiliary                 # 执行辅助模块
rexploit / rerun          # 重新执行模块

# === 载荷管理命令 ===
generate                  # 生成载荷
encode                    # 编码载荷
decode                    # 解码载荷
```

### 3. AI交互命令 (AI Commands)
**作用域**: AI模式下的专用命令

```bash
# === AI模式控制 ===
ai                        # 进入AI交互模式
ai exit                   # 退出AI模式
ai help                   # AI命令帮助
ai status                 # AI状态信息

# === AI模型管理 ===
ai models                 # 列出所有可用模型
ai model <name>           # 切换到指定模型
ai model info            # 显示当前模型信息
ai model test            # 测试模型连接
ai model config          # 配置模型参数

# === AI分析功能 ===
ai analyze [target]       # 智能分析目标
ai suggest               # 获取下一步建议
ai explain <command>     # 解释命令作用
ai risk <action>         # 评估操作风险
ai plan <objective>      # 制定攻击计划

# === AI自动化 ===
ai auto scan             # 自动扫描
ai auto exploit          # 自动漏洞利用
ai auto report           # 自动生成报告
ai execute plan          # 执行AI计划
```

## 🔧 命令解析架构

### 命令解析器设计
```cpp
class CommandParser {
public:
    struct ParsedCommand {
        std::string command;                    // 主命令
        std::vector<std::string> arguments;     // 位置参数
        std::map<std::string, std::string> options; // 选项参数
        CommandType type;                       // 命令类型
        std::string context;                    // 执行上下文
        bool isValid;                          // 解析是否成功
        std::string errorMessage;              // 错误信息
    };
    
    // 核心解析功能
    ParsedCommand parseCommand(const std::string& input);
    bool validateCommand(const ParsedCommand& cmd);
    std::vector<std::string> getCompletions(const std::string& input);
    
    // 命令注册
    void registerCommand(const CommandDefinition& cmdDef);
    void registerAlias(const std::string& alias, const std::string& command);
    
    // 历史管理
    void addToHistory(const std::string& command);
    std::vector<std::string> getHistory() const;
    std::string getHistoryItem(int index) const;
};
```

### 命令执行引擎
```cpp
class CommandExecutor {
public:
    // 命令执行
    ExecutionResult executeCommand(const ParsedCommand& cmd);
    bool canExecute(const ParsedCommand& cmd) const;
    
    // 上下文管理
    void setContext(const std::string& context);
    std::string getCurrentContext() const;
    void pushContext(const std::string& context);
    void popContext();
    
    // 权限检查
    bool hasPermission(const std::string& command) const;
    void setPermissionLevel(PermissionLevel level);

private:
    std::stack<std::string> contextStack;
    std::map<std::string, std::unique_ptr<CommandHandler>> handlers;
    PermissionLevel currentPermission;
};
```

## 📝 命令语法规范

### 基础语法
```bash
# 基本命令格式
command [arguments] [options]

# 选项格式
-option value              # 短选项
--long-option value        # 长选项
-flag                      # 布尔标志
--verbose                  # 长布尔标志

# 参数引用
"string with spaces"       # 带空格的字符串
'single quoted'            # 单引号字符串
$VARIABLE                  # 变量引用
${COMPLEX_VAR}            # 复杂变量引用
```

### 高级语法特性
```bash
# 管道操作
search web | grep sql      # 搜索结果管道过滤
show modules | head -10    # 显示前10个模块

# 命令组合
set RHOSTS 192.168.1.1; run; back    # 分号分隔多命令
use exploit/windows/smb/ms17_010 && set RHOSTS 192.168.1.100 && exploit

# 变量替换
set TARGET 192.168.1.100
use discovery/network/port_scanner
set RHOSTS $TARGET
run

# 条件执行
check && exploit           # 检查成功后执行利用
run || echo "Scan failed"  # 扫描失败时显示消息
```

## 🎯 智能补全系统

### 补全类型
```cpp
enum class CompletionType {
    COMMAND,        // 命令补全
    MODULE,         // 模块路径补全
    OPTION,         // 选项名补全
    VALUE,          // 选项值补全
    FILE,           // 文件路径补全
    VARIABLE,       // 变量名补全
    TARGET,         // 目标地址补全
    PAYLOAD         // 载荷补全
};

class CompletionEngine {
public:
    std::vector<std::string> getCompletions(
        const std::string& input, 
        size_t cursorPos,
        const std::string& context
    );
    
    void registerCompletionProvider(
        CompletionType type,
        std::unique_ptr<CompletionProvider> provider
    );
    
    void updateCompletionCache();
};
```

### 智能补全示例
```bash
# 命令补全
MindSploit > se<TAB>
search  sessions  set  setg

# 模块补全
MindSploit > use exp<TAB>
exploits/

MindSploit > use exploits/win<TAB>
exploits/windows/

# 选项补全
MindSploit exploit(ms17_010) > set RH<TAB>
RHOSTS

# 值补全
MindSploit exploit(ms17_010) > set PAYLOAD <TAB>
windows/x64/meterpreter/reverse_tcp
windows/x64/meterpreter/bind_tcp
windows/x64/shell/reverse_tcp
```

## 📚 帮助系统设计

### 多层次帮助
```cpp
class HelpSystem {
public:
    // 帮助内容获取
    std::string getGlobalHelp();
    std::string getCommandHelp(const std::string& command);
    std::string getModuleHelp(const std::string& module);
    std::string getContextHelp(const std::string& context);
    
    // 交互式帮助
    void showInteractiveHelp();
    void showQuickStart();
    void showExamples(const std::string& topic);
    
    // 搜索功能
    std::vector<std::string> searchHelp(const std::string& keyword);
    std::vector<std::string> getSimilarCommands(const std::string& command);
};
```

### 帮助内容示例
```bash
# 全局帮助
MindSploit > help
MindSploit Framework v2.0 - AI驱动的渗透测试框架

核心命令:
  help                显示帮助信息
  use <module>        使用模块
  search <keyword>    搜索模块
  show <type>         显示信息
  set <option>        设置选项
  
更多信息请使用: help <command>

# 命令特定帮助
MindSploit > help use
命令: use
描述: 选择并加载指定的模块
语法: use <module_path>

示例:
  use exploits/windows/smb/ms17_010
  use discovery/network/port_scanner
  use auxiliary/scanner/http/dir_scanner

相关命令: back, info, show modules

# 模块帮助
MindSploit > use exploits/windows/smb/ms17_010
MindSploit exploit(ms17_010) > help

模块: exploits/windows/smb/ms17_010
名称: MS17-010 EternalBlue SMB Remote Windows Kernel Pool Corruption
作者: Sean Dillon, Lucas Leong
描述: 利用MS17-010漏洞进行远程代码执行

基本选项:
  RHOSTS    目标主机地址    必需
  RPORT     目标端口       445
  
高级选项:
  PAYLOAD   攻击载荷       自动选择
  
使用方法:
  1. set RHOSTS <target>
  2. check (可选)
  3. exploit
```

## 🔄 命令历史和会话

### 历史管理
```cpp
class CommandHistory {
public:
    void addCommand(const std::string& command);
    std::vector<std::string> getHistory() const;
    std::string getCommand(int index) const;
    void clearHistory();
    
    // 搜索功能
    std::vector<std::string> searchHistory(const std::string& pattern);
    std::string getLastCommand(const std::string& prefix);
    
    // 持久化
    bool saveHistory(const std::string& filename);
    bool loadHistory(const std::string& filename);
};
```

### 会话状态管理
```cpp
class SessionState {
public:
    // 状态保存/恢复
    void saveState();
    void restoreState();
    void exportState(const std::string& filename);
    void importState(const std::string& filename);
    
    // 状态查询
    std::string getCurrentModule() const;
    std::map<std::string, std::string> getCurrentOptions() const;
    std::vector<std::string> getLoadedModules() const;
    
    // 状态修改
    void setCurrentModule(const std::string& module);
    void setOption(const std::string& key, const std::string& value);
    void clearState();
};
```

这个命令系统设计提供了完整的专业渗透测试命令行体验，支持智能补全、多层次帮助、历史管理和AI集成，既保持了MSF的专业性，又增加了现代化的用户体验。
