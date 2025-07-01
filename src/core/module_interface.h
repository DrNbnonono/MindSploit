#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QProcess>

/**
 * @brief 模块接口基类 - 所有渗透测试工具模块的统一接口
 * 
 * 定义了所有工具模块必须实现的标准接口，确保模块间的一致性和可扩展性
 */
class ModuleInterface : public QObject {
    Q_OBJECT

public:
    // 模块类型枚举
    enum ModuleType {
        SCANNER,        // 扫描器类型 (nmap, zmap, xmap)
        EXPLOIT,        // 漏洞利用类型 (metasploit modules)
        AUXILIARY,      // 辅助工具类型 (searchsploit, curl)
        POST,           // 后渗透类型
        PAYLOAD,        // 载荷类型
        ENCODER,        // 编码器类型
        NOP             // 空指令类型
    };

    // 模块状态枚举
    enum ModuleStatus {
        READY,          // 就绪
        RUNNING,        // 运行中
        COMPLETED,      // 完成
        FAILED,         // 失败
        STOPPED         // 已停止
    };

    explicit ModuleInterface(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ModuleInterface() = default;

    // 基本信息接口
    virtual QString getName() const = 0;
    virtual QString getVersion() const = 0;
    virtual QString getDescription() const = 0;
    virtual QString getAuthor() const = 0;
    virtual ModuleType getType() const = 0;
    virtual QStringList getDependencies() const = 0;

    // 参数配置接口
    virtual QJsonObject getDefaultOptions() const = 0;
    virtual bool setOption(const QString& key, const QVariant& value) = 0;
    virtual QVariant getOption(const QString& key) const = 0;
    virtual QStringList getRequiredOptions() const = 0;
    virtual bool validateOptions() const = 0;

    // 执行控制接口
    virtual bool isAvailable() const = 0;
    virtual bool canExecute() const = 0;
    virtual void execute() = 0;
    virtual void stop() = 0;
    virtual ModuleStatus getStatus() const = 0;

    // 结果处理接口
    virtual QJsonObject getResults() const = 0;
    virtual QString getLastError() const = 0;
    virtual QStringList getSupportedTargets() const = 0;

    // 帮助信息接口
    virtual QString getUsage() const = 0;
    virtual QStringList getExamples() const = 0;
    virtual QString getHelp() const = 0;

signals:
    void statusChanged(ModuleStatus status);
    void outputReceived(const QString& output);
    void errorReceived(const QString& error);
    void progressUpdated(int percentage);
    void executionCompleted(const QJsonObject& results);
    void executionFailed(const QString& error);

    // 依赖检查方法（公开访问）
    virtual bool checkDependency(const QString& tool) const;

protected:
    // 通用工具方法
    virtual QString formatCommand(const QStringList& args) const;
    virtual QString getToolPath(const QString& tool) const;
    virtual QProcess* createProcess();

private:
    ModuleStatus m_status = READY;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
};
