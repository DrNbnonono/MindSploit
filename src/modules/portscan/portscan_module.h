#pragma once

#include "../../core/module_interface.h"
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 自定义端口扫描模块 - 纯C++实现的端口扫描器
 * 
 * 支持功能：
 * - TCP端口扫描 (Connect扫描)
 * - UDP端口扫描
 * - 多线程并发扫描
 * - 端口范围扫描
 * - 服务识别
 * - 扫描速度控制
 * - 实时进度显示
 * 
 * 适用场景：
 * - Windows环境下没有nmap时的替代方案
 * - 轻量级快速扫描
 * - 集成到应用程序中的内置扫描功能
 */
class PortScanModule : public ModuleInterface {
    Q_OBJECT

public:
    explicit PortScanModule(QObject *parent = nullptr);
    ~PortScanModule() override;

    // 基本信息接口实现
    QString getName() const override { return "portscan"; }
    QString getVersion() const override { return "1.0.0"; }
    QString getDescription() const override { return "Built-in TCP/UDP port scanner"; }
    QString getAuthor() const override { return "MindSploit Team"; }
    ModuleType getType() const override { return SCANNER; }
    QStringList getDependencies() const override { return {}; } // 无外部依赖

    // 参数配置接口实现
    QJsonObject getDefaultOptions() const override;
    bool setOption(const QString& key, const QVariant& value) override;
    QVariant getOption(const QString& key) const override;
    QStringList getRequiredOptions() const override { return {"target"}; }
    bool validateOptions() const override;

    // 执行控制接口实现
    bool isAvailable() const override { return true; } // 总是可用
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

    // 端口扫描特有的方法
    void setTarget(const QString& target);
    void setPortRange(int startPort, int endPort);
    void setSpecificPorts(const QList<int>& ports);
    void setProtocol(const QString& protocol); // "tcp", "udp", "both"
    void setThreadCount(int threads);
    void setTimeout(int milliseconds);
    void setDelay(int milliseconds);
    
    // 预设扫描模式
    void quickScan(const QString& target);        // 常用端口
    void fullScan(const QString& target);         // 全端口扫描
    void topPortsScan(const QString& target);     // Top 1000端口
    void webPortsScan(const QString& target);     // Web相关端口

private slots:
    void onScanProgress(int completedPorts, int totalPorts);
    void onPortResult(const QString& host, int port, const QString& protocol, bool isOpen, const QString& service);
    void onScanCompleted();
    void onScanError(const QString& error);

private:
    class ScanWorker;
    friend class ScanWorker;
    
    void startScan();
    void stopScan();
    void resetState();
    QList<int> parsePortRange(const QString& range) const;
    QString identifyService(int port, const QString& protocol) const;
    QList<int> getCommonPorts() const;
    QList<int> getTopPorts() const;
    QList<int> getWebPorts() const;

    QThread* m_scanThread;
    ScanWorker* m_scanWorker;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
    QMutex m_resultsMutex;
    
    bool m_stopRequested;
    int m_completedPorts;
    int m_totalPorts;
    
    static const QHash<int, QString> SERVICE_MAP;
};

/**
 * @brief 端口扫描工作线程
 */
class PortScanModule::ScanWorker : public QObject {
    Q_OBJECT

public:
    explicit ScanWorker(const QString& target, const QList<int>& ports, 
                       const QString& protocol, int timeout, int delay, QObject* parent = nullptr);

public slots:
    void startScan();
    void stopScan();

signals:
    void progress(int completed, int total);
    void portResult(const QString& host, int port, const QString& protocol, bool isOpen, const QString& service);
    void scanCompleted();
    void scanError(const QString& error);

private:
    bool scanTcpPort(const QString& host, int port, int timeout);
    bool scanUdpPort(const QString& host, int port, int timeout);
    QString identifyService(int port, const QString& protocol);

    QString m_target;
    QList<int> m_ports;
    QString m_protocol;
    int m_timeout;
    int m_delay;
    bool m_stopRequested;
};
