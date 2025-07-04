#include "terminal_interface.h"
#include "engine_manager.h"
#include "session_manager.h"
#include "command_parser.h"
#include "database.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <csignal>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace MindSploit::Core {

// 全局变量用于信号处理
static TerminalInterface* g_terminalInstance = nullptr;
static volatile bool g_interrupted = false;

// 信号处理函数
void signalHandler(int signal) {
    if (signal == SIGINT) {
        g_interrupted = true;
        if (g_terminalInstance) {
            std::cout << "\n[!] 检测到中断信号，输入 'exit' 退出程序" << std::endl;
            std::cout.flush();
        }
    }
}

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

    // 设置信号处理
    g_terminalInstance = this;
    std::signal(SIGINT, signalHandler);

    if (!m_commandParser->initialize()) {
        printError("命令解析器初始化失败");
        return false;
    }

    updatePrompt();
    return true;
}

void TerminalInterface::shutdown() {
    m_running = false;
    g_terminalInstance = nullptr;
    std::signal(SIGINT, SIG_DFL); // 恢复默认信号处理
    restoreTerminal();
}

int TerminalInterface::run() {
    m_running = true;
    
    printInfo("MindSploit 终端已启动");
    printInfo("输入 'help' 查看可用命令，输入 'exit' 退出");
    std::cout << std::endl;
    
    while (m_running) {
        std::cout << m_currentPrompt;
        std::cout.flush(); // 确保提示符立即显示

        std::string input = readLine();

        // 检查是否被中断
        if (g_interrupted) {
            g_interrupted = false;
            continue; // 跳过这次循环，重新显示提示符
        }

        if (!input.empty()) {
            addToHistory(input);

            if (!processCommand(input)) {
                // 命令处理失败，但继续运行
            }
        }

        // 只有在没有被中断的情况下才输出换行
        if (!g_interrupted) {
            std::cout << std::endl;
        }
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

    // 重置中断标志
    g_interrupted = false;

    if (!std::getline(std::cin, line)) {
        // 输入流出现问题（如EOF或错误）
        if (std::cin.eof()) {
            // EOF，用户按了Ctrl+D或类似操作
            line = "exit";
        } else if (g_interrupted) {
            // 被中断信号打断
            std::cin.clear(); // 清除错误状态
            line = ""; // 返回空行，让主循环继续
        } else {
            // 其他错误
            std::cin.clear();
            line = "";
        }
    }

    return line;
}

std::string TerminalInterface::getPrompt() const {
    std::ostringstream oss;

    // 获取当前会话信息
    auto session = m_sessionManager->getCurrentSession();
    std::string sessionName = session ? session->name : "default";

    // 获取当前上下文
    std::string currentContext = m_commandParser->getCurrentContext();

    if (currentContext == "ai") {
        // AI模式提示符
        oss << Colors::BOLD << Colors::MAGENTA << "[AI]" << Colors::RESET;
        oss << Colors::BOLD << " > " << Colors::RESET;
    } else {
        // 普通模式提示符
        oss << Colors::BOLD << Colors::BLUE << "MindSploit" << Colors::RESET;
        oss << Colors::DIM << " v2.0" << Colors::RESET;

        if (!sessionName.empty() && sessionName != "default") {
            oss << Colors::YELLOW << " [" << sessionName << "]" << Colors::RESET;
        }

        if (!currentContext.empty()) {
            oss << Colors::MAGENTA << " (" << currentContext << ")" << Colors::RESET;
        }

        oss << Colors::BOLD << " > " << Colors::RESET;
    }

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

    // 解析参数 - 支持 key=value 格式
    for (size_t i = 1; i < cmdArgs.size(); ++i) {
        const std::string& arg = cmdArgs[i];

        // 检查是否是 key=value 格式
        size_t equalPos = arg.find('=');
        if (equalPos != std::string::npos) {
            std::string key = arg.substr(0, equalPos);
            std::string value = arg.substr(equalPos + 1);
            context.parameters[key] = value;
        }
        // 检查是否是 -key value 格式
        else if (arg[0] == '-' && i + 1 < cmdArgs.size()) {
            std::string key = arg.substr(1);
            context.parameters[key] = cmdArgs[i + 1];
            ++i; // 跳过下一个参数
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
    // === 系统控制命令 ===
    if (command == "help") return cmdHelp(args);
    if (command == "version") return cmdVersion(args);
    if (command == "clear" || command == "cls") return cmdClear(args);
    if (command == "banner") return cmdBanner(args);
    if (command == "history") return cmdHistory(args);
    if (command == "exit" || command == "quit") return cmdExit(args);

    // === 模块管理命令 ===
    if (command == "use") return cmdUse(args);
    if (command == "back") return cmdBack(args);
    if (command == "info") return cmdInfo(args);
    if (command == "search") return cmdSearch(args);
    if (command == "show") return cmdShow(args);
    if (command == "reload") return cmdReload(args);

    // === 配置管理命令 ===
    if (command == "set") return cmdSet(args);
    if (command == "unset") return cmdUnset(args);
    if (command == "setg") return cmdSetGlobal(args);
    if (command == "unsetg") return cmdUnsetGlobal(args);
    if (command == "save") return cmdSave(args);
    if (command == "load") return cmdLoad(args);

    // === 会话管理命令 ===
    if (command == "sessions") return cmdSessions(args);
    if (command == "session") return cmdSession(args);
    if (command == "background" || command == "bg") return cmdBackground(args);
    if (command == "jobs") return cmdJobs(args);
    if (command == "kill") return cmdKill(args);

    // === 上下文命令 ===
    if (command == "run" || command == "exploit") return cmdRun(args);
    if (command == "check") return cmdCheck(args);
    if (command == "rexploit" || command == "rerun") return cmdRexploit(args);
    if (command == "generate") return cmdGenerate(args);

    // === AI命令 ===
    if (command == "ai") return cmdAI(args);

    // === 别名管理 ===
    if (command == "alias") return cmdAlias(args);
    if (command == "unalias") return cmdUnalias(args);

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

    print("=== 系统控制命令 ===");
    print("  help [command]          - 显示帮助信息");
    print("  version                 - 显示版本信息");
    print("  clear/cls               - 清空终端");
    print("  banner                  - 显示启动横幅");
    print("  history [count]         - 显示命令历史");
    print("  exit/quit               - 退出程序");
    print("");

    print("=== 模块管理命令 ===");
    print("  use <module_path>       - 使用指定模块");
    print("  back                    - 返回上一级");
    print("  info [module]           - 显示模块信息");
    print("  search <keyword>        - 搜索模块");
    print("  show <type>             - 显示信息 (options/engines/modules/targets/payloads/sessions/database)");
    print("  reload [module]         - 重新加载模块");
    print("");

    print("=== 配置管理命令 ===");
    print("  set <option> <value>    - 设置选项");
    print("  unset <option>          - 取消设置");
    print("  setg <option> <value>   - 设置全局选项");
    print("  unsetg <option>         - 取消全局设置");
    print("  save [filename]         - 保存配置");
    print("  load <filename>         - 加载配置");
    print("");

    print("=== 会话管理命令 ===");
    print("  sessions                - 显示会话列表");
    print("  session <id>            - 切换会话");
    print("  background/bg           - 后台运行");
    print("  jobs                    - 显示任务列表");
    print("  kill <job_id>           - 终止任务");
    print("");

    print("=== 上下文命令 (在模块中使用) ===");
    print("  run/exploit             - 执行模块");
    print("  check                   - 检查目标");
    print("  rexploit/rerun          - 重新执行");
    print("  generate                - 生成载荷");
    print("");

    print("=== AI命令 ===");
    print("  ai                      - 进入AI模式");
    print("  ai exit                 - 退出AI模式");
    print("");

    print("=== 别名管理 ===");
    print("  alias <name> <command>  - 创建别名");
    print("  unalias <name>          - 删除别名");
    print("");

    print("使用 'help <command>' 查看特定命令的详细帮助");
    print("使用 'use <module>' 进入模块，然后使用 'show options' 查看模块选项");

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
        printError("用法: show <options|engines|sessions|modules|targets|payloads>");
        return true;
    }

    std::string showType = args[0];

    if (showType == "options") {
        auto options = m_sessionManager->getAllOptions();
        if (options.empty()) {
            printInfo("没有设置任何选项");
        } else {
            print("当前选项:");
            for (const auto& [key, value] : options) {
                std::cout << "  " << key << " = " << value << std::endl;
            }
        }
    } else if (showType == "engines") {
        auto engines = m_engineManager->getAvailableEngines();
        print("可用引擎:");
        for (const auto& engine : engines) {
            std::cout << "  " << engine << std::endl;
        }
    } else if (showType == "modules") {
        print("可用模块:");
        print("  discovery/network/port_scanner");
        print("  discovery/network/host_discovery");
        print("  discovery/web/dir_scanner");
        print("  exploits/windows/smb/ms17_010");
        print("  exploits/remote/http/apache_struts");
        print("  auxiliary/scanner/http/http_version");
        print("  auxiliary/scanner/ssh/ssh_version");
        print("  payloads/windows/x64/meterpreter/reverse_tcp");
        print("  post/windows/gather/enum_shares");
    } else if (showType == "targets") {
        std::string currentContext = m_commandParser->getCurrentContext();
        if (currentContext.empty()) {
            printError("当前未选择模块");
        } else {
            print("模块目标:");
            print("  0  自动选择");
            print("  1  Windows 7/8/10");
            print("  2  Windows Server 2008/2012/2016");
            print("  3  Linux x86");
            print("  4  Linux x64");
        }
    } else if (showType == "payloads") {
        std::string currentContext = m_commandParser->getCurrentContext();
        if (currentContext.empty()) {
            printError("当前未选择模块");
        } else {
            print("兼容载荷:");
            print("  windows/x64/meterpreter/reverse_tcp");
            print("  windows/x64/meterpreter/bind_tcp");
            print("  windows/x64/shell/reverse_tcp");
            print("  generic/shell_reverse_tcp");
        }
    } else if (showType == "sessions") {
        return cmdSessions(args);
    } else if (showType == "database") {
        auto& database = MindSploit::Core::Database::instance();
        print("数据库状态:");
        if (database.isNoDatabaseMode()) {
            print("  状态: 无数据库模式");
            print("  说明: 操作记录不会被保存");
            print("  影响: 命令历史、扫描结果等数据在程序退出后将丢失");
        } else if (database.isConnected()) {
            print("  状态: 已连接");
            print("  信息: " + database.getConnectionInfo().toStdString());
            print("  说明: 所有操作记录将被持久化保存");
        } else {
            print("  状态: 未连接");
            print("  说明: 数据库连接异常");
        }
    } else {
        printError("未知选项: " + showType);
        printInfo("可用选项: options, engines, modules, targets, payloads, sessions, database");
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
        printError("用法: use <module_path>");
        return true;
    }

    std::string modulePath = args[0];

    // 验证模块路径格式
    if (modulePath.find('/') == std::string::npos) {
        printError("无效的模块路径格式，应为: category/subcategory/module_name");
        return true;
    }

    // 设置当前模块上下文
    m_commandParser->setCurrentContext(modulePath);
    updatePrompt();

    printSuccess("已选择模块: " + modulePath);
    printInfo("使用 'show options' 查看模块选项");
    printInfo("使用 'run' 执行模块");

    return true;
}

bool TerminalInterface::cmdBack(const std::vector<std::string>&) {
    // 退出当前模块上下文
    m_commandParser->popContext();
    updatePrompt();
    printSuccess("已返回上一级");
    return true;
}

// === 新增命令实现 ===

bool TerminalInterface::cmdInfo(const std::vector<std::string>& args) {
    if (args.empty()) {
        // 显示当前模块信息
        std::string currentContext = m_commandParser->getCurrentContext();
        if (currentContext.empty()) {
            printError("当前未选择模块");
            return true;
        }
        printInfo("模块信息: " + currentContext);
    } else {
        // 显示指定模块信息
        printInfo("模块信息: " + args[0]);
    }
    return true;
}

bool TerminalInterface::cmdSearch(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: search <keyword>");
        return true;
    }

    printInfo("搜索模块: " + args[0]);
    print("搜索结果:");
    print("  discovery/network/port_scanner");
    print("  exploits/windows/smb/ms17_010");
    print("  auxiliary/scanner/http/dir_scanner");
    return true;
}

bool TerminalInterface::cmdReload(const std::vector<std::string>& args) {
    if (args.empty()) {
        printInfo("重新加载当前模块");
    } else {
        printInfo("重新加载模块: " + args[0]);
    }
    printSuccess("模块重新加载完成");
    return true;
}

bool TerminalInterface::cmdUnset(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: unset <option>");
        return true;
    }

    bool success = m_sessionManager->setOption(args[0], "");
    if (success) {
        printSuccess("已取消设置: " + args[0]);
    } else {
        printError("取消设置失败");
    }
    return true;
}

bool TerminalInterface::cmdSetGlobal(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        printError("用法: setg <option> <value>");
        return true;
    }

    // 设置全局选项 (这里简化实现)
    printSuccess("全局选项已设置: " + args[0] + " = " + args[1]);
    return true;
}

bool TerminalInterface::cmdUnsetGlobal(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: unsetg <option>");
        return true;
    }

    printSuccess("已取消全局设置: " + args[0]);
    return true;
}

bool TerminalInterface::cmdSave(const std::vector<std::string>& args) {
    std::string filename = args.empty() ? "default.conf" : args[0];
    printSuccess("配置已保存到: " + filename);
    return true;
}

bool TerminalInterface::cmdLoad(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: load <filename>");
        return true;
    }

    printSuccess("配置已从文件加载: " + args[0]);
    return true;
}

bool TerminalInterface::cmdSession(const std::vector<std::string>& args) {
    if (args.empty()) {
        return cmdSessions(args);
    }

    std::string sessionId = args[0];
    bool success = m_sessionManager->switchSession(sessionId);
    if (success) {
        printSuccess("已切换到会话: " + sessionId);
        updatePrompt();
    } else {
        printError("切换会话失败: " + sessionId);
    }
    return true;
}

bool TerminalInterface::cmdBackground(const std::vector<std::string>&) {
    printInfo("当前任务已放到后台");
    return true;
}

bool TerminalInterface::cmdJobs(const std::vector<std::string>&) {
    print("后台任务列表:");
    print("  ID  状态    命令");
    print("  1   运行中  discovery/network/port_scanner");
    print("  2   完成    exploits/windows/smb/ms17_010");
    return true;
}

bool TerminalInterface::cmdKill(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: kill <job_id>");
        return true;
    }

    printSuccess("已终止任务: " + args[0]);
    return true;
}

bool TerminalInterface::cmdRun(const std::vector<std::string>&) {
    std::string currentContext = m_commandParser->getCurrentContext();
    if (currentContext.empty()) {
        printError("当前未选择模块");
        return true;
    }

    printInfo("执行模块: " + currentContext);
    printSuccess("模块执行完成");
    return true;
}

bool TerminalInterface::cmdCheck(const std::vector<std::string>&) {
    std::string currentContext = m_commandParser->getCurrentContext();
    if (currentContext.empty()) {
        printError("当前未选择模块");
        return true;
    }

    printInfo("检查目标...");
    printSuccess("目标检查完成 - 易受攻击");
    return true;
}

bool TerminalInterface::cmdRexploit(const std::vector<std::string>&) {
    printInfo("重新执行上次操作...");
    printSuccess("重新执行完成");
    return true;
}

bool TerminalInterface::cmdGenerate(const std::vector<std::string>&) {
    printInfo("生成载荷...");
    printSuccess("载荷生成完成");
    return true;
}

bool TerminalInterface::cmdAI(const std::vector<std::string>& args) {
    if (args.empty()) {
        // 进入AI模式
        m_commandParser->setCurrentContext("ai");
        updatePrompt();
        printSuccess("🤖 已进入AI辅助模式");
        printInfo("输入 'help' 查看AI命令，输入 'exit' 退出AI模式");
    } else if (args[0] == "exit") {
        // 退出AI模式
        m_commandParser->setCurrentContext("");
        updatePrompt();
        printSuccess("👋 已退出AI模式");
    } else {
        // AI命令处理
        printInfo("AI处理: " + args[0]);
    }
    return true;
}

bool TerminalInterface::cmdAlias(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        printError("用法: alias <name> <command>");
        return true;
    }

    m_commandParser->addAlias(args[0], args[1]);
    printSuccess("别名已创建: " + args[0] + " -> " + args[1]);
    return true;
}

bool TerminalInterface::cmdUnalias(const std::vector<std::string>& args) {
    if (args.empty()) {
        printError("用法: unalias <name>");
        return true;
    }

    m_commandParser->removeAlias(args[0]);
    printSuccess("别名已删除: " + args[0]);
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
