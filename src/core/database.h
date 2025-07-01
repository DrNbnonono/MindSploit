#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>

/**
 * @brief 数据库管理类 - 使用SQLite存储结构化数据
 * 
 * 负责管理命令历史、扫描结果、项目会话等数据的存储和检索
 */
class Database {
public:
    static Database& instance();
    
    // 数据库初始化
    bool initialize(const QString& dbPath = "");
    void close();
    
    // 命令历史管理
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
    
    bool createTables();
    bool executeQuery(const QString& query, const QVariantList& params = {});
    QSqlQuery prepareQuery(const QString& query, const QVariantList& params = {});
    
    QSqlDatabase m_db;
    QString m_currentProject = "default";
    static Database* s_instance;
};
