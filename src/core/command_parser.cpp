#include "command_parser.h"
#include <algorithm>
#include <sstream>

namespace MindSploit::Core {

CommandParser::CommandParser() = default;

CommandParser::~CommandParser() = default;

bool CommandParser::initialize() {
    defineBuiltinCommands();
    return true;
}

ParsedCommand CommandParser::parseCommand(const std::string& input) {
    ParsedCommand result;
    result.originalCommand = input;
    
    if (input.empty()) {
        result.errorMessage = "Empty command";
        return result;
    }
    
    // 解析别名
    std::string resolvedInput = resolveAlias(input);
    
    // 分词
    auto tokens = tokenize(resolvedInput);
    if (tokens.empty()) {
        result.errorMessage = "No command found";
        return result;
    }
    
    result.command = normalizeCommand(tokens[0]);
    result.arguments = std::vector<std::string>(tokens.begin() + 1, tokens.end());
    
    // 解析参数
    result.parameters = parseParameters(tokens, 1);
    
    // 确定命令类型
    auto it = m_commands.find(result.command);
    if (it != m_commands.end()) {
        result.type = it->second.type;
        result.engineName = it->second.engineName;
        result.isValid = true;
    } else {
        result.type = CommandType::UNKNOWN;
        result.isValid = false;
        result.errorMessage = "Unknown command: " + result.command;
    }
    
    return result;
}

bool CommandParser::validateCommand(const ParsedCommand& command) {
    if (!command.isValid) {
        return false;
    }
    
    auto it = m_commands.find(command.command);
    if (it == m_commands.end()) {
        return false;
    }
    
    const auto& commandDef = it->second;
    
    // 验证必需参数
    for (const auto& paramDef : commandDef.parameters) {
        if (paramDef.required) {
            auto paramIt = command.parameters.find(paramDef.name);
            if (paramIt == command.parameters.end()) {
                return false; // 缺少必需参数
            }
            
            // 验证参数类型
            if (!validateParameter(paramIt->second, paramDef.type)) {
                return false; // 参数类型不匹配
            }
        }
    }
    
    return true;
}

void CommandParser::registerCommand(const CommandDef& commandDef) {
    m_commands[commandDef.name] = commandDef;
    
    // 注册别名
    for (const auto& alias : commandDef.aliases) {
        m_aliases[alias] = commandDef.name;
    }
}

std::string CommandParser::getCommandHelp(const std::string& command) {
    auto it = m_commands.find(command);
    if (it == m_commands.end()) {
        return "";
    }
    
    const auto& commandDef = it->second;
    std::ostringstream oss;
    
    oss << "命令: " << commandDef.name << "\n";
    oss << "描述: " << commandDef.description << "\n";
    oss << "用法: " << commandDef.usage << "\n";
    
    if (!commandDef.aliases.empty()) {
        oss << "别名: ";
        for (size_t i = 0; i < commandDef.aliases.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << commandDef.aliases[i];
        }
        oss << "\n";
    }
    
    if (!commandDef.parameters.empty()) {
        oss << "参数:\n";
        for (const auto& param : commandDef.parameters) {
            oss << "  " << param.name;
            if (param.required) {
                oss << " (必需)";
            } else {
                oss << " (可选)";
            }
            oss << " - " << param.description << "\n";
        }
    }
    
    return oss.str();
}

std::vector<std::string> CommandParser::getAvailableCommands(CommandType type) {
    std::vector<std::string> commands;
    
    for (const auto& [name, commandDef] : m_commands) {
        if (type == CommandType::UNKNOWN || commandDef.type == type) {
            commands.push_back(name);
        }
    }
    
    return commands;
}

void CommandParser::addToHistory(const std::string& command) {
    if (command.empty()) return;
    
    // 避免重复的连续命令
    if (!m_history.empty() && m_history.back() == command) {
        return;
    }
    
    m_history.push_back(command);
    
    // 限制历史记录大小
    if (m_history.size() > m_maxHistorySize) {
        m_history.erase(m_history.begin());
    }
}

std::vector<std::string> CommandParser::getHistory() const {
    return m_history;
}

void CommandParser::addAlias(const std::string& alias, const std::string& command) {
    m_aliases[alias] = command;
}

void CommandParser::removeAlias(const std::string& alias) {
    m_aliases.erase(alias);
}

std::string CommandParser::resolveAlias(const std::string& input) {
    auto tokens = tokenize(input);
    if (tokens.empty()) {
        return input;
    }
    
    auto it = m_aliases.find(tokens[0]);
    if (it != m_aliases.end()) {
        tokens[0] = it->second;
        
        std::ostringstream oss;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) oss << " ";
            oss << tokens[i];
        }
        return oss.str();
    }
    
    return input;
}

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::map<std::string, std::string> CommandParser::parseParameters(const std::vector<std::string>& tokens, size_t startIndex) {
    std::map<std::string, std::string> parameters;
    
    for (size_t i = startIndex; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        
        if (token.length() > 0 && token[0] == '-') {
            std::string key = token.substr(1);
            
            if (i + 1 < tokens.size() && (tokens[i + 1].empty() || tokens[i + 1][0] != '-')) {
                // 有值的参数
                parameters[key] = tokens[i + 1];
                ++i; // 跳过值
            } else {
                // 布尔参数
                parameters[key] = "true";
            }
        }
    }
    
    return parameters;
}

bool CommandParser::validateParameter(const std::string& value, ParameterType type) {
    switch (type) {
        case ParameterType::STRING:
            return true; // 任何字符串都有效
            
        case ParameterType::INTEGER:
            try {
                std::stoi(value);
                return true;
            } catch (...) {
                return false;
            }
            
        case ParameterType::BOOLEAN:
            return value == "true" || value == "false" || value == "1" || value == "0";
            
        case ParameterType::IP_ADDRESS:
            return isValidIPAddress(value);
            
        case ParameterType::PORT_RANGE:
            return isValidPortRange(value);
            
        case ParameterType::URL:
            return isValidURL(value);
            
        default:
            return false;
    }
}

std::string CommandParser::normalizeCommand(const std::string& command) {
    std::string normalized = command;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

bool CommandParser::isValidIPAddress(const std::string& ip) {
    // 简化的IP地址验证
    std::istringstream iss(ip);
    std::string octet;
    int count = 0;
    
    while (std::getline(iss, octet, '.')) {
        if (octet.empty()) return false;
        
        try {
            int value = std::stoi(octet);
            if (value < 0 || value > 255) return false;
        } catch (...) {
            return false;
        }
        
        count++;
    }
    
    return count == 4;
}

bool CommandParser::isValidPortRange(const std::string& portRange) {
    // 简化的端口范围验证
    if (portRange.find('-') != std::string::npos) {
        // 范围格式
        auto dashPos = portRange.find('-');
        std::string start = portRange.substr(0, dashPos);
        std::string end = portRange.substr(dashPos + 1);
        
        try {
            int startPort = std::stoi(start);
            int endPort = std::stoi(end);
            return startPort >= 1 && startPort <= 65535 && 
                   endPort >= 1 && endPort <= 65535 && 
                   startPort <= endPort;
        } catch (...) {
            return false;
        }
    } else {
        // 单个端口
        try {
            int port = std::stoi(portRange);
            return port >= 1 && port <= 65535;
        } catch (...) {
            return false;
        }
    }
}

bool CommandParser::isValidURL(const std::string& url) {
    // 简化的URL验证
    return url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://";
}

void CommandParser::defineBuiltinCommands() {
    // === 系统控制命令 ===
    defineCommand("help", "显示帮助信息", "help [command/module]", {}, CommandType::BUILTIN);
    defineCommand("version", "显示版本信息", "version", {}, CommandType::BUILTIN);
    defineCommand("exit", "退出程序", "exit", {"quit"}, CommandType::BUILTIN);
    defineCommand("clear", "清空终端", "clear", {"cls"}, CommandType::BUILTIN);
    defineCommand("banner", "显示程序横幅", "banner", {}, CommandType::BUILTIN);
    defineCommand("history", "显示命令历史", "history [count]", {}, CommandType::BUILTIN);

    // === 模块管理命令 ===
    defineCommand("use", "使用指定模块", "use <module_path>", {}, CommandType::MODULE);
    defineCommand("back", "返回上一级", "back", {}, CommandType::MODULE);
    defineCommand("info", "显示模块信息", "info [module]", {}, CommandType::MODULE);
    defineCommand("search", "搜索模块", "search <keyword>", {}, CommandType::MODULE);
    defineCommand("show", "显示信息", "show <type>", {}, CommandType::MODULE);
    defineCommand("reload", "重新加载模块", "reload [module]", {}, CommandType::MODULE);

    // === 配置管理命令 ===
    defineCommand("set", "设置选项", "set <option> <value>", {}, CommandType::BUILTIN);
    defineCommand("unset", "取消设置", "unset <option>", {}, CommandType::BUILTIN);
    defineCommand("setg", "设置全局选项", "setg <option> <value>", {}, CommandType::BUILTIN);
    defineCommand("unsetg", "取消全局设置", "unsetg <option>", {}, CommandType::BUILTIN);
    defineCommand("save", "保存配置", "save [filename]", {}, CommandType::BUILTIN);
    defineCommand("load", "加载配置", "load <filename>", {}, CommandType::BUILTIN);

    // === 会话管理命令 ===
    defineCommand("sessions", "显示会话列表", "sessions", {}, CommandType::SESSION);
    defineCommand("session", "切换会话", "session <id>", {}, CommandType::SESSION);
    defineCommand("background", "后台运行", "background", {"bg"}, CommandType::SESSION);
    defineCommand("jobs", "显示任务列表", "jobs", {}, CommandType::SESSION);
    defineCommand("kill", "终止任务", "kill <job_id>", {}, CommandType::SESSION);

    // === 上下文命令 ===
    defineCommand("run", "执行模块", "run", {"exploit"}, CommandType::CONTEXT);
    defineCommand("check", "检查目标", "check", {}, CommandType::CONTEXT);
    defineCommand("rexploit", "重新执行", "rexploit", {"rerun"}, CommandType::CONTEXT);
    defineCommand("generate", "生成载荷", "generate", {}, CommandType::CONTEXT);

    // === AI命令 ===
    defineCommand("ai", "AI交互模式", "ai [command]", {}, CommandType::AI);

    // === 网络扫描命令 ===
    defineCommand("discover", "主机发现", "discover <target>", {"ping"}, CommandType::ENGINE, "network");
    defineCommand("scan", "端口扫描", "scan <target> [ports=<ports>]", {"portscan"}, CommandType::ENGINE, "network");
    defineCommand("service", "服务识别", "service <target>", {"svc"}, CommandType::ENGINE, "network");
    defineCommand("os", "操作系统识别", "os <target>", {"osdetect"}, CommandType::ENGINE, "network");

    // === 别名管理 ===
    defineCommand("alias", "创建别名", "alias <name> <command>", {}, CommandType::BUILTIN);
    defineCommand("unalias", "删除别名", "unalias <name>", {}, CommandType::BUILTIN);
}

// 辅助方法来简化命令定义
void CommandParser::defineCommand(const std::string& name, const std::string& description,
                                 const std::string& usage, const std::vector<std::string>& aliases,
                                 CommandType type) {
    CommandDef cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.usage = usage;
    cmd.aliases = aliases;
    cmd.type = type;
    registerCommand(cmd);
}

// 重载版本，支持引擎名称
void CommandParser::defineCommand(const std::string& name, const std::string& description,
                                 const std::string& usage, const std::vector<std::string>& aliases,
                                 CommandType type, const std::string& engineName) {
    CommandDef cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.usage = usage;
    cmd.aliases = aliases;
    cmd.type = type;
    cmd.engineName = engineName;
    registerCommand(cmd);
}

// 上下文管理方法实现
void CommandParser::setCurrentContext(const std::string& context) {
    m_currentContext = context;
}

std::string CommandParser::getCurrentContext() const {
    return m_currentContext;
}

void CommandParser::pushContext(const std::string& context) {
    m_contextStack.push_back(m_currentContext);
    m_currentContext = context;
}

void CommandParser::popContext() {
    if (!m_contextStack.empty()) {
        m_currentContext = m_contextStack.back();
        m_contextStack.pop_back();
    } else {
        m_currentContext.clear();
    }
}

bool CommandParser::isInModuleContext() const {
    return !m_currentContext.empty() && m_currentContext != "ai";
}

bool CommandParser::isInAIContext() const {
    return m_currentContext == "ai";
}

} // namespace MindSploit::Core
