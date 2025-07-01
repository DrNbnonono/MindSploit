#pragma once

#include "../../core/module_interface.h"
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief Nmap扫描模块 - 网络发现和端口扫描
 * 
 * 支持功能：
 * - 主机发现扫描
 * - 端口扫描 (TCP/UDP)
 * - 服务版本检测
 * - 操作系统检测
 * - 脚本扫描 (NSE)
 * - 输出格式化和解析
 */
class NmapModule : public ModuleInterface {
    Q_OBJECT

public:
    explicit NmapModule(QObject *parent = nullptr);
    ~NmapModule() override;

    // 基本信息接口实现
    QString getName() const override { return "nmap"; }
    QString getVersion() const override;
    QString getDescription() const override { return "Network Mapper - Network discovery and security auditing"; }
    QString getAuthor() const override { return "Gordon Lyon (Fyodor)"; }
    ModuleType getType() const override { return SCANNER; }
    QStringList getDependencies() const override { return {"nmap"}; }

    // 参数配置接口实现
    QJsonObject getDefaultOptions() const override;
    bool setOption(const QString& key, const QVariant& value) override;
    QVariant getOption(const QString& key) const override;
    QStringList getRequiredOptions() const override { return {"target"}; }
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

    // Nmap特有的方法
    void setTarget(const QString& target);
    void setScanType(const QString& scanType);
    void setPorts(const QString& ports);
    void setTiming(int timing);
    void enableServiceDetection(bool enable);
    void enableOSDetection(bool enable);
    void enableScriptScan(bool enable);
    void setOutputFormat(const QString& format);
    
    // 预设扫描模式
    void quickScan(const QString& target);
    void intensiveScan(const QString& target);
    void stealthScan(const QString& target);
    void comprehensiveScan(const QString& target);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProgressTimeout();

private:
    void parseNmapOutput(const QString& output);
    void parseXmlOutput(const QString& xmlData);
    void parseGrepableOutput(const QString& grepData);
    QJsonObject parseHostInfo(const QString& hostData);
    QJsonArray parsePortInfo(const QString& portData);
    QString buildCommand() const;
    void updateProgress();
    void resetState();

    QProcess* m_process;
    QTimer* m_progressTimer;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
    QString m_outputBuffer;
    QString m_errorBuffer;
    int m_progress;
    
    // Nmap特有的配置
    static const QStringList SCAN_TYPES;
    static const QStringList OUTPUT_FORMATS;
    static const QStringList TIMING_TEMPLATES;
};
