#pragma once

#include "../../core/module_interface.h"
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpSocket>

/**
 * @brief Metasploit集成模块 - 漏洞利用框架集成
 * 
 * 集成方式：
 * 1. msfconsole命令行接口 - 通过stdin/stdout交互
 * 2. msfvenom载荷生成 - 独立进程调用
 * 3. msfrpcd RPC接口 - 网络API调用（高级功能）
 * 
 * 支持功能：
 * - 模块搜索和信息查询
 * - 漏洞利用模块执行
 * - 载荷生成和编码
 * - 会话管理
 * - 监听器设置
 */
class MetasploitModule : public ModuleInterface {
    Q_OBJECT

public:
    // Metasploit模块类型
    enum MSFModuleType {
        EXPLOIT,        // 漏洞利用模块
        AUXILIARY,      // 辅助模块
        POST,           // 后渗透模块
        PAYLOAD,        // 载荷模块
        ENCODER,        // 编码器模块
        NOP             // 空指令模块
    };

    // 集成模式
    enum IntegrationMode {
        CONSOLE_MODE,   // msfconsole命令行模式
        VENOM_MODE,     // msfvenom载荷生成模式
        RPC_MODE        // msfrpcd RPC模式
    };

    explicit MetasploitModule(QObject *parent = nullptr);
    ~MetasploitModule() override;

    // 基本信息接口实现
    QString getName() const override { return "metasploit"; }
    QString getVersion() const override;
    QString getDescription() const override { return "Metasploit Framework Integration"; }
    QString getAuthor() const override { return "Rapid7"; }
    ModuleType getType() const override { return EXPLOIT; }
    QStringList getDependencies() const override;

    // 参数配置接口实现
    QJsonObject getDefaultOptions() const override;
    bool setOption(const QString& key, const QVariant& value) override;
    QVariant getOption(const QString& key) const override;
    QStringList getRequiredOptions() const override;
    bool validateOptions() const override;

    // 执行控制接口实现
    bool isAvailable() const override;
    bool canExecute() const override;
    void execute() override;
    void stop() override;
    ModuleStatus getStatus() const override { return m_status; }

    // 结果处理接口实现
    QJsonObject getResults() const override { return m_results; }
    QString getLastError() const override { return m_lastError; }
    QStringList getSupportedTargets() const override;

    // 帮助信息接口实现
    QString getUsage() const override;
    QStringList getExamples() const override;
    QString getHelp() const override;

    // Metasploit特有的方法
    void setIntegrationMode(IntegrationMode mode);
    void searchModules(const QString& keyword);
    void useModule(const QString& modulePath);
    void setTarget(int targetIndex);
    void setPayload(const QString& payloadName);
    void showOptions();
    void showTargets();
    void showPayloads();
    void exploit();
    void generatePayload();
    
    // 会话管理
    void listSessions();
    void interactSession(int sessionId);
    void killSession(int sessionId);
    
    // RPC模式方法
    bool connectRPC(const QString& host = "127.0.0.1", int port = 55553);
    void disconnectRPC();
    bool authenticateRPC(const QString& username, const QString& password);

private slots:
    void onConsoleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onConsoleProcessError(QProcess::ProcessError error);
    void onConsoleReadyRead();
    void onRPCConnected();
    void onRPCDisconnected();
    void onRPCError(QAbstractSocket::SocketError error);

private:
    void initializeConsoleMode();
    void initializeRPCMode();
    void sendConsoleCommand(const QString& command);
    void parseConsoleOutput(const QString& output);
    void parseModuleInfo(const QString& output);
    void parseSearchResults(const QString& output);
    void parseSessionList(const QString& output);
    QString buildVenomCommand() const;
    void resetState();

    // 控制变量
    IntegrationMode m_integrationMode;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
    
    // Console模式
    QProcess* m_consoleProcess;
    QString m_consoleBuffer;
    QString m_currentPrompt;
    bool m_consoleReady;
    
    // RPC模式
    QTcpSocket* m_rpcSocket;
    QString m_rpcToken;
    bool m_rpcAuthenticated;
    
    // 当前状态
    QString m_currentModule;
    MSFModuleType m_currentModuleType;
    QJsonObject m_moduleInfo;
    QJsonArray m_availableTargets;
    QJsonArray m_availablePayloads;
    QJsonArray m_sessions;
    
    // 常量定义
    static const QStringList MSF_COMMANDS;
    static const QHash<QString, MSFModuleType> MODULE_TYPE_MAP;
};
