#include "database.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSqlRecord>
#include <QJsonArray>
#include <QCoreApplication>
#include <QThread>
#include <QProcess>
#include <QFile>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace MindSploit::Core {

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

Database::InitResult Database::initialize() {
    InitResult result;
    m_initSteps.clear();
    
    std::cout << "\n=== MindSploit 数据库初始化 ===" << std::endl;
    std::cout.flush();
    
    // 步骤1: 检查MySQL安装
    logStep("检查MySQL安装状态...");
    if (!checkMySQLInstallation()) {
        result.status = ConnectionStatus::MYSQL_NOT_FOUND;
        result.message = "MySQL未安装或未找到可执行文件";
        logStep("MySQL未安装", false);
        std::cout << "\n❌ 错误: MySQL未安装" << std::endl;
        std::cout << "请安装MySQL服务器后重新启动程序" << std::endl;
        std::cout << "下载地址: https://dev.mysql.com/downloads/mysql/" << std::endl;
        return result;
    }
    logStep("MySQL已安装");
    
    // 步骤2: 检查MySQL服务状态
    logStep("检查MySQL服务状态...");
    if (!checkMySQLService()) {
        result.status = ConnectionStatus::MYSQL_NOT_RUNNING;
        result.message = "MySQL服务未运行";
        logStep("MySQL服务未运行", false);
        std::cout << "\n❌ 错误: MySQL服务未运行" << std::endl;
        std::cout << "请启动MySQL服务后重新启动程序" << std::endl;
        std::cout << "Windows: net start mysql" << std::endl;
        std::cout << "Linux: sudo systemctl start mysql" << std::endl;
        return result;
    }
    logStep("MySQL服务正在运行");
    
    // 步骤3: 检查MySQL驱动
    logStep("检查MySQL驱动...");
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        logStep("MySQL驱动不可用", false);

        std::cout << "\n❌ 错误: MySQL驱动不可用" << std::endl;
        std::cout << "当前可用驱动: ";
        for (const QString& driver : QSqlDatabase::drivers()) {
            std::cout << driver.toStdString() << " ";
        }
        std::cout << std::endl;
        std::cout << "\n解决方案:" << std::endl;
        std::cout << "1. 安装包含MySQL驱动的Qt版本" << std::endl;
        std::cout << "2. 手动编译Qt MySQL驱动" << std::endl;
        std::cout << "3. 使用无数据库模式继续运行" << std::endl;
        std::cout << "\n详细说明:" << std::endl;
        std::cout << "Qt MySQL驱动需要单独安装或编译" << std::endl;
        std::cout << "参考: https://doc.qt.io/qt-6/sql-driver.html#qmysql" << std::endl;

        // 询问用户是否继续
        std::cout << "\n是否在无数据库模式下继续启动? (y/N): ";
        std::cout.flush();
        std::string userInput;
        std::getline(std::cin, userInput);

        if (userInput == "y" || userInput == "Y" || userInput == "yes" || userInput == "YES") {
            enableNoDatabaseMode();
            result.success = true;
            result.status = ConnectionStatus::NO_DATABASE_MODE;
            result.message = "无数据库模式启动 (MySQL驱动不可用)";
            result.noDatabaseMode = true;

            std::cout << "\n⚠️  已启用无数据库模式" << std::endl;
            std::cout << "注意: 所有操作记录将不会被保存" << std::endl;
            std::cout << "扫描结果、命令历史等数据在程序退出后将丢失" << std::endl;
        } else {
            result.status = ConnectionStatus::CONNECTION_FAILED;
            result.message = "MySQL驱动不可用";
        }

        return result;
    }
    logStep("MySQL驱动可用");

    // 步骤4: 连接到MySQL
    logStep("连接到MySQL服务器...");
    if (!tryConnectWithPassword()) {
        result.status = ConnectionStatus::CONNECTION_FAILED;
        result.message = "无法连接到MySQL服务器";
        logStep("连接MySQL失败", false);

        // 询问用户是否继续
        std::cout << "\n是否在无数据库模式下继续启动? (y/N): ";
        std::string userInput;
        std::getline(std::cin, userInput);

        if (userInput == "y" || userInput == "Y" || userInput == "yes" || userInput == "YES") {
            enableNoDatabaseMode();
            result.success = true;
            result.status = ConnectionStatus::NO_DATABASE_MODE;
            result.message = "无数据库模式启动";
            result.noDatabaseMode = true;

            std::cout << "\n⚠️  已启用无数据库模式" << std::endl;
            std::cout << "注意: 所有操作记录将不会被保存" << std::endl;
            std::cout << "扫描结果、命令历史等数据在程序退出后将丢失" << std::endl;
        }

        return result;
    }
    logStep("成功连接到MySQL服务器");
    
    // 步骤5: 创建数据库和用户
    logStep("检查/创建MindSploit数据库...");
    if (!createDatabaseAndUser()) {
        result.status = ConnectionStatus::DATABASE_NOT_EXIST;
        result.message = "无法创建数据库或用户";
        logStep("创建数据库失败", false);
        return result;
    }
    logStep("数据库和用户创建完成");

    // 步骤6: 连接到MindSploit数据库
    logStep("连接到MindSploit数据库...");
    if (!connectToDatabase()) {
        result.status = ConnectionStatus::CONNECTION_FAILED;
        result.message = "无法连接到MindSploit数据库";
        logStep("连接数据库失败", false);
        return result;
    }
    logStep("成功连接到MindSploit数据库");

    // 步骤7: 创建表结构
    logStep("创建/检查数据表结构...");
    if (!createTables()) {
        result.message = "无法创建数据表";
        logStep("创建表结构失败", false);
        return result;
    }
    logStep("数据表结构创建完成");
    
    m_status = ConnectionStatus::INITIALIZED;
    result.success = true;
    result.status = ConnectionStatus::INITIALIZED;
    result.message = "数据库初始化成功";
    result.steps = m_initSteps;
    
    std::cout << "\n✅ 数据库初始化完成!" << std::endl;
    std::cout << "数据库: " << m_config.databaseName.toStdString() << std::endl;
    std::cout << "用户: " << m_config.username.toStdString() << std::endl;
    std::cout << "主机: " << m_config.host.toStdString() << ":" << m_config.port << std::endl;
    
    return result;
}

Database::InitResult Database::initializeWithUserChoice() {
    InitResult result;
    m_initSteps.clear();

    std::cout << "\n=== MindSploit 数据库初始化 ===" << std::endl;

    // 步骤1: 检查MySQL安装
    logStep("检查MySQL安装状态...");
    if (!checkMySQLInstallation()) {
        result.status = ConnectionStatus::MYSQL_NOT_FOUND;
        result.message = "MySQL未安装或未找到可执行文件";
        logStep("MySQL未安装", false);

        std::cout << "\n❌ 错误: MySQL未安装" << std::endl;
        std::cout << "请安装MySQL服务器后重新启动程序" << std::endl;
        std::cout << "下载地址: https://dev.mysql.com/downloads/mysql/" << std::endl;

        // 询问用户是否继续
        std::cout << "\n是否在无数据库模式下继续启动? (y/N): ";
        std::string userInput;
        std::getline(std::cin, userInput);

        if (userInput == "y" || userInput == "Y" || userInput == "yes" || userInput == "YES") {
            enableNoDatabaseMode();
            result.success = true;
            result.status = ConnectionStatus::NO_DATABASE_MODE;
            result.message = "无数据库模式启动";
            result.noDatabaseMode = true;

            std::cout << "\n⚠️  已启用无数据库模式" << std::endl;
            std::cout << "注意: 所有操作记录将不会被保存" << std::endl;
            std::cout << "扫描结果、命令历史等数据在程序退出后将丢失" << std::endl;
        }

        return result;
    }
    logStep("MySQL已安装");

    // 步骤2: 检查MySQL服务状态
    logStep("检查MySQL服务状态...");
    if (!checkMySQLService()) {
        result.status = ConnectionStatus::MYSQL_NOT_RUNNING;
        result.message = "MySQL服务未运行";
        logStep("MySQL服务未运行", false);

        std::cout << "\n❌ 错误: MySQL服务未运行" << std::endl;
        std::cout << "请启动MySQL服务后重新启动程序" << std::endl;
        std::cout << "Windows: net start mysql" << std::endl;
        std::cout << "Linux: sudo systemctl start mysql" << std::endl;

        // 询问用户是否继续
        std::cout << "\n是否在无数据库模式下继续启动? (y/N): ";
        std::string userInput;
        std::getline(std::cin, userInput);

        if (userInput == "y" || userInput == "Y" || userInput == "yes" || userInput == "YES") {
            enableNoDatabaseMode();
            result.success = true;
            result.status = ConnectionStatus::NO_DATABASE_MODE;
            result.message = "无数据库模式启动";
            result.noDatabaseMode = true;

            std::cout << "\n⚠️  已启用无数据库模式" << std::endl;
            std::cout << "注意: 所有操作记录将不会被保存" << std::endl;
            std::cout << "扫描结果、命令历史等数据在程序退出后将丢失" << std::endl;
        }

        return result;
    }
    logStep("MySQL服务正在运行");

    // 继续正常的数据库初始化流程
    return initialize();
}

bool Database::checkMySQLInstallation() {
    // 检查MySQL可执行文件
    QString mysqlPath = findMySQLExecutable();
    if (mysqlPath.isEmpty()) {
        return false;
    }
    
    std::cout << "找到MySQL: " << mysqlPath.toStdString() << std::endl;
    return true;
}

bool Database::checkMySQLService() {
    // 检查MySQL进程是否运行
    return isProcessRunning("mysqld") || isProcessRunning("mysql");
}

bool Database::createDatabaseAndUser() {
    std::cout << "创建数据库和用户..." << std::endl;
    
    // 创建数据库
    QString createDbQuery = QString("CREATE DATABASE IF NOT EXISTS %1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
                           .arg(m_config.databaseName);
    
    if (!executeQuery(createDbQuery)) {
        std::cout << "创建数据库失败" << std::endl;
        return false;
    }
    std::cout << "数据库 '" << m_config.databaseName.toStdString() << "' 创建成功" << std::endl;

    // 创建用户
    QString createUserQuery = QString("CREATE USER IF NOT EXISTS '%1'@'localhost' IDENTIFIED BY '%2'")
                             .arg(m_config.username, m_config.password);

    if (!executeQuery(createUserQuery)) {
        std::cout << "创建用户失败" << std::endl;
        return false;
    }
    std::cout << "用户 '" << m_config.username.toStdString() << "' 创建成功" << std::endl;
    
    // 授权
    QString grantQuery = QString("GRANT ALL PRIVILEGES ON %1.* TO '%2'@'localhost'")
                        .arg(m_config.databaseName, m_config.username);
    
    if (!executeQuery(grantQuery)) {
        std::cout << "用户授权失败" << std::endl;
        return false;
    }
    
    executeQuery("FLUSH PRIVILEGES");
    std::cout << "用户权限设置完成" << std::endl;
    
    return true;
}

bool Database::connectToMySQL(const QString& host, int port, const QString& username, const QString& password) {
    // 检查MySQL驱动
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        std::cout << "\n❌ 错误: MySQL驱动不可用" << std::endl;
        std::cout << "当前可用驱动: ";
        for (const QString& driver : QSqlDatabase::drivers()) {
            std::cout << driver.toStdString() << " ";
        }
        std::cout << std::endl;
        std::cout << "\n解决方案:" << std::endl;
        std::cout << "1. 安装包含MySQL驱动的Qt版本" << std::endl;
        std::cout << "2. 手动编译Qt MySQL驱动" << std::endl;
        std::cout << "3. 使用无数据库模式继续运行" << std::endl;
        std::cout << "\n详细说明:" << std::endl;
        std::cout << "Qt MySQL驱动需要单独安装或编译" << std::endl;
        std::cout << "参考: https://doc.qt.io/qt-6/sql-driver.html#qmysql" << std::endl;
        return false;
    }
    
    // 创建连接
    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setUserName(username);
    m_db.setPassword(password);
    
    if (!m_db.open()) {
        std::cout << "连接MySQL失败: " << m_db.lastError().text().toStdString() << std::endl;
        return false;
    }
    
    std::cout << "成功连接到MySQL服务器 " << host.toStdString() << ":" << port << std::endl;
    return true;
}

bool Database::connectToDatabase() {
    m_db.close();
    
    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(m_config.host);
    m_db.setPort(m_config.port);
    m_db.setDatabaseName(m_config.databaseName);
    m_db.setUserName(m_config.username);
    m_db.setPassword(m_config.password);
    
    if (!m_db.open()) {
        std::cout << "连接数据库失败: " << m_db.lastError().text().toStdString() << std::endl;
        return false;
    }
    
    return true;
}

void Database::logStep(const QString& step, bool success) {
    QString status = success ? "✅" : "❌";
    QString logEntry = QString("%1 %2").arg(status, step);
    m_initSteps.append(logEntry);
    std::cout << logEntry.toStdString() << std::endl;
    std::cout.flush();
}

Database::ConnectionStatus Database::getConnectionStatus() const {
    return m_status;
}

QString Database::getConnectionInfo() const {
    return QString("Host: %1:%2, Database: %3, User: %4")
           .arg(m_config.host)
           .arg(m_config.port)
           .arg(m_config.databaseName)
           .arg(m_config.username);
}

bool Database::isConnected() const {
    return m_db.isOpen() && m_status == ConnectionStatus::INITIALIZED;
}

void Database::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_status = ConnectionStatus::NOT_CHECKED;
}

void Database::enableNoDatabaseMode() {
    m_status = ConnectionStatus::NO_DATABASE_MODE;
    std::cout << "数据库模式: 无数据库模式 (操作不会被记录)" << std::endl;
}

bool Database::isNoDatabaseMode() const {
    return m_status == ConnectionStatus::NO_DATABASE_MODE;
}

bool Database::createTables() {
    std::cout << "创建数据表..." << std::endl;

    // 命令历史表
    QString commandHistoryTable = R"(
        CREATE TABLE IF NOT EXISTS command_history (
            id INT AUTO_INCREMENT PRIMARY KEY,
            project VARCHAR(100) NOT NULL DEFAULT 'default',
            command TEXT NOT NULL,
            output LONGTEXT,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_project (project),
            INDEX idx_timestamp (timestamp)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!executeQuery(commandHistoryTable)) {
        std::cout << "创建命令历史表失败" << std::endl;
        return false;
    }
    std::cout << "命令历史表创建完成" << std::endl;

    // 扫描结果表
    QString scanResultsTable = R"(
        CREATE TABLE IF NOT EXISTS scan_results (
            id INT AUTO_INCREMENT PRIMARY KEY,
            project VARCHAR(100) NOT NULL DEFAULT 'default',
            target VARCHAR(255) NOT NULL,
            scan_type VARCHAR(50) NOT NULL,
            result LONGTEXT NOT NULL,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_project (project),
            INDEX idx_target (target),
            INDEX idx_scan_type (scan_type),
            INDEX idx_timestamp (timestamp)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!executeQuery(scanResultsTable)) {
        std::cout << "创建扫描结果表失败" << std::endl;
        return false;
    }
    std::cout << "扫描结果表创建完成" << std::endl;

    // 项目表
    QString projectsTable = R"(
        CREATE TABLE IF NOT EXISTS projects (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100) NOT NULL UNIQUE,
            description TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            INDEX idx_name (name)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!executeQuery(projectsTable)) {
        std::cout << "创建项目表失败" << std::endl;
        return false;
    }
    std::cout << "项目表创建完成" << std::endl;

    // AI对话表
    QString aiConversationsTable = R"(
        CREATE TABLE IF NOT EXISTS ai_conversations (
            id INT AUTO_INCREMENT PRIMARY KEY,
            project VARCHAR(100) NOT NULL DEFAULT 'default',
            user_input TEXT NOT NULL,
            ai_response LONGTEXT NOT NULL,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            INDEX idx_project (project),
            INDEX idx_timestamp (timestamp)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!executeQuery(aiConversationsTable)) {
        std::cout << "创建AI对话表失败" << std::endl;
        return false;
    }
    std::cout << "AI对话表创建完成" << std::endl;

    // 报告表
    QString reportsTable = R"(
        CREATE TABLE IF NOT EXISTS reports (
            id INT AUTO_INCREMENT PRIMARY KEY,
            project VARCHAR(100) NOT NULL DEFAULT 'default',
            name VARCHAR(255) NOT NULL,
            data LONGTEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            INDEX idx_project (project),
            INDEX idx_name (name)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
    )";

    if (!executeQuery(reportsTable)) {
        std::cout << "创建报告表失败" << std::endl;
        return false;
    }
    std::cout << "报告表创建完成" << std::endl;

    // 插入默认项目
    QString insertDefaultProject = "INSERT IGNORE INTO projects (name, description) VALUES ('default', '默认项目')";
    executeQuery(insertDefaultProject);

    std::cout << "所有数据表创建完成" << std::endl;
    return true;
}

bool Database::executeQuery(const QString& query, const QVariantList& params) {
    QSqlQuery sqlQuery = prepareQuery(query, params);

    if (!sqlQuery.exec()) {
        std::cout << "SQL执行失败: " << sqlQuery.lastError().text().toStdString() << std::endl;
        std::cout << "查询: " << query.toStdString() << std::endl;
        return false;
    }

    return true;
}

QSqlQuery Database::prepareQuery(const QString& query, const QVariantList& params) {
    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);

    for (int i = 0; i < params.size(); ++i) {
        sqlQuery.bindValue(i, params[i]);
    }

    return sqlQuery;
}

bool Database::isProcessRunning(const QString& processName) {
#ifdef _WIN32
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << QString("IMAGENAME eq %1.exe").arg(processName));
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    return output.contains(processName, Qt::CaseInsensitive);
#else
    QProcess process;
    process.start("pgrep", QStringList() << processName);
    process.waitForFinished();

    return process.exitCode() == 0;
#endif
}

QString Database::findMySQLExecutable() {
    QStringList possiblePaths;

#ifdef _WIN32
    possiblePaths << "C:/Program Files/MySQL/MySQL Server 8.0/bin/mysql.exe"
                  << "C:/Program Files/MySQL/MySQL Server 5.7/bin/mysql.exe"
                  << "C:/MySQL/bin/mysql.exe"
                  << "C:/xampp/mysql/bin/mysql.exe";
#else
    possiblePaths << "/usr/bin/mysql"
                  << "/usr/local/bin/mysql"
                  << "/opt/mysql/bin/mysql";
#endif

    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    // 尝试在PATH中查找
    QProcess process;
    process.start("mysql", QStringList() << "--version");
    process.waitForFinished();

    if (process.exitCode() == 0) {
        return "mysql"; // 在PATH中找到
    }

    return QString();
}

// 数据库操作方法实现 (支持无数据库模式)
bool Database::addCommandHistory(const QString& command, const QString& output, const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QString query = "INSERT INTO command_history (project, command, output) VALUES (?, ?, ?)";
    return executeQuery(query, {project, command, output});
}

QList<QJsonObject> Database::getCommandHistory(const QString& project, int limit) {
    QList<QJsonObject> result;

    if (isNoDatabaseMode()) {
        // 无数据库模式下返回空列表
        return result;
    }

    if (!isConnected()) {
        return result;
    }

    QString query = "SELECT * FROM command_history";
    QStringList conditions;
    QVariantList params;

    if (!project.isEmpty()) {
        conditions << "project = ?";
        params << project;
    }

    if (!conditions.isEmpty()) {
        query += " WHERE " + conditions.join(" AND ");
    }

    query += " ORDER BY timestamp DESC LIMIT ?";
    params << limit;

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            QJsonObject obj;
            obj["id"] = sqlQuery.value("id").toInt();
            obj["project"] = sqlQuery.value("project").toString();
            obj["command"] = sqlQuery.value("command").toString();
            obj["output"] = sqlQuery.value("output").toString();
            obj["timestamp"] = sqlQuery.value("timestamp").toString();
            result.append(obj);
        }
    }

    return result;
}

bool Database::clearCommandHistory(const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QString query = "DELETE FROM command_history";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    return executeQuery(query, params);
}

bool Database::addScanResult(const QString& target, const QString& scanType, const QJsonObject& result, const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QJsonDocument doc(result);
    QString resultJson = doc.toJson(QJsonDocument::Compact);

    QString query = "INSERT INTO scan_results (project, target, scan_type, result) VALUES (?, ?, ?, ?)";
    return executeQuery(query, {project, target, scanType, resultJson});
}

QList<QJsonObject> Database::getScanResults(const QString& project, const QString& target, const QString& scanType) {
    QList<QJsonObject> result;

    if (isNoDatabaseMode()) {
        // 无数据库模式下返回空列表
        return result;
    }

    if (!isConnected()) {
        return result;
    }

    QString query = "SELECT * FROM scan_results";
    QStringList conditions;
    QVariantList params;

    if (!project.isEmpty()) {
        conditions << "project = ?";
        params << project;
    }

    if (!target.isEmpty()) {
        conditions << "target = ?";
        params << target;
    }

    if (!scanType.isEmpty()) {
        conditions << "scan_type = ?";
        params << scanType;
    }

    if (!conditions.isEmpty()) {
        query += " WHERE " + conditions.join(" AND ");
    }

    query += " ORDER BY timestamp DESC";

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            QJsonObject obj;
            obj["id"] = sqlQuery.value("id").toInt();
            obj["project"] = sqlQuery.value("project").toString();
            obj["target"] = sqlQuery.value("target").toString();
            obj["scan_type"] = sqlQuery.value("scan_type").toString();

            QJsonDocument doc = QJsonDocument::fromJson(sqlQuery.value("result").toString().toUtf8());
            obj["result"] = doc.object();
            obj["timestamp"] = sqlQuery.value("timestamp").toString();
            result.append(obj);
        }
    }

    return result;
}

bool Database::deleteScanResult(int resultId) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QString query = "DELETE FROM scan_results WHERE id = ?";
    return executeQuery(query, {resultId});
}

bool Database::createProject(const QString& projectName, const QString& description) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QString query = "INSERT INTO projects (name, description) VALUES (?, ?)";
    return executeQuery(query, {projectName, description});
}

QList<QJsonObject> Database::getProjects() {
    QList<QJsonObject> result;

    if (isNoDatabaseMode()) {
        // 无数据库模式下返回默认项目
        QJsonObject defaultProject;
        defaultProject["name"] = "default";
        defaultProject["description"] = "默认项目 (无数据库模式)";
        result.append(defaultProject);
        return result;
    }

    if (!isConnected()) {
        return result;
    }

    QString query = "SELECT * FROM projects ORDER BY created_at DESC";
    QSqlQuery sqlQuery = prepareQuery(query);

    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            QJsonObject obj;
            obj["id"] = sqlQuery.value("id").toInt();
            obj["name"] = sqlQuery.value("name").toString();
            obj["description"] = sqlQuery.value("description").toString();
            obj["created_at"] = sqlQuery.value("created_at").toString();
            obj["updated_at"] = sqlQuery.value("updated_at").toString();
            result.append(obj);
        }
    }

    return result;
}

bool Database::deleteProject(const QString& projectName) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QString query = "DELETE FROM projects WHERE name = ?";
    return executeQuery(query, {projectName});
}

bool Database::setCurrentProject(const QString& projectName) {
    m_currentProject = projectName;
    return true;
}

QString Database::getCurrentProject() const {
    return m_currentProject;
}

bool Database::addAIConversation(const QString& userInput, const QString& aiResponse, const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QString query = "INSERT INTO ai_conversations (project, user_input, ai_response) VALUES (?, ?, ?)";
    return executeQuery(query, {project, userInput, aiResponse});
}

QList<QJsonObject> Database::getAIConversations(const QString& project, int limit) {
    QList<QJsonObject> result;

    if (isNoDatabaseMode()) {
        // 无数据库模式下返回空列表
        return result;
    }

    if (!isConnected()) {
        return result;
    }

    QString query = "SELECT * FROM ai_conversations";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    query += " ORDER BY timestamp DESC LIMIT ?";
    params << limit;

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            QJsonObject obj;
            obj["id"] = sqlQuery.value("id").toInt();
            obj["project"] = sqlQuery.value("project").toString();
            obj["user_input"] = sqlQuery.value("user_input").toString();
            obj["ai_response"] = sqlQuery.value("ai_response").toString();
            obj["timestamp"] = sqlQuery.value("timestamp").toString();
            result.append(obj);
        }
    }

    return result;
}

bool Database::saveReport(const QString& reportName, const QJsonObject& reportData, const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下不执行实际操作
        return true;
    }

    if (!isConnected()) {
        return false;
    }

    QJsonDocument doc(reportData);
    QString dataJson = doc.toJson(QJsonDocument::Compact);

    QString query = "INSERT INTO reports (project, name, data) VALUES (?, ?, ?) "
                   "ON DUPLICATE KEY UPDATE data = VALUES(data), updated_at = CURRENT_TIMESTAMP";
    return executeQuery(query, {project, reportName, dataJson});
}

QJsonObject Database::getReport(const QString& reportName, const QString& project) {
    QJsonObject result;

    if (isNoDatabaseMode()) {
        // 无数据库模式下返回空对象
        return result;
    }

    if (!isConnected()) {
        return result;
    }

    QString query = "SELECT data FROM reports WHERE project = ? AND name = ?";
    QSqlQuery sqlQuery = prepareQuery(query, {project, reportName});

    if (sqlQuery.exec() && sqlQuery.next()) {
        QJsonDocument doc = QJsonDocument::fromJson(sqlQuery.value("data").toString().toUtf8());
        result = doc.object();
    }

    return result;
}

QList<QString> Database::getReportList(const QString& project) {
    QList<QString> result;

    if (isNoDatabaseMode()) {
        // 无数据库模式下返回空列表
        return result;
    }

    if (!isConnected()) {
        return result;
    }

    QString query = "SELECT name FROM reports";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    query += " ORDER BY updated_at DESC";

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec()) {
        while (sqlQuery.next()) {
            result.append(sqlQuery.value("name").toString());
        }
    }

    return result;
}

int Database::getCommandCount(const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下返回0
        return 0;
    }

    if (!isConnected()) {
        return 0;
    }

    QString query = "SELECT COUNT(*) FROM command_history";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec() && sqlQuery.next()) {
        return sqlQuery.value(0).toInt();
    }

    return 0;
}

int Database::getScanCount(const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下返回0
        return 0;
    }

    if (!isConnected()) {
        return 0;
    }

    QString query = "SELECT COUNT(*) FROM scan_results";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec() && sqlQuery.next()) {
        return sqlQuery.value(0).toInt();
    }

    return 0;
}

QDateTime Database::getLastActivity(const QString& project) {
    if (isNoDatabaseMode()) {
        // 无数据库模式下返回当前时间
        return QDateTime::currentDateTime();
    }

    if (!isConnected()) {
        return QDateTime();
    }

    QString query = "SELECT MAX(timestamp) FROM command_history";
    QVariantList params;

    if (!project.isEmpty()) {
        query += " WHERE project = ?";
        params << project;
    }

    QSqlQuery sqlQuery = prepareQuery(query, params);
    if (sqlQuery.exec() && sqlQuery.next()) {
        return sqlQuery.value(0).toDateTime();
    }

    return QDateTime();
}

QString Database::promptForPassword(const QString& prompt) {
    std::cout << prompt.toStdString();
    std::string password;

#ifdef _WIN32
    // Windows下隐藏密码输入
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));

    std::getline(std::cin, password);

    SetConsoleMode(hStdin, mode);
    std::cout << std::endl;
#else
    // Linux下隐藏密码输入
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::getline(std::cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;
#endif

    return QString::fromStdString(password);
}

bool Database::tryConnectWithPassword() {
    // 首先尝试无密码连接
    std::cout << "尝试连接MySQL (无密码)..." << std::endl;
    if (connectToMySQL()) {
        return true;
    }

    // 无密码连接失败，提示用户输入密码
    std::cout << "\n无密码连接失败，请输入MySQL root密码" << std::endl;

    int attempts = 0;
    const int maxAttempts = 3;

    while (attempts < maxAttempts) {
        QString password = promptForPassword("MySQL root密码: ");

        std::cout << "尝试连接MySQL (使用密码)..." << std::endl;
        if (connectToMySQL("localhost", 3306, "root", password)) {
            // 保存密码供后续使用
            m_config.rootPassword = password;
            return true;
        }

        attempts++;
        if (attempts < maxAttempts) {
            std::cout << "❌ 密码错误，请重试 (" << (maxAttempts - attempts) << " 次机会剩余)" << std::endl;
        }
    }

    std::cout << "\n❌ 错误: 无法连接到MySQL" << std::endl;
    std::cout << "可能的原因:" << std::endl;
    std::cout << "1. MySQL root密码错误" << std::endl;
    std::cout << "2. MySQL服务未正确启动" << std::endl;
    std::cout << "3. 网络连接问题" << std::endl;
    std::cout << "4. MySQL配置问题" << std::endl;

    return false;
}

} // namespace MindSploit::Core
