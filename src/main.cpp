#include <iostream>
#include <string>
#include <memory>
#include <csignal>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

#include "core/terminal_interface.h"
#include "core/engine_manager.h"
#include "core/session_manager.h"

// 全局变量用于信号处理
std::unique_ptr<MindSploit::Core::TerminalInterface> g_terminal;

// 信号处理函数
void signalHandler(int signal) {
    if (signal == SIGINT) {
        // Ctrl+C 只是中断当前输入，不退出程序
        std::cout << "\n";
        if (g_terminal) {
            std::cout << g_terminal->getPrompt();
            std::cout.flush();
        }
        return;
    }

    // 其他信号才退出程序
    if (g_terminal) {
        std::cout << "\n[!] 接收到退出信号，正在安全退出..." << std::endl;
        g_terminal->shutdown();
    }
    std::exit(signal);
}

// 设置控制台编码 (Windows)
void setupConsole() {
#ifdef _WIN32
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 启用ANSI转义序列支持
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

// 显示启动横幅
void showBanner() {
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
    std::cout << "    Copyright (c) 2024 MindSploit Team" << std::endl;
    std::cout << std::endl;
    std::cout << "    输入 'help' 查看可用命令" << std::endl;
    std::cout << "    输入 'banner' 重新显示此横幅" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    try {
        // 设置控制台
        setupConsole();

        // 注册信号处理
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // 显示启动横幅
        showBanner();

        // 初始化核心组件
        auto engineManager = std::make_unique<MindSploit::Core::EngineManager>();
        auto sessionManager = std::make_unique<MindSploit::Core::SessionManager>();

        if (!engineManager->initialize()) {
            std::cerr << "[!] 引擎管理器初始化失败" << std::endl;
            return -1;
        }

        if (!sessionManager->initialize()) {
            std::cerr << "[!] 会话管理器初始化失败" << std::endl;
            return -1;
        }

        // 创建终端接口
        g_terminal = std::make_unique<MindSploit::Core::TerminalInterface>(
            std::move(engineManager), std::move(sessionManager));

        if (!g_terminal->initialize()) {
            std::cerr << "[!] 终端接口初始化失败" << std::endl;
            return -1;
        }

        std::cout << "[+] MindSploit 初始化完成" << std::endl;
        std::cout << std::endl;

        // 启动主循环
        int exitCode = g_terminal->run();

        std::cout << "[+] MindSploit 已安全退出" << std::endl;
        return exitCode;

    } catch (const std::exception& e) {
        std::cerr << "[!] 异常: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "[!] 未知异常" << std::endl;
        return -1;
    }
}