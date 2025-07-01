#pragma once

#include "../../core/module_interface.h"
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief Curl模块 - HTTP/HTTPS请求和Web应用测试
 * 
 * 支持功能：
 * - HTTP/HTTPS请求发送
 * - 自定义请求头和参数
 * - 文件上传和下载
 * - Cookie管理
 * - 代理支持
 * - SSL/TLS配置
 * - 响应分析和格式化
 */
class CurlModule : public ModuleInterface {
    Q_OBJECT

public:
    explicit CurlModule(QObject *parent = nullptr);
    ~CurlModule() override;

    // 基本信息接口实现
    QString getName() const override { return "curl"; }
    QString getVersion() const override;
    QString getDescription() const override { return "Command line tool for transferring data with URLs"; }
    QString getAuthor() const override { return "Daniel Stenberg"; }
    ModuleType getType() const override { return AUXILIARY; }
    QStringList getDependencies() const override { return {"curl"}; }

    // 参数配置接口实现
    QJsonObject getDefaultOptions() const override;
    bool setOption(const QString& key, const QVariant& value) override;
    QVariant getOption(const QString& key) const override;
    QStringList getRequiredOptions() const override { return {"url"}; }
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

    // Curl特有的方法
    void setUrl(const QString& url);
    void setMethod(const QString& method);
    void addHeader(const QString& header);
    void setData(const QString& data);
    void setUserAgent(const QString& userAgent);
    void setProxy(const QString& proxy);
    void setFollowRedirects(bool follow);
    void setInsecure(bool insecure);
    void setTimeout(int seconds);
    
    // 预设请求模式
    void simpleGet(const QString& url);
    void postData(const QString& url, const QString& data);
    void uploadFile(const QString& url, const QString& filePath);
    void downloadFile(const QString& url, const QString& outputPath);
    void testHeaders(const QString& url);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void parseResponse(const QString& output);
    QString buildCommand() const;
    void resetState();

    QProcess* m_process;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
    
    static const QStringList HTTP_METHODS;
};
