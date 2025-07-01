#include "config_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <QCryptographicHash>

// 静态成员初始化
ConfigManager* ConfigManager::s_instance = nullptr;
const QString ConfigManager::DEFAULT_THEME = "dark";
const int ConfigManager::DEFAULT_AUTOSAVE_INTERVAL = 300; // 5分钟
const QString ConfigManager::DEFAULT_AI_PROVIDER = "openai";
const QString ConfigManager::DEFAULT_AI_MODEL = "gpt-3.5-turbo";

ConfigManager& ConfigManager::instance() {
    if (!s_instance) {
        s_instance = new ConfigManager();
    }
    return *s_instance;
}

ConfigManager::ConfigManager(QObject* parent) : QObject(parent) {
    // 初始化配置文件路径
    m_dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_configFilePath = m_dataDirectory + "/config.json";
    
    // 创建数据目录
    createDataDirectory();
    
    // 初始化QSettings
    m_settings = new QSettings(m_dataDirectory + "/settings.ini", QSettings::IniFormat, this);
    
    // QSettings没有settingsChanged信号，这里注释掉
    // connect(m_settings, &QSettings::settingsChanged, this, &ConfigManager::onSettingsChanged);
}

void ConfigManager::initialize() {
    loadDefaults();
    
    // 加载JSON配置文件
    QFile file(m_configFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        m_jsonConfig = doc.object();
        file.close();
    }
    
    qDebug() << "ConfigManager initialized. Data directory:" << m_dataDirectory;
}

void ConfigManager::createDataDirectory() {
    QDir dir;
    if (!dir.exists(m_dataDirectory)) {
        dir.mkpath(m_dataDirectory);
        qDebug() << "Created data directory:" << m_dataDirectory;
    }
}

void ConfigManager::loadDefaults() {
    // 设置默认值（如果不存在）
    if (!contains("ui/theme")) {
        setValue("ui/theme", DEFAULT_THEME);
    }
    if (!contains("general/autosave_interval")) {
        setValue("general/autosave_interval", DEFAULT_AUTOSAVE_INTERVAL);
    }
    if (!contains("ai/default_provider")) {
        setValue("ai/default_provider", DEFAULT_AI_PROVIDER);
    }
    if (!contains("ai/default_model")) {
        setValue("ai/default_model", DEFAULT_AI_MODEL);
    }
    if (!contains("security/encryption_enabled")) {
        setValue("security/encryption_enabled", false);
    }
}

void ConfigManager::setValue(const QString& key, const QVariant& value) {
    m_settings->setValue(key, value);
    m_settings->sync();
    emit configChanged(key, value);
}

QVariant ConfigManager::getValue(const QString& key, const QVariant& defaultValue) const {
    return m_settings->value(key, defaultValue);
}

bool ConfigManager::contains(const QString& key) const {
    return m_settings->contains(key);
}

void ConfigManager::removeKey(const QString& key) {
    m_settings->remove(key);
    m_settings->sync();
}

// AI配置方法
void ConfigManager::setAIApiKey(const QString& provider, const QString& apiKey) {
    QString key = QString("ai/%1/api_key").arg(provider);
    if (isEncryptionEnabled()) {
        setValue(key, encryptValue(apiKey));
    } else {
        setValue(key, apiKey);
    }
}

QString ConfigManager::getAIApiKey(const QString& provider) const {
    QString key = QString("ai/%1/api_key").arg(provider);
    QString value = getValue(key).toString();
    
    if (isEncryptionEnabled() && !value.isEmpty()) {
        return decryptValue(value);
    }
    return value;
}

void ConfigManager::setAIModel(const QString& provider, const QString& model) {
    setValue(QString("ai/%1/model").arg(provider), model);
}

QString ConfigManager::getAIModel(const QString& provider) const {
    return getValue(QString("ai/%1/model").arg(provider), DEFAULT_AI_MODEL).toString();
}

void ConfigManager::setAIEndpoint(const QString& provider, const QString& endpoint) {
    setValue(QString("ai/%1/endpoint").arg(provider), endpoint);
}

QString ConfigManager::getAIEndpoint(const QString& provider) const {
    return getValue(QString("ai/%1/endpoint").arg(provider)).toString();
}

// 工具路径配置
void ConfigManager::setToolPath(const QString& toolName, const QString& path) {
    setValue(QString("tools/%1/path").arg(toolName), path);
}

QString ConfigManager::getToolPath(const QString& toolName) const {
    return getValue(QString("tools/%1/path").arg(toolName)).toString();
}

QStringList ConfigManager::getAvailableTools() const {
    QStringList tools;
    m_settings->beginGroup("tools");
    QStringList groups = m_settings->childGroups();
    for (const QString& group : groups) {
        tools.append(group);
    }
    m_settings->endGroup();
    return tools;
}

// 界面配置
void ConfigManager::setWindowGeometry(const QByteArray& geometry) {
    setValue("ui/window_geometry", geometry);
}

QByteArray ConfigManager::getWindowGeometry() const {
    return getValue("ui/window_geometry").toByteArray();
}

void ConfigManager::setWindowState(const QByteArray& state) {
    setValue("ui/window_state", state);
}

QByteArray ConfigManager::getWindowState() const {
    return getValue("ui/window_state").toByteArray();
}

void ConfigManager::setTheme(const QString& theme) {
    setValue("ui/theme", theme);
}

QString ConfigManager::getTheme() const {
    return getValue("ui/theme", DEFAULT_THEME).toString();
}

// 项目配置
void ConfigManager::setLastProject(const QString& projectName) {
    setValue("general/last_project", projectName);
}

QString ConfigManager::getLastProject() const {
    return getValue("general/last_project", "default").toString();
}

void ConfigManager::setAutoSaveInterval(int seconds) {
    setValue("general/autosave_interval", seconds);
}

int ConfigManager::getAutoSaveInterval() const {
    return getValue("general/autosave_interval", DEFAULT_AUTOSAVE_INTERVAL).toInt();
}

// 安全配置
void ConfigManager::setEncryptionEnabled(bool enabled) {
    setValue("security/encryption_enabled", enabled);
}

bool ConfigManager::isEncryptionEnabled() const {
    return getValue("security/encryption_enabled", false).toBool();
}

void ConfigManager::setPasswordHash(const QString& hash) {
    setValue("security/password_hash", hash);
}

QString ConfigManager::getPasswordHash() const {
    return getValue("security/password_hash").toString();
}

QString ConfigManager::getConfigFilePath() const {
    return m_configFilePath;
}

QString ConfigManager::getDataDirectory() const {
    return m_dataDirectory;
}

void ConfigManager::onSettingsChanged() {
    // 配置文件发生变化时的处理
    qDebug() << "Settings changed";
}

QString ConfigManager::encryptValue(const QString& value) const {
    // 简单的加密实现（实际项目中应使用更安全的加密方法）
    QByteArray data = value.toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return hash.toBase64();
}

QString ConfigManager::decryptValue(const QString& value) const {
    // 简单的解密实现（这里只是示例，实际需要可逆加密）
    Q_UNUSED(value)
    return QString(); // 实际实现需要可逆加密算法
}

void ConfigManager::resetToDefaults() {
    m_settings->clear();
    loadDefaults();
    emit configChanged("", QVariant()); // 通知所有配置已重置
}
