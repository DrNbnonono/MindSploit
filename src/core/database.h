#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>
#include <functional>

namespace MindSploit::Core {

/**
 * @brief MySQL数据库管理类 - 使用MySQL存储持久化数据
 *
 * 负责管理命令历史、扫描结果、项目会话等数据的存储和检索
 * 支持自动检测MySQL服务、创建数据库和用户、初始化表结构
 */
class Database {
public:
    // 数据库连接状态
    enum class ConnectionStatus {
        NOT_CHECKED,        // 未检查
        MYSQL_NOT_FOUND,    // MySQL未安装
        MYSQL_NOT_RUNNING,  // MySQL未运行
        CONNECTION_FAILED,  // 连接失败
        DATABASE_NOT_EXIST, // 数据库不存在
        CONNECTED,          // 已连接
        INITIALIZED,        // 已初始化
        NO_DATABASE_MODE    // 无数据库模式
    };

    // 初始化结果
    struct InitResult {
        bool success = false;
        ConnectionStatus status = ConnectionStatus::NOT_CHECKED;
        QString message;
        QStringList steps;  // 初始化步骤记录
        bool noDatabaseMode = false;  // 是否为无数据库模式
    };

    static Database& instance();

    // 数据库初始化和检查
    InitResult initialize();
    InitResult initializeWithUserChoice();  // 带用户选择的初始化
    bool checkMySQLInstallation();
    bool checkMySQLService();
    bool createDatabaseAndUser();
    bool createTables();
    void close();

    // 无数据库模式
    void enableNoDatabaseMode();
    bool isNoDatabaseMode() const;
    
    // 连接状态查询
    ConnectionStatus getConnectionStatus() const;
    QString getConnectionInfo() const;
    bool isConnected() const;

    // 命令历史管理 (无数据库模式下不执行实际操作)
    bool addCommandHistory(const QString& command, const QString& output = "",
                          const QString& project = "default");
    QList<QJsonObject> getCommandHistory(const QString& project = "", int limit = 100);
    bool clearCommandHistory(const QString& project = "");

    // 扫描结果管理
    bool addScanResult(const QString& target, const QString& scanType,
                      const QJsonObject& result, const QString& project = "default");
    QList<QJsonObject> getScanResults(const QString& project = "",
                                     const QString& target = "",
                                     const QString& scanType = "");
    bool deleteScanResult(int resultId);

    // 项目会话管理
    bool createProject(const QString& projectName, const QString& description = "");
    QList<QJsonObject> getProjects();
    bool deleteProject(const QString& projectName);
    bool setCurrentProject(const QString& projectName);
    QString getCurrentProject() const;

    // AI对话记录
    bool addAIConversation(const QString& userInput, const QString& aiResponse,
                          const QString& project = "default");
    QList<QJsonObject> getAIConversations(const QString& project = "", int limit = 50);

    // 报告数据
    bool saveReport(const QString& reportName, const QJsonObject& reportData,
                   const QString& project = "default");
    QJsonObject getReport(const QString& reportName, const QString& project = "default");
    QList<QString> getReportList(const QString& project = "");

    // 统计信息
    int getCommandCount(const QString& project = "");
    int getScanCount(const QString& project = "");
    QDateTime getLastActivity(const QString& project = "");

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // 内部方法
    bool executeQuery(const QString& query, const QVariantList& params = {});
    QSqlQuery prepareQuery(const QString& query, const QVariantList& params = {});
    bool connectToMySQL(const QString& host = "localhost", int port = 3306,
                       const QString& username = "root", const QString& password = "");
    bool connectToDatabase();
    QString getMySQLVersion();
    void logStep(const QString& step, bool success = true);

    // 用户交互
    QString promptForPassword(const QString& prompt);
    bool tryConnectWithPassword();

    // MySQL服务检查
    bool isProcessRunning(const QString& processName);
    QString findMySQLExecutable();

    // 数据库配置
    struct DatabaseConfig {
        QString host = "localhost";
        int port = 3306;
        QString rootUsername = "root";
        QString rootPassword = "";
        QString databaseName = "mindsploit";
        QString username = "mindsploit";
        QString password = "mindsploit";
    } m_config;

    QSqlDatabase m_db;
    QString m_currentProject = "default";
    ConnectionStatus m_status = ConnectionStatus::NOT_CHECKED;
    QStringList m_initSteps;
    static Database* s_instance;
};

} // namespace MindSploit::Core
