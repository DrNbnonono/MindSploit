#pragma once

#include "module_interface.h"
#include <QObject>
#include <QHash>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 模块管理器 - 负责所有渗透测试模块的注册、管理和调度
 * 
 * 功能包括：
 * - 模块注册和发现
 * - 模块生命周期管理
 * - 命令路由和执行
 * - 模块依赖检查
 */
class ModuleManager : public QObject {
    Q_OBJECT

public:
    static ModuleManager& instance();
    
    // 模块注册和管理
    bool registerModule(const QString& name, ModuleInterface* module);
    bool unregisterModule(const QString& name);
    ModuleInterface* getModule(const QString& name) const;
    QStringList getAvailableModules() const;
    QStringList getModulesByType(ModuleInterface::ModuleType type) const;
    
    // 模块信息查询
    QJsonObject getModuleInfo(const QString& name) const;
    QJsonArray getAllModulesInfo() const;
    bool isModuleAvailable(const QString& name) const;
    QStringList getMissingDependencies(const QString& name) const;
    
    // 命令执行
    bool executeCommand(const QString& command, const QStringList& args = {});
    bool executeModule(const QString& moduleName, const QJsonObject& options = {});
    void stopModule(const QString& moduleName);
    void stopAllModules();
    
    // 模块搜索
    QStringList searchModules(const QString& keyword) const;
    QStringList getModulesForTarget(const QString& target) const;
    
    // 依赖管理
    bool checkAllDependencies();
    QStringList getSystemTools() const;
    bool installMissingTools();
    
    // 配置管理
    void loadModuleConfigs();
    void saveModuleConfigs();
    QJsonObject getModuleConfig(const QString& name) const;
    void setModuleConfig(const QString& name, const QJsonObject& config);

signals:
    void moduleRegistered(const QString& name);
    void moduleUnregistered(const QString& name);
    void moduleExecutionStarted(const QString& name);
    void moduleExecutionCompleted(const QString& name, const QJsonObject& results);
    void moduleExecutionFailed(const QString& name, const QString& error);
    void dependencyCheckCompleted(const QStringList& missing);

public slots:
    void onModuleStatusChanged(ModuleInterface::ModuleStatus status);
    void onModuleOutputReceived(const QString& output);
    void onModuleErrorReceived(const QString& error);

private:
    ModuleManager(QObject* parent = nullptr);
    ~ModuleManager() = default;
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
    
    void initializeBuiltinModules();
    void discoverExternalModules();
    QString parseCommand(const QString& command, QStringList& args) const;
    
    QHash<QString, ModuleInterface*> m_modules;
    QHash<QString, QJsonObject> m_moduleConfigs;
    static ModuleManager* s_instance;
};
