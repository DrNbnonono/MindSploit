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
    // 定义内置命令
    CommandDef helpCmd;
    helpCmd.name = "help";
    helpCmd.description = "显示帮助信息";
    helpCmd.usage = "help [command]";
    helpCmd.type = CommandType::BUILTIN;
    registerCommand(helpCmd);
    
    CommandDef versionCmd;
    versionCmd.name = "version";
    versionCmd.description = "显示版本信息";
    versionCmd.usage = "version";
    versionCmd.type = CommandType::BUILTIN;
    registerCommand(versionCmd);
    
    CommandDef clearCmd;
    clearCmd.name = "clear";
    clearCmd.description = "清空终端";
    clearCmd.usage = "clear";
    clearCmd.aliases = {"cls"};
    clearCmd.type = CommandType::BUILTIN;
    registerCommand(clearCmd);
    
    CommandDef exitCmd;
    exitCmd.name = "exit";
    exitCmd.description = "退出程序";
    exitCmd.usage = "exit";
    exitCmd.aliases = {"quit"};
    exitCmd.type = CommandType::BUILTIN;
    registerCommand(exitCmd);
}

} // namespace MindSploit::Core
