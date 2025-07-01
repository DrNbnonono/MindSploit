#pragma once

#include "../../core/module_interface.h"
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief Searchsploit模块 - 漏洞搜索和利用代码查找
 * 
 * 支持功能：
 * - 根据关键词搜索漏洞
 * - 根据CVE编号搜索
 * - 根据软件名称和版本搜索
 * - 显示漏洞详细信息
 * - 复制利用代码到本地
 */
class SearchsploitModule : public ModuleInterface {
    Q_OBJECT

public:
    explicit SearchsploitModule(QObject *parent = nullptr);
    ~SearchsploitModule() override;

    // 基本信息接口实现
    QString getName() const override { return "searchsploit"; }
    QString getVersion() const override;
    QString getDescription() const override { return "Exploit Database search tool"; }
    QString getAuthor() const override { return "Offensive Security"; }
    ModuleType getType() const override { return AUXILIARY; }
    QStringList getDependencies() const override { return {"searchsploit"}; }

    // 参数配置接口实现
    QJsonObject getDefaultOptions() const override;
    bool setOption(const QString& key, const QVariant& value) override;
    QVariant getOption(const QString& key) const override;
    QStringList getRequiredOptions() const override { return {"query"}; }
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

    // Searchsploit特有的方法
    void searchByKeyword(const QString& keyword);
    void searchByCVE(const QString& cve);
    void searchByPlatform(const QString& platform);
    void searchExact(const QString& term);
    void updateDatabase();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void parseSearchResults(const QString& output);
    QString buildCommand() const;
    void resetState();

    QProcess* m_process;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
};
