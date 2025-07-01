#pragma once

#include "../../core/module_interface.h"
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QQueue>

/**
 * @brief Web应用测试工具基类 - 支持Dirb、Gobuster、SQLMap等
 * 
 * 特殊处理：
 * - 交互式输入处理
 * - HTTP会话管理
 * - 字典文件管理
 * - 结果去重和分类
 * - 进度实时显示
 */
class WebScannerBase : public ModuleInterface {
    Q_OBJECT

public:
    // Web扫描器类型
    enum WebScannerType {
        DIRB,           // 目录暴力破解
        GOBUSTER,       // Go语言目录扫描器
        SQLMAP,         // SQL注入检测
        NIKTO,          // Web漏洞扫描
        WPSCAN,         // WordPress扫描
        CUSTOM          // 自定义Web扫描器
    };

    // 扫描模式
    enum ScanMode {
        DIRECTORY_SCAN, // 目录扫描
        SUBDOMAIN_SCAN, // 子域名扫描
        VULN_SCAN,      // 漏洞扫描
        INJECTION_SCAN, // 注入测试
        BRUTE_FORCE     // 暴力破解
    };

    // 交互模式
    enum InteractionMode {
        BATCH_MODE,     // 批处理模式（非交互）
        INTERACTIVE_MODE, // 交互模式
        SEMI_AUTO_MODE  // 半自动模式
    };

    explicit WebScannerBase(WebScannerType type, QObject *parent = nullptr);
    ~WebScannerBase() override;

    // 基本信息接口实现
    QString getName() const override;
    QString getVersion() const override;
    QString getDescription() const override;
    QString getAuthor() const override { return "Various"; }
    ModuleType getType() const override { return AUXILIARY; }
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

    // Web扫描特有方法
    void setScanMode(ScanMode mode);
    void setInteractionMode(InteractionMode mode);
    void setWordlist(const QString& wordlistPath);
    void setUserAgent(const QString& userAgent);
    void setHeaders(const QStringList& headers);
    void setCookies(const QString& cookies);
    void setProxy(const QString& proxy);
    void setThreads(int threads);
    void setDelay(int milliseconds);
    void setTimeout(int seconds);
    
    // 交互式处理
    void handleInteractivePrompt(const QString& prompt, const QString& response);
    void setAutoResponses(const QHash<QString, QString>& responses);
    
    // 字典管理
    void loadBuiltinWordlists();
    QStringList getAvailableWordlists() const;
    void createCustomWordlist(const QStringList& words);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onInteractivePromptDetected();
    void onNetworkReplyFinished();

private:
    void setupScanner();
    QString buildCommand() const;
    void handleInteractiveInput();
    void processOutput(const QString& output);
    void parseResults(const QString& output);
    void detectInteractivePrompts(const QString& output);
    void sendInteractiveResponse(const QString& response);
    void validateTarget();
    void resetState();
    
    // 扫描器特定方法
    QString buildDirbCommand() const;
    QString buildGobusterCommand() const;
    QString buildSQLMapCommand() const;
    QString buildNiktoCommand() const;
    void parseDirbOutput(const QString& output);
    void parseGobusterOutput(const QString& output);
    void parseSQLMapOutput(const QString& output);
    void parseNiktoOutput(const QString& output);
    
    // 交互式处理
    void handleSQLMapPrompts(const QString& output);
    void handleNiktoPrompts(const QString& output);
    QString getAutoResponse(const QString& prompt);

    WebScannerType m_scannerType;
    ScanMode m_scanMode;
    InteractionMode m_interactionMode;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
    
    // 进程管理
    QProcess* m_process;
    QTimer* m_interactionTimer;
    QString m_outputBuffer;
    QString m_errorBuffer;
    
    // 交互式处理
    QQueue<QString> m_pendingPrompts;
    QHash<QString, QString> m_autoResponses;
    bool m_waitingForInput;
    QString m_currentPrompt;
    
    // 网络管理
    QNetworkAccessManager* m_networkManager;
    QStringList m_discoveredUrls;
    QStringList m_vulnerabilities;
    
    // 字典管理
    QStringList m_availableWordlists;
    QString m_currentWordlist;
    
    // 结果处理
    QJsonArray m_foundDirectories;
    QJsonArray m_foundFiles;
    QJsonArray m_foundVulnerabilities;
    QJsonArray m_injectionPoints;
    
    // 配置常量
    static const QHash<WebScannerType, QString> SCANNER_NAMES;
    static const QHash<WebScannerType, QStringList> SCANNER_DEPENDENCIES;
    static const QStringList BUILTIN_WORDLISTS;
    static const QHash<QString, QString> COMMON_PROMPTS;
};

// 具体工具实现
class DirbModule : public WebScannerBase {
    Q_OBJECT
public:
    explicit DirbModule(QObject *parent = nullptr) : WebScannerBase(DIRB, parent) {}
};

class GobusterModule : public WebScannerBase {
    Q_OBJECT
public:
    explicit GobusterModule(QObject *parent = nullptr) : WebScannerBase(GOBUSTER, parent) {}
};

class SQLMapModule : public WebScannerBase {
    Q_OBJECT
public:
    explicit SQLMapModule(QObject *parent = nullptr) : WebScannerBase(SQLMAP, parent) {}
    
    // SQLMap特有方法
    void setInjectionTechniques(const QStringList& techniques);
    void setDatabaseType(const QString& dbType);
    void setRiskLevel(int level);
    void setVerbosity(int level);
    void enableBatch(bool enable);
};

class NiktoModule : public WebScannerBase {
    Q_OBJECT
public:
    explicit NiktoModule(QObject *parent = nullptr) : WebScannerBase(NIKTO, parent) {}
};
