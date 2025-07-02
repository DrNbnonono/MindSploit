#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace MindSploit::Core {

// 命令类型枚举
enum class CommandType {
    BUILTIN,        // 内置命令 (help, version, set, show等)
    ENGINE,         // 引擎命令 (discover, scan, vuln等)
    AI,             // AI命令 (ai, analyze, suggest等)
    SESSION,        // 会话命令 (sessions, use, back等)
    UNKNOWN         // 未知命令
};

// 参数类型
enum class ParameterType {
    STRING,
    INTEGER,
    BOOLEAN,
    IP_ADDRESS,
    PORT_RANGE,
    FILE_PATH,
    URL
};

// 参数定义
struct ParameterDef {
    std::string name;
    ParameterType type;
    bool required = false;
    std::string defaultValue;
    std::string description;
    std::vector<std::string> allowedValues; // 对于枚举类型
};

// 命令定义
struct CommandDef {
    std::string name;
    std::string description;
    std::string usage;
    std::vector<std::string> aliases;
    std::vector<ParameterDef> parameters;
    CommandType type;
    std::string engineName; // 对于ENGINE类型命令
};

// 解析后的命令
struct ParsedCommand {
    std::string originalCommand;
    std::string command;
    std::vector<std::string> arguments;
    std::map<std::string, std::string> parameters;
    CommandType type;
    std::string engineName;
    bool isValid = false;
    std::string errorMessage;
};

// 命令补全结果
struct CompletionResult {
    std::vector<std::string> suggestions;
    std::string commonPrefix;
    bool hasExactMatch = false;
};

// 命令解析器
class CommandParser {
public:
    CommandParser();
    ~CommandParser();

    // 初始化解析器
    bool initialize();
    
    // 注册命令
    void registerCommand(const CommandDef& commandDef);
    void registerBuiltinCommands();
    void registerEngineCommands(const std::string& engineName, 
                               const std::vector<std::string>& commands);
    
    // 命令解析
    ParsedCommand parseCommand(const std::string& input);
    bool validateCommand(const ParsedCommand& command);
    
    // 命令补全
    CompletionResult getCompletions(const std::string& input, size_t cursorPos = std::string::npos);
    std::vector<std::string> getCommandSuggestions(const std::string& prefix);
    std::vector<std::string> getParameterSuggestions(const std::string& command, 
                                                    const std::string& paramPrefix);
    
    // 帮助系统
    std::string getCommandHelp(const std::string& command);
    std::string getAllCommandsHelp();
    std::vector<std::string> getAvailableCommands(CommandType type = CommandType::UNKNOWN);
    
    // 命令历史
    void addToHistory(const std::string& command);
    std::vector<std::string> getHistory() const;
    std::string getHistoryItem(int index) const; // 负数表示从末尾开始
    void clearHistory();
    
    // 别名管理
    void addAlias(const std::string& alias, const std::string& command);
    void removeAlias(const std::string& alias);
    std::string resolveAlias(const std::string& input);
    std::map<std::string, std::string> getAllAliases() const;

private:
    // 内部解析方法
    std::vector<std::string> tokenize(const std::string& input);
    std::map<std::string, std::string> parseParameters(const std::vector<std::string>& tokens, 
                                                       size_t startIndex);
    bool validateParameter(const std::string& value, ParameterType type);
    std::string normalizeCommand(const std::string& command);
    
    // 工具方法
    bool isValidIPAddress(const std::string& ip);
    bool isValidPortRange(const std::string& portRange);
    bool isValidURL(const std::string& url);
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);

private:
    std::map<std::string, CommandDef> m_commands;
    std::map<std::string, std::string> m_aliases;
    std::vector<std::string> m_history;
    size_t m_maxHistorySize = 1000;
    
    // 内置命令定义
    void defineBuiltinCommands();
    
    // 常用命令模板
    static const std::vector<CommandDef> BUILTIN_COMMANDS;
};

// 命令执行器接口
class CommandExecutor {
public:
    virtual ~CommandExecutor() = default;
    virtual bool executeCommand(const ParsedCommand& command) = 0;
    virtual std::string getExecutorName() const = 0;
};

// 内置命令执行器
class BuiltinCommandExecutor : public CommandExecutor {
public:
    bool executeCommand(const ParsedCommand& command) override;
    std::string getExecutorName() const override { return "BuiltinExecutor"; }

private:
    bool executeHelp(const ParsedCommand& command);
    bool executeVersion(const ParsedCommand& command);
    bool executeSet(const ParsedCommand& command);
    bool executeShow(const ParsedCommand& command);
    bool executeClear(const ParsedCommand& command);
    bool executeHistory(const ParsedCommand& command);
    bool executeAlias(const ParsedCommand& command);
};

} // namespace MindSploit::Core
