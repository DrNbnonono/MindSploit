#pragma once

#include <QObject>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>
#include <QVariant>

/**
 * @brief 配置管理类 - 使用QSettings和JSON文件管理配置
 * 
 * 负责管理用户偏好设置、API密钥、工具路径等配置信息
 */
class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager& instance();
    
    // 初始化配置
    void initialize();
    
    // 通用配置操作
    void setValue(const QString& key, const QVariant& value);
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    bool contains(const QString& key) const;
    void removeKey(const QString& key);
    
    // AI配置
    void setAIApiKey(const QString& provider, const QString& apiKey);
    QString getAIApiKey(const QString& provider) const;
    void setAIModel(const QString& provider, const QString& model);
    QString getAIModel(const QString& provider) const;
    void setAIEndpoint(const QString& provider, const QString& endpoint);
    QString getAIEndpoint(const QString& provider) const;
    
    // 工具路径配置
    void setToolPath(const QString& toolName, const QString& path);
    QString getToolPath(const QString& toolName) const;
    QStringList getAvailableTools() const;
    
    // 界面配置
    void setWindowGeometry(const QByteArray& geometry);
    QByteArray getWindowGeometry() const;
    void setWindowState(const QByteArray& state);
    QByteArray getWindowState() const;
    void setTheme(const QString& theme);
    QString getTheme() const;
    
    // 项目配置
    void setLastProject(const QString& projectName);
    QString getLastProject() const;
    void setAutoSaveInterval(int seconds);
    int getAutoSaveInterval() const;
    
    // 安全配置
    void setEncryptionEnabled(bool enabled);
    bool isEncryptionEnabled() const;
    void setPasswordHash(const QString& hash);
    QString getPasswordHash() const;
    
    // 导入导出配置
    bool exportConfig(const QString& filePath) const;
    bool importConfig(const QString& filePath);
    void resetToDefaults();
    
    // 配置文件路径
    QString getConfigFilePath() const;
    QString getDataDirectory() const;

signals:
    void configChanged(const QString& key, const QVariant& value);

private slots:
    void onSettingsChanged();

private:
    ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    void loadDefaults();
    void createDataDirectory();
    QString encryptValue(const QString& value) const;
    QString decryptValue(const QString& value) const;
    
    QSettings* m_settings;
    QJsonObject m_jsonConfig;
    QString m_configFilePath;
    QString m_dataDirectory;
    
    static ConfigManager* s_instance;
    
    // 默认配置常量
    static const QString DEFAULT_THEME;
    static const int DEFAULT_AUTOSAVE_INTERVAL;
    static const QString DEFAULT_AI_PROVIDER;
    static const QString DEFAULT_AI_MODEL;
};
