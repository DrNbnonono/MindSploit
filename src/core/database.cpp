#include "database.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSqlRecord>

Database* Database::s_instance = nullptr;

Database& Database::instance() {
    if (!s_instance) {
        s_instance = new Database();
    }
    return *s_instance;
}

Database::~Database() {
    close();
}

bool Database::initialize(const QString& dbPath) {
    qDebug() << "Starting database initialization...";

    // 检查SQLite驱动是否可用
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        qDebug() << "SQLite driver is not available!";
        qDebug() << "Available drivers:" << QSqlDatabase::drivers();
        return false;
    }

    // 确定数据库文件路径
    QString path = dbPath;
    if (path.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        qDebug() << "Data directory:" << dataDir;

        if (!QDir().mkpath(dataDir)) {
            qDebug() << "Failed to create data directory:" << dataDir;
            return false;
        }

        path = dataDir + "/mindsploit.db";
    }

    qDebug() << "Database path:" << path;

    // 创建SQLite连接
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qDebug() << "Failed to open database:" << m_db.lastError().text();
        qDebug() << "Database error type:" << m_db.lastError().type();
        qDebug() << "Database driver text:" << m_db.lastError().driverText();
        return false;
    }

    qDebug() << "Database opened successfully:" << path;
    return createTables();
}

void Database::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::createTables() {
    QStringList queries = {
        // 项目表
        R"(CREATE TABLE IF NOT EXISTS projects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            description TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // 命令历史表
        R"(CREATE TABLE IF NOT EXISTS command_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project TEXT NOT NULL DEFAULT 'default',
            command TEXT NOT NULL,
            output TEXT,
            execution_time REAL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // 扫描结果表
        R"(CREATE TABLE IF NOT EXISTS scan_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project TEXT NOT NULL DEFAULT 'default',
            target TEXT NOT NULL,
            scan_type TEXT NOT NULL,
            result_data TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // AI对话记录表
        R"(CREATE TABLE IF NOT EXISTS ai_conversations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project TEXT NOT NULL DEFAULT 'default',
            user_input TEXT NOT NULL,
            ai_response TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        ))",
        
        // 报告表
        R"(CREATE TABLE IF NOT EXISTS reports (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project TEXT NOT NULL DEFAULT 'default',
            report_name TEXT NOT NULL,
            report_data TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(project, report_name)
        ))"
    };
    
    for (const QString& query : queries) {
        if (!executeQuery(query)) {
            return false;
        }
    }
    
    // 创建默认项目
    executeQuery("INSERT OR IGNORE INTO projects (name, description) VALUES (?, ?)",
                {"default", "Default project"});
    
    return true;
}

bool Database::executeQuery(const QString& query, const QVariantList& params) {
    QSqlQuery q = prepareQuery(query, params);
    if (!q.exec()) {
        qDebug() << "Query failed:" << q.lastError().text();
        qDebug() << "Query:" << query;
        return false;
    }
    return true;
}

QSqlQuery Database::prepareQuery(const QString& query, const QVariantList& params) {
    QSqlQuery q(m_db);
    q.prepare(query);
    for (const QVariant& param : params) {
        q.addBindValue(param);
    }
    return q;
}

bool Database::addCommandHistory(const QString& command, const QString& output, const QString& project) {
    return executeQuery(
        "INSERT INTO command_history (project, command, output) VALUES (?, ?, ?)",
        {project, command, output}
    );
}

QList<QJsonObject> Database::getCommandHistory(const QString& project, int limit) {
    QString query = "SELECT * FROM command_history";
    QVariantList params;
    
    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }
    
    query += " ORDER BY created_at DESC LIMIT ?";
    params << limit;
    
    QSqlQuery q = prepareQuery(query, params);
    QList<QJsonObject> results;
    
    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["id"] = q.value("id").toInt();
            obj["project"] = q.value("project").toString();
            obj["command"] = q.value("command").toString();
            obj["output"] = q.value("output").toString();
            obj["created_at"] = q.value("created_at").toString();
            results.append(obj);
        }
    }
    
    return results;
}

bool Database::addScanResult(const QString& target, const QString& scanType, 
                           const QJsonObject& result, const QString& project) {
    QJsonDocument doc(result);
    return executeQuery(
        "INSERT INTO scan_results (project, target, scan_type, result_data) VALUES (?, ?, ?, ?)",
        {project, target, scanType, doc.toJson(QJsonDocument::Compact)}
    );
}

QList<QJsonObject> Database::getScanResults(const QString& project, const QString& target, const QString& scanType) {
    QString query = "SELECT * FROM scan_results WHERE 1=1";
    QVariantList params;
    
    if (!project.isEmpty()) {
        query += " AND project = ?";
        params << project;
    }
    if (!target.isEmpty()) {
        query += " AND target = ?";
        params << target;
    }
    if (!scanType.isEmpty()) {
        query += " AND scan_type = ?";
        params << scanType;
    }
    
    query += " ORDER BY created_at DESC";
    
    QSqlQuery q = prepareQuery(query, params);
    QList<QJsonObject> results;
    
    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["id"] = q.value("id").toInt();
            obj["project"] = q.value("project").toString();
            obj["target"] = q.value("target").toString();
            obj["scan_type"] = q.value("scan_type").toString();
            obj["created_at"] = q.value("created_at").toString();
            
            // 解析JSON数据
            QJsonDocument doc = QJsonDocument::fromJson(q.value("result_data").toByteArray());
            obj["result_data"] = doc.object();
            
            results.append(obj);
        }
    }
    
    return results;
}

bool Database::createProject(const QString& projectName, const QString& description) {
    return executeQuery(
        "INSERT INTO projects (name, description) VALUES (?, ?)",
        {projectName, description}
    );
}

QList<QJsonObject> Database::getProjects() {
    QSqlQuery q = prepareQuery("SELECT * FROM projects ORDER BY created_at DESC", {});
    QList<QJsonObject> results;
    
    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["id"] = q.value("id").toInt();
            obj["name"] = q.value("name").toString();
            obj["description"] = q.value("description").toString();
            obj["created_at"] = q.value("created_at").toString();
            results.append(obj);
        }
    }
    
    return results;
}

bool Database::setCurrentProject(const QString& projectName) {
    m_currentProject = projectName;
    return true;
}

QString Database::getCurrentProject() const {
    return m_currentProject;
}

bool Database::addAIConversation(const QString& userInput, const QString& aiResponse, const QString& project) {
    return executeQuery(
        "INSERT INTO ai_conversations (project, user_input, ai_response) VALUES (?, ?, ?)",
        {project, userInput, aiResponse}
    );
}

QList<QJsonObject> Database::getAIConversations(const QString& project, int limit) {
    QString query = "SELECT * FROM ai_conversations";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    query += " ORDER BY created_at DESC LIMIT ?";
    params << limit;

    QSqlQuery q = prepareQuery(query, params);
    QList<QJsonObject> results;

    if (q.exec()) {
        while (q.next()) {
            QJsonObject obj;
            obj["id"] = q.value("id").toInt();
            obj["project"] = q.value("project").toString();
            obj["user_input"] = q.value("user_input").toString();
            obj["ai_response"] = q.value("ai_response").toString();
            obj["created_at"] = q.value("created_at").toString();
            results.append(obj);
        }
    }

    return results;
}

bool Database::saveReport(const QString& reportName, const QJsonObject& reportData, const QString& project) {
    QJsonDocument doc(reportData);
    return executeQuery(
        "INSERT OR REPLACE INTO reports (project, report_name, report_data) VALUES (?, ?, ?)",
        {project, reportName, doc.toJson(QJsonDocument::Compact)}
    );
}

QJsonObject Database::getReport(const QString& reportName, const QString& project) {
    QSqlQuery q = prepareQuery(
        "SELECT report_data FROM reports WHERE project = ? AND report_name = ?",
        {project, reportName}
    );

    if (q.exec() && q.next()) {
        QJsonDocument doc = QJsonDocument::fromJson(q.value("report_data").toByteArray());
        return doc.object();
    }

    return QJsonObject();
}

int Database::getCommandCount(const QString& project) {
    QString query = "SELECT COUNT(*) FROM command_history";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    QSqlQuery q = prepareQuery(query, params);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }

    return 0;
}
