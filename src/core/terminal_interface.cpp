#include "terminal_interface.h"
#include "engine_manager.h"
#include "session_manager.h"
#include "command_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace MindSploit::Core {

TerminalInterface::TerminalInterface(std::unique_ptr<EngineManager> engineManager,
                                   std::unique_ptr<SessionManager> sessionManager)
    : m_engineManager(std::move(engineManager))
    , m_sessionManager(std::move(sessionManager))
    , m_commandParser(std::make_unique<CommandParser>()) {
}

TerminalInterface::~TerminalInterface() {
    shutdown();
}

bool TerminalInterface::initialize() {
    setupTerminal();
    
    if (!m_commandParser->initialize()) {
        printError("命令解析器初始化失败");
        return false;
    }
    
    updatePrompt();
    return true;
}

void TerminalInterface::shutdown() {
    m_running = false;
    restoreTerminal();
}

int TerminalInterface::run() {
    m_running = true;
    
    printInfo("MindSploit 终端已启动");
    printInfo("输入 'help' 查看可用命令，输入 'exit' 退出");
    std::cout << std::endl;
    
    while (m_running) {
        std::cout << m_currentPrompt;
        std::string input = readLine();
        
        if (!input.empty()) {
            addToHistory(input);
            
            if (!processCommand(input)) {
                // 命令处理失败，但继续运行
            }
        }
        
        std::cout << std::endl;
    }
    
    return 0;
}

void TerminalInterface::print(const std::string& message) {
    std::cout << message << std::endl;
}

void TerminalInterface::printInfo(const std::string& message) {
    std::cout << Colors::CYAN << "[*] " << Colors::RESET << message << std::endl;
}

void TerminalInterface::printSuccess(const std::string& message) {
    std::cout << Colors::GREEN << "[+] " << Colors::RESET << message << std::endl;
}

void TerminalInterface::printWarning(const std::string& message) {
    std::cout << Colors::YELLOW << "[!] " << Colors::RESET << message << std::endl;
}

void TerminalInterface::printError(const std::string& message) {
    std::cout << Colors::RED << "[-] " << Colors::RESET << message << std::endl;
}

void TerminalInterface::printDebug(const std::string& message) {
    if (m_debugMode) {
        std::cout << Colors::DIM << "[DEBUG] " << Colors::RESET << message << std::endl;
    }
}

std::string TerminalInterface::readLine() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

std::string TerminalInterface::getPrompt() const {
    std::ostringstream oss;
    
    // 获取当前会话信息
    auto session = m_sessionManager->getCurrentSession();
    std::string sessionName = session ? session->name : "default";
    
    // 获取当前引擎
    std::string currentEngine = m_sessionManager->getCurrentEngine();
    
    oss << Colors::BOLD << Colors::BLUE << "MindSploit" << Colors::RESET;
    oss << Colors::DIM << " v2.0" << Colors::RESET;
    
    if (!sessionName.empty() && sessionName != "default") {
        oss << Colors::YELLOW << " [" << sessionName << "]" << Colors::RESET;
    }
    
    if (!currentEngine.empty()) {
        oss << Colors::MAGENTA << " (" << currentEngine << ")" << Colors::RESET;
    }
    
    oss << Colors::BOLD << " > " << Colors::RESET;
    
    return oss.str();
}

void TerminalInterface::updatePrompt() {
    m_currentPrompt = getPrompt();
}

void TerminalInterface::addToHistory(const std::string& command) {
    if (command.empty()) return;
    
    // 避免重复的连续命令
    if (!m_commandHistory.empty() && m_commandHistory.back() == command) {
        return;
    }
    
    m_commandHistory.push_back(command);
    
    // 限制历史记录大小
    if (m_commandHistory.size() > m_maxHistorySize) {
        m_commandHistory.erase(m_commandHistory.begin());
    }
    
    // 添加到会话历史
    m_sessionManager->addCommandToHistory(command);
}

std::vector<std::string> TerminalInterface::getHistory() const {
    return m_commandHistory;
}

bool TerminalInterface::processCommand(const std::string& input) {
    auto args = parseInput(input);
    if (args.empty()) {
        return true;
    }
    
    std::string command = args[0];
    std::vector<std::string> cmdArgs(args.begin() + 1, args.end());
    
    // 转换为小写进行比较
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    
    // 检查是否是内置命令
    if (executeBuiltinCommand(command, cmdArgs)) {
        return true;
    }
    
    // 尝试通过引擎管理器执行
    CommandContext context;
    context.command = command;
    context.target = cmdArgs.empty() ? "" : cmdArgs[0];
    
    // 解析参数
    for (size_t i = 0; i < cmdArgs.size(); i += 2) {
        if (i + 1 < cmdArgs.size() && !cmdArgs[i].empty() && cmdArgs[i][0] == '-') {
            std::string key = cmdArgs[i].substr(1);
            context.parameters[key] = cmdArgs[i + 1];
        }
    }
    
    // 设置输出回调
    context.outputCallback = [this](const std::string& msg) { printInfo(msg); };
    context.errorCallback = [this](const std::string& msg) { printError(msg); };
    
    auto result = m_engineManager->executeCommand(command, context);
    
    if (!result.success) {
        printError("命令执行失败: " + result.message);
        return false;
    }
    
    if (!result.message.empty()) {
        printSuccess(result.message);
    }
    
    return true;
}

bool TerminalInterface::executeBuiltinCommand(const std::string& command, const std::vector<std::string>& args) {
    if (command == "help") return cmdHelp(args);
    if (command == "version") return cmdVersion(args);
    if (command == "clear") return cmdClear(args);
    if (command == "history") return cmdHistory(args);
    if (command == "banner") return cmdBanner(args);
    if (command == "set") return cmdSet(args);
    if (command == "show") return cmdShow(args);
    if (command == "sessions") return cmdSessions(args);
    if (command == "use") return cmdUse(args);
    if (command == "back") return cmdBack(args);
    if (command == "exit" || command == "quit") return cmdExit(args);
    
    return false; // 不是内置命令
}

bool TerminalInterface::cmdHelp(const std::vector<std::string>& args) {
    if (!args.empty()) {
        // 显示特定命令的帮助
        std::string helpText = m_engineManager->getCommandHelp(args[0]);
        if (!helpText.empty()) {
            print(helpText);
        } else {
            printError("未找到命令: " + args[0]);
        }
        return true;
    }
    
    // 显示通用帮助
    print("MindSploit v2.0 - AI驱动的自主渗透测试框架");
    print("");
    print("基础命令:");
    print("  help [command]          - 显示帮助信息");
    print("  version                 - 显示版本信息");
    print("  clear                   - 清空终端");
    print("  history                 - 显示命令历史");
    print("  banner                  - 显示启动横幅");
    print("  exit/quit               - 退出程序");
    print("");
    print("会话管理:");
    print("  sessions                - 显示所有会话");
    print("  use <session>           - 切换会话");
    print("  set <option> <value>    - 设置选项");
    print("  show <options/engines>  - 显示信息");
    print("");
    print("网络扫描:");
    print("  discover <target>       - 主机发现");
    print("  scan <target> [ports]   - 端口扫描");
    print("  service <target>        - 服务识别");
    print("");
    print("使用 'help <command>' 查看特定命令的详细帮助");
    
    return true;
}

bool TerminalInterface::cmdVersion(const std::vector<std::string>&) {
    print("MindSploit v2.0.0");
    print("AI驱动的自主渗透测试框架");
    print("Copyright (c) 2024 MindSploit Team");
    return true;
}

bool TerminalInterface::cmdClear(const std::vector<std::string>&) {
    clearScreen();
    return true;
}

bool TerminalInterface::cmdHistory(const std::vector<std::string>&) {
    auto history = getHistory();
    
    if (history.empty()) {
        printInfo("命令历史为空");
        return true;
    }
    
    print("命令历史:");
    for (size_t i = 0; i < history.size(); ++i) {
        std::cout << "  " << (i + 1) << ": " << history[i] << std::endl;
    }
    
    return true;
}

bool TerminalInterface::cmdBanner(const std::vector<std::string>&) {
    showBanner();
    return true;
}

bool TerminalInterface::cmdExit(const std::vector<std::string>&) {
    printInfo("正在退出 MindSploit...");
    m_running = false;
    return true;
}

bool TerminalInterface::cmdSet(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        printError("用法: set <option> <value>");
        return true;
    }
    
    bool success = m_sessionManager->setOption(args[0], args[1]);
    if (success) {
        printSuccess("选项已设置: " + args[0] + " = " + args[1]);
    } else {
        printError("设置选项失败");
    }
    
    return true;
}

bool TerminalInterface::cmdShow(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: show <options|engines|sessions>");
        return true;
    }
    
    if (args[0] == "options") {
        auto options = m_sessionManager->getAllOptions();
        if (options.empty()) {
            printInfo("没有设置任何选项");
        } else {
            print("当前选项:");
            for (const auto& [key, value] : options) {
                std::cout << "  " << key << " = " << value << std::endl;
            }
        }
    } else if (args[0] == "engines") {
        auto engines = m_engineManager->getAvailableEngines();
        print("可用引擎:");
        for (const auto& engine : engines) {
            std::cout << "  " << engine << std::endl;
        }
    } else {
        printError("未知选项: " + args[0]);
    }
    
    return true;
}

bool TerminalInterface::cmdSessions(const std::vector<std::string>&) {
    auto sessionIds = m_sessionManager->getSessionIds();
    auto currentSession = m_sessionManager->getCurrentSession();
    
    print("会话列表:");
    for (const auto& id : sessionIds) {
        auto session = m_sessionManager->getSession(id);
        if (session) {
            std::string marker = (currentSession && currentSession->id == id) ? " *" : "  ";
            std::cout << marker << id << " (" << session->name << ")" << std::endl;
        }
    }
    
    return true;
}

bool TerminalInterface::cmdUse(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: use <session_id>");
        return true;
    }
    
    bool success = m_sessionManager->switchSession(args[0]);
    if (success) {
        printSuccess("已切换到会话: " + args[0]);
        updatePrompt();
    } else {
        printError("切换会话失败: " + args[0]);
    }
    
    return true;
}

bool TerminalInterface::cmdBack(const std::vector<std::string>&) {
    // 切换回默认会话
    auto sessionIds = m_sessionManager->getSessionIds();
    if (!sessionIds.empty()) {
        bool success = m_sessionManager->switchSession(sessionIds[0]);
        if (success) {
            printSuccess("已返回默认会话");
            updatePrompt();
        }
    }
    
    return true;
}

std::vector<std::string> TerminalInterface::parseInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void TerminalInterface::showBanner() {
    std::cout << R"(
    ███╗   ███╗██╗███╗   ██╗██████╗ ███████╗██████╗ ██╗      ██████╗ ██╗████████╗
    ████╗ ████║██║████╗  ██║██╔══██╗██╔════╝██╔══██╗██║     ██╔═══██╗██║╚══██╔══╝
    ██╔████╔██║██║██╔██╗ ██║██║  ██║███████╗██████╔╝██║     ██║   ██║██║   ██║   
    ██║╚██╔╝██║██║██║╚██╗██║██║  ██║╚════██║██╔═══╝ ██║     ██║   ██║██║   ██║   
    ██║ ╚═╝ ██║██║██║ ╚████║██████╔╝███████║██║     ███████╗╚██████╔╝██║   ██║   
    ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝     ╚══════╝ ╚═════╝ ╚═╝   ╚═╝   
    )" << std::endl;
    
    std::cout << "    MindSploit v2.0 - AI驱动的自主渗透测试框架" << std::endl;
    std::cout << "    全自研内核 | 无外部依赖 | 智能化分析" << std::endl;
    std::cout << std::endl;
}

void TerminalInterface::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void TerminalInterface::setupTerminal() {
    // 平台相关的终端设置
#ifdef _WIN32
    // Windows终端设置
#else
    // Linux终端设置
#endif
}

void TerminalInterface::restoreTerminal() {
    // 恢复终端设置
}

// TerminalStream 实现
TerminalStream::TerminalStream(TerminalInterface* terminal, Type type)
    : m_terminal(terminal), m_type(type) {
}

TerminalStream::~TerminalStream() {
    if (!m_terminal) return;

    std::string message = m_stream.str();
    if (message.empty()) return;

    switch (m_type) {
        case Type::INFO:
            m_terminal->printInfo(message);
            break;
        case Type::SUCCESS:
            m_terminal->printSuccess(message);
            break;
        case Type::WARNING:
            m_terminal->printWarning(message);
            break;
        case Type::STREAM_ERROR:
            m_terminal->printError(message);
            break;
        case Type::DEBUG:
            m_terminal->printDebug(message);
            break;
    }
}

} // namespace MindSploit::Core
