#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief 工具管理器 - 统一管理外部工具的检查、安装和更新
 * 
 * 功能包括：
 * - 工具可用性检查
 * - 版本检测和比较
 * - 自动安装和更新
 * - 路径管理和配置
 * - 依赖关系处理
 */
class ToolManager : public QObject {
    Q_OBJECT

public:
    // 工具状态
    enum ToolStatus {
        NOT_INSTALLED,      // 未安装
        INSTALLED,          // 已安装
        OUTDATED,           // 版本过旧
        UNKNOWN,            // 状态未知
        ERROR               // 检查出错
    };

    // 安装方式
    enum InstallMethod {
        PACKAGE_MANAGER,    // 包管理器安装
        BINARY_DOWNLOAD,    // 二进制下载
        SOURCE_COMPILE,     // 源码编译
        PORTABLE_EXTRACT,   // 便携版解压
        MANUAL_INSTALL      // 手动安装
    };

    // 工具信息结构
    struct ToolInfo {
        QString name;               // 工具名称
        QString displayName;        // 显示名称
        QString description;        // 描述
        QString homepage;           // 官方网站
        QString currentVersion;     // 当前版本
        QString latestVersion;      // 最新版本
        QString executablePath;     // 可执行文件路径
        QStringList aliases;        // 别名列表
        QStringList dependencies;   // 依赖工具
        ToolStatus status;          // 当前状态
        InstallMethod installMethod; // 安装方式
        QString installCommand;     // 安装命令
        QString versionCommand;     // 版本检查命令
        QString versionPattern;     // 版本提取正则
        QJsonObject metadata;       // 额外元数据
    };

    static ToolManager& instance();
    
    // 工具注册和管理
    void registerTool(const ToolInfo& toolInfo);
    void unregisterTool(const QString& toolName);
    bool isToolRegistered(const QString& toolName) const;
    QStringList getRegisteredTools() const;
    ToolInfo getToolInfo(const QString& toolName) const;
    void updateToolInfo(const QString& toolName, const ToolInfo& toolInfo);

    // 工具检查
    ToolStatus checkToolStatus(const QString& toolName);
    bool isToolAvailable(const QString& toolName);
    QString getToolPath(const QString& toolName);
    QString getToolVersion(const QString& toolName);
    QStringList checkAllTools();
    QStringList getMissingTools();
    QStringList getOutdatedTools();

    // 工具安装
    bool installTool(const QString& toolName);
    bool updateTool(const QString& toolName);
    bool uninstallTool(const QString& toolName);
    void installMissingTools();
    void updateAllTools();

    // 路径管理
    void addToolPath(const QString& path);
    void removeToolPath(const QString& path);
    QStringList getToolPaths() const;
    void setToolPath(const QString& toolName, const QString& path);
    void autoDiscoverTools();
    void refreshToolPaths();

    // 依赖管理
    QStringList getToolDependencies(const QString& toolName) const;
    QStringList getMissingDependencies(const QString& toolName) const;
    bool checkDependencies(const QString& toolName);
    void installDependencies(const QString& toolName);

    // 配置管理
    void loadConfiguration();
    void saveConfiguration();
    void resetConfiguration();
    void importConfiguration(const QString& filePath);
    void exportConfiguration(const QString& filePath);

    // 在线更新
    void checkForUpdates();
    void downloadToolDatabase();
    void setUpdateServer(const QString& url);

signals:
    void toolStatusChanged(const QString& toolName, ToolStatus status);
    void toolInstalled(const QString& toolName);
    void toolUninstalled(const QString& toolName);
    void toolUpdated(const QString& toolName, const QString& newVersion);
    void installationProgress(const QString& toolName, int percentage);
    void installationCompleted(const QString& toolName, bool success);
    void updateCheckCompleted(const QStringList& availableUpdates);

private slots:
    void onInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onVersionCheckFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onUpdateCheckReplyFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    ToolManager(QObject* parent = nullptr);
    ~ToolManager() = default;
    ToolManager(const ToolManager&) = delete;
    ToolManager& operator=(const ToolManager&) = delete;

    void initializeBuiltinTools();
    void setupDefaultPaths();
    QString findExecutable(const QString& toolName) const;
    QString extractVersion(const QString& output, const QString& pattern) const;
    bool compareVersions(const QString& version1, const QString& version2) const;
    QString getInstallCommand(const QString& toolName) const;
    QString getPackageManagerCommand() const;
    void executeInstallCommand(const QString& command, const QString& toolName);
    void processVersionOutput(const QString& toolName, const QString& output);

    QHash<QString, ToolInfo> m_tools;
    QStringList m_toolPaths;
    QHash<QString, QProcess*> m_activeProcesses;
    QNetworkAccessManager* m_networkManager;
    QString m_updateServerUrl;
    QString m_configFilePath;

    static ToolManager* s_instance;
    static const QStringList DEFAULT_PATHS;
    static const QString CONFIG_FILE_NAME;
};

/**
 * @brief 工具安装助手 - 提供图形化的工具安装界面
 */
class ToolInstallWizard : public QObject {
    Q_OBJECT

public:
    explicit ToolInstallWizard(QObject* parent = nullptr);
    
    // 安装向导
    void showInstallWizard();
    void showToolManager();
    void checkMissingTools();
    void installRecommendedTools();

    // 批量操作
    void installToolList(const QStringList& tools);
    void updateToolList(const QStringList& tools);
    void verifyInstallation(const QStringList& tools);

signals:
    void wizardCompleted();
    void installationFailed(const QString& toolName, const QString& error);

private slots:
    void onToolInstallationCompleted(const QString& toolName, bool success);

private:
    void createInstallDialog();
    void updateProgress();
    void showResults();

    QStringList m_pendingInstalls;
    QStringList m_successfulInstalls;
    QStringList m_failedInstalls;
    int m_currentInstallIndex;
};

// 预定义工具信息
namespace BuiltinTools {
    ToolManager::ToolInfo createNmapInfo();
    ToolManager::ToolInfo createSearchsploitInfo();
    ToolManager::ToolInfo createCurlInfo();
    ToolManager::ToolInfo createMasscanInfo();
    ToolManager::ToolInfo createZmapInfo();
    ToolManager::ToolInfo createSQLMapInfo();
    ToolManager::ToolInfo createDirbInfo();
    ToolManager::ToolInfo createGobusterInfo();
    ToolManager::ToolInfo createNiktoInfo();
    ToolManager::ToolInfo createMetasploitInfo();
    ToolManager::ToolInfo createWiresharkInfo();
    ToolManager::ToolInfo createBurpSuiteInfo();
    ToolManager::ToolInfo createOWASPZAPInfo();
    
    // Windows特定工具
    ToolManager::ToolInfo createPsExecInfo();
    ToolManager::ToolInfo createSysinternalsInfo();
    ToolManager::ToolInfo createPowerShellEmpireInfo();
    
    QList<ToolManager::ToolInfo> getAllBuiltinTools();
}
