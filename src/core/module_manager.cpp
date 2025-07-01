#include "module_manager.h"
#include "../modules/nmap/nmap_module.h"
#include "../modules/searchsploit/searchsploit_module.h"
#include "../modules/curl/curl_module.h"
#include "../modules/portscan/portscan_module.h"
#include "config_manager.h"
#include <QDebug>
#include <QDir>
#include <QJsonDocument>

ModuleManager* ModuleManager::s_instance = nullptr;

ModuleManager& ModuleManager::instance() {
    if (!s_instance) {
        s_instance = new ModuleManager();
    }
    return *s_instance;
}

ModuleManager::ModuleManager(QObject* parent) : QObject(parent) {
    initializeBuiltinModules();
    discoverExternalModules();
    loadModuleConfigs();
}

void ModuleManager::initializeBuiltinModules() {
    qDebug() << "Initializing builtin modules...";
    
    // 注册内置模块
    registerModule("nmap", new NmapModule(this));
    registerModule("searchsploit", new SearchsploitModule(this));
    registerModule("curl", new CurlModule(this));
    registerModule("portscan", new PortScanModule(this));
    
    qDebug() << "Builtin modules initialized:" << m_modules.keys();
}

void ModuleManager::discoverExternalModules() {
    // 未来可以在这里扫描外部模块目录
    // 加载第三方模块或插件
}

bool ModuleManager::registerModule(const QString& name, ModuleInterface* module) {
    if (!module) {
        qWarning() << "Cannot register null module:" << name;
        return false;
    }
    
    if (m_modules.contains(name)) {
        qWarning() << "Module already registered:" << name;
        return false;
    }
    
    // 连接模块信号
    connect(module, &ModuleInterface::statusChanged, 
            this, &ModuleManager::onModuleStatusChanged);
    connect(module, &ModuleInterface::outputReceived,
            this, &ModuleManager::onModuleOutputReceived);
    connect(module, &ModuleInterface::errorReceived,
            this, &ModuleManager::onModuleErrorReceived);
    connect(module, &ModuleInterface::executionCompleted,
            [this, name](const QJsonObject& results) {
                emit moduleExecutionCompleted(name, results);
            });
    connect(module, &ModuleInterface::executionFailed,
            [this, name](const QString& error) {
                emit moduleExecutionFailed(name, error);
            });
    
    m_modules[name] = module;
    emit moduleRegistered(name);
    
    qDebug() << "Module registered:" << name;
    return true;
}

bool ModuleManager::unregisterModule(const QString& name) {
    if (!m_modules.contains(name)) {
        return false;
    }
    
    ModuleInterface* module = m_modules.take(name);
    if (module) {
        module->stop();
        module->deleteLater();
    }
    
    emit moduleUnregistered(name);
    qDebug() << "Module unregistered:" << name;
    return true;
}

ModuleInterface* ModuleManager::getModule(const QString& name) const {
    return m_modules.value(name, nullptr);
}

QStringList ModuleManager::getAvailableModules() const {
    return m_modules.keys();
}

QStringList ModuleManager::getModulesByType(ModuleInterface::ModuleType type) const {
    QStringList result;
    for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
        if (it.value()->getType() == type) {
            result.append(it.key());
        }
    }
    return result;
}

QJsonObject ModuleManager::getModuleInfo(const QString& name) const {
    ModuleInterface* module = getModule(name);
    if (!module) {
        return QJsonObject();
    }
    
    QJsonObject info;
    info["name"] = module->getName();
    info["version"] = module->getVersion();
    info["description"] = module->getDescription();
    info["author"] = module->getAuthor();
    info["type"] = static_cast<int>(module->getType());
    info["dependencies"] = QJsonArray::fromStringList(module->getDependencies());
    info["available"] = module->isAvailable();
    info["status"] = static_cast<int>(module->getStatus());
    info["supported_targets"] = QJsonArray::fromStringList(module->getSupportedTargets());
    info["required_options"] = QJsonArray::fromStringList(module->getRequiredOptions());
    info["default_options"] = module->getDefaultOptions();
    
    return info;
}

QJsonArray ModuleManager::getAllModulesInfo() const {
    QJsonArray result;
    for (const QString& name : m_modules.keys()) {
        result.append(getModuleInfo(name));
    }
    return result;
}

bool ModuleManager::isModuleAvailable(const QString& name) const {
    ModuleInterface* module = getModule(name);
    return module && module->isAvailable();
}

QStringList ModuleManager::getMissingDependencies(const QString& name) const {
    ModuleInterface* module = getModule(name);
    if (!module) {
        return QStringList();
    }
    
    QStringList missing;
    for (const QString& dep : module->getDependencies()) {
        if (!module->checkDependency(dep)) {
            missing.append(dep);
        }
    }
    return missing;
}

bool ModuleManager::executeCommand(const QString& command, const QStringList& args) {
    QStringList cmdArgs = args;
    QString moduleName = parseCommand(command, cmdArgs);
    
    if (moduleName.isEmpty()) {
        qWarning() << "Invalid command:" << command;
        return false;
    }
    
    ModuleInterface* module = getModule(moduleName);
    if (!module) {
        qWarning() << "Module not found:" << moduleName;
        return false;
    }
    
    if (!module->isAvailable()) {
        qWarning() << "Module not available:" << moduleName;
        return false;
    }
    
    // 解析命令行参数并设置模块选项
    QJsonObject options;
    for (int i = 0; i < cmdArgs.size(); i += 2) {
        if (i + 1 < cmdArgs.size()) {
            QString key = cmdArgs[i];
            if (key.startsWith("-")) {
                key = key.mid(1); // 移除前缀 -
            }
            options[key] = cmdArgs[i + 1];
        }
    }
    
    return executeModule(moduleName, options);
}

bool ModuleManager::executeModule(const QString& moduleName, const QJsonObject& options) {
    ModuleInterface* module = getModule(moduleName);
    if (!module) {
        qWarning() << "Module not found:" << moduleName;
        return false;
    }
    
    if (!module->canExecute()) {
        qWarning() << "Module cannot execute:" << moduleName;
        return false;
    }
    
    // 设置模块选项
    for (auto it = options.begin(); it != options.end(); ++it) {
        module->setOption(it.key(), it.value().toVariant());
    }
    
    // 验证选项
    if (!module->validateOptions()) {
        qWarning() << "Module options validation failed:" << moduleName;
        return false;
    }
    
    emit moduleExecutionStarted(moduleName);
    module->execute();
    
    qDebug() << "Module execution started:" << moduleName;
    return true;
}

void ModuleManager::stopModule(const QString& moduleName) {
    ModuleInterface* module = getModule(moduleName);
    if (module) {
        module->stop();
        qDebug() << "Module stopped:" << moduleName;
    }
}

void ModuleManager::stopAllModules() {
    for (ModuleInterface* module : m_modules.values()) {
        if (module) {
            module->stop();
        }
    }
    qDebug() << "All modules stopped";
}

QStringList ModuleManager::searchModules(const QString& keyword) const {
    QStringList result;
    QString lowerKeyword = keyword.toLower();
    
    for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
        ModuleInterface* module = it.value();
        if (module->getName().toLower().contains(lowerKeyword) ||
            module->getDescription().toLower().contains(lowerKeyword)) {
            result.append(it.key());
        }
    }
    
    return result;
}

QString ModuleManager::parseCommand(const QString& command, QStringList& args) const {
    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return QString();
    }
    
    QString moduleName = parts.first();
    args = parts.mid(1);
    
    return moduleName;
}

void ModuleManager::onModuleStatusChanged(ModuleInterface::ModuleStatus status) {
    Q_UNUSED(status)
    // 处理模块状态变化
}

void ModuleManager::onModuleOutputReceived(const QString& output) {
    Q_UNUSED(output)
    // 处理模块输出
}

void ModuleManager::onModuleErrorReceived(const QString& error) {
    Q_UNUSED(error)
    // 处理模块错误
}

void ModuleManager::loadModuleConfigs() {
    // 从配置文件加载模块配置
    // 实现配置持久化
}

void ModuleManager::saveModuleConfigs() {
    // 保存模块配置到文件
    // 实现配置持久化
}
