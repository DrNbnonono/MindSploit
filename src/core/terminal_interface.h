#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>

namespace MindSploit::Core {

class EngineManager;
class SessionManager;
class CommandParser;

// 终端颜色定义
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
}

// 终端接口类
class TerminalInterface {
public:
    TerminalInterface(std::unique_ptr<EngineManager> engineManager,
                     std::unique_ptr<SessionManager> sessionManager);
    ~TerminalInterface();

    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // 主运行循环
    int run();
    
    // 输出方法
    void print(const std::string& message);
    void printInfo(const std::string& message);
    void printSuccess(const std::string& message);
    void printWarning(const std::string& message);
    void printError(const std::string& message);
    void printDebug(const std::string& message);
    
    // 输入方法
    std::string readLine();
    std::string readPassword();
    bool confirm(const std::string& message);
    
    // 提示符管理
    std::string getPrompt() const;
    void updatePrompt();
    
    // 命令历史
    void addToHistory(const std::string& command);
    std::vector<std::string> getHistory() const;
    
    // 自动补全
    std::vector<std::string> getCompletions(const std::string& input, size_t cursorPos);

private:
    // 命令处理
    bool processCommand(const std::string& input);
    bool executeBuiltinCommand(const std::string& command, const std::vector<std::string>& args);
    
    // 内置命令实现
    bool cmdHelp(const std::vector<std::string>& args);
    bool cmdVersion(const std::vector<std::string>& args);
    bool cmdClear(const std::vector<std::string>& args);
    bool cmdHistory(const std::vector<std::string>& args);
    bool cmdBanner(const std::vector<std::string>& args);
    bool cmdSet(const std::vector<std::string>& args);
    bool cmdShow(const std::vector<std::string>& args);
    bool cmdSessions(const std::vector<std::string>& args);
    bool cmdUse(const std::vector<std::string>& args);
    bool cmdBack(const std::vector<std::string>& args);
    bool cmdExit(const std::vector<std::string>& args);
    
    // 工具方法
    std::vector<std::string> parseInput(const std::string& input);
    std::string formatTable(const std::vector<std::vector<std::string>>& data,
                           const std::vector<std::string>& headers = {});
    void showBanner();
    void clearScreen();
    
    // 平台相关方法
    void setupTerminal();
    void restoreTerminal();
    std::string getInput();
    void enableRawMode();
    void disableRawMode();

private:
    std::unique_ptr<EngineManager> m_engineManager;
    std::unique_ptr<SessionManager> m_sessionManager;
    std::unique_ptr<CommandParser> m_commandParser;
    
    std::vector<std::string> m_commandHistory;
    size_t m_maxHistorySize = 1000;
    size_t m_historyIndex = 0;
    
    std::string m_currentPrompt;
    bool m_running = false;
    bool m_debugMode = false;
    
    // 平台相关状态
#ifdef _WIN32
    void* m_hStdin = nullptr;
    void* m_hStdout = nullptr;
    unsigned long m_originalMode = 0;
#else
    struct termios m_originalTermios;
    bool m_termiosSet = false;
#endif
};

// 输出流包装器
class TerminalStream {
public:
    enum class Type {
        INFO,
        SUCCESS,
        WARNING,
        STREAM_ERROR,
        DEBUG
    };
    
    TerminalStream(TerminalInterface* terminal, Type type);
    ~TerminalStream();
    
    template<typename T>
    TerminalStream& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }
    
    TerminalStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        m_stream << manip;
        return *this;
    }

private:
    TerminalInterface* m_terminal;
    Type m_type;
    std::ostringstream m_stream;
};

// 便利宏定义
#define TERM_INFO(terminal) TerminalStream(terminal, TerminalStream::Type::INFO)
#define TERM_SUCCESS(terminal) TerminalStream(terminal, TerminalStream::Type::SUCCESS)
#define TERM_WARNING(terminal) TerminalStream(terminal, TerminalStream::Type::WARNING)
#define TERM_ERROR(terminal) TerminalStream(terminal, TerminalStream::Type::STREAM_ERROR)
#define TERM_DEBUG(terminal) TerminalStream(terminal, TerminalStream::Type::DEBUG)

} // namespace MindSploit::Core
