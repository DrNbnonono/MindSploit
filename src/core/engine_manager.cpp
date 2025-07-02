#include "engine_manager.h"
#include "../engines/network/network_engine.h"
#include <iostream>

namespace MindSploit::Core {

EngineManager::EngineManager() = default;

EngineManager::~EngineManager() {
    shutdown();
}

bool EngineManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // 注册内置引擎
    registerBuiltinEngines();
    
    // 构建命令路由
    buildCommandRouting();
    
    m_initialized = true;
    return true;
}

void EngineManager::shutdown() {
    // 卸载所有引擎
    for (auto& [name, registration] : m_engines) {
        if (registration->isLoaded && registration->instance) {
            registration->instance->shutdown();
        }
    }
    
    m_engines.clear();
    m_commandToEngine.clear();
    m_initialized = false;
}

bool EngineManager::registerEngine(const std::string& name, std::unique_ptr<EngineFactory> factory) {
    if (!factory) {
        return false;
    }
    
    if (m_engines.find(name) != m_engines.end()) {
        return false; // 引擎已存在
    }
    
    auto registration = std::make_unique<EngineRegistration>();
    registration->name = name;
    registration->factory = std::move(factory);
    registration->description = registration->factory->getEngineName();
    
    m_engines[name] = std::move(registration);
    
    return true;
}

bool EngineManager::loadEngine(const std::string& name) {
    auto it = m_engines.find(name);
    if (it == m_engines.end()) {
        return false;
    }
    
    auto& registration = it->second;
    if (registration->isLoaded) {
        return true; // 已经加载
    }
    
    // 创建引擎实例
    registration->instance = registration->factory->createEngine();
    if (!registration->instance) {
        return false;
    }
    
    // 初始化引擎
    if (!registration->instance->initialize()) {
        registration->instance.reset();
        return false;
    }
    
    // 获取支持的命令
    registration->supportedCommands = registration->instance->getSupportedCommands();
    registration->version = registration->instance->getVersion();
    registration->isLoaded = true;
    
    // 更新命令路由
    updateCommandRouting(name);
    
    // 通知引擎加载
    notifyEngineLoaded(name);
    
    return true;
}

bool EngineManager::unloadEngine(const std::string& name) {
    auto it = m_engines.find(name);
    if (it == m_engines.end()) {
        return false;
    }
    
    auto& registration = it->second;
    if (!registration->isLoaded) {
        return true; // 已经卸载
    }
    
    // 关闭引擎
    if (registration->instance) {
        registration->instance->shutdown();
        registration->instance.reset();
    }
    
    registration->isLoaded = false;
    registration->supportedCommands.clear();
    
    // 更新命令路由
    buildCommandRouting();
    
    // 通知引擎卸载
    notifyEngineUnloaded(name);
    
    return true;
}

ExecutionResult EngineManager::executeCommand(const std::string& command, const CommandContext& context) {
    ExecutionResult result;
    
    // 查找处理该命令的引擎
    auto it = m_commandToEngine.find(command);
    if (it == m_commandToEngine.end()) {
        result.success = false;
        result.message = "未知命令: " + command;
        return result;
    }
    
    const std::string& engineName = it->second;
    
    // 确保引擎已加载
    if (!loadEngine(engineName)) {
        result.success = false;
        result.message = "引擎加载失败: " + engineName;
        return result;
    }
    
    // 获取引擎实例
    auto engine = getEngine(engineName);
    if (!engine) {
        result.success = false;
        result.message = "引擎不可用: " + engineName;
        return result;
    }
    
    // 执行命令
    try {
        result = engine->execute(context);
    } catch (const std::exception& e) {
        result.success = false;
        result.message = "执行异常: " + std::string(e.what());
        notifyEngineError(engineName, result.message);
    }
    
    return result;
}

bool EngineManager::isCommandSupported(const std::string& command) const {
    return m_commandToEngine.find(command) != m_commandToEngine.end();
}

std::string EngineManager::getEngineForCommand(const std::string& command) const {
    auto it = m_commandToEngine.find(command);
    return (it != m_commandToEngine.end()) ? it->second : "";
}

std::vector<std::string> EngineManager::getAvailableEngines() const {
    std::vector<std::string> engines;
    for (const auto& [name, registration] : m_engines) {
        engines.push_back(name);
    }
    return engines;
}

std::vector<std::string> EngineManager::getLoadedEngines() const {
    std::vector<std::string> engines;
    for (const auto& [name, registration] : m_engines) {
        if (registration->isLoaded) {
            engines.push_back(name);
        }
    }
    return engines;
}

std::vector<std::string> EngineManager::getSupportedCommands() const {
    std::vector<std::string> commands;
    for (const auto& [command, engine] : m_commandToEngine) {
        commands.push_back(command);
    }
    return commands;
}

EngineInterface* EngineManager::getEngine(const std::string& name) {
    auto it = m_engines.find(name);
    if (it != m_engines.end() && it->second->isLoaded) {
        return it->second->instance.get();
    }
    return nullptr;
}

std::string EngineManager::getCommandHelp(const std::string& command) const {
    auto engineName = getEngineForCommand(command);
    if (engineName.empty()) {
        return "";
    }
    
    auto it = m_engines.find(engineName);
    if (it != m_engines.end() && it->second->isLoaded && it->second->instance) {
        return it->second->instance->getCommandHelp(command);
    }
    
    return "";
}

void EngineManager::registerBuiltinEngines() {
    // 注册网络引擎
    auto networkFactory = std::make_unique<EngineFactoryTemplate<Network::NetworkEngine>>();
    registerEngine("network", std::move(networkFactory));
}

void EngineManager::buildCommandRouting() {
    m_commandToEngine.clear();
    
    for (const auto& [name, registration] : m_engines) {
        if (registration->isLoaded) {
            for (const auto& command : registration->supportedCommands) {
                m_commandToEngine[command] = name;
            }
        }
    }
}

void EngineManager::updateCommandRouting(const std::string& engineName) {
    auto it = m_engines.find(engineName);
    if (it != m_engines.end() && it->second->isLoaded) {
        for (const auto& command : it->second->supportedCommands) {
            m_commandToEngine[command] = engineName;
        }
    }
}

void EngineManager::notifyEngineLoaded(const std::string& name) {
    if (m_onEngineLoaded) {
        m_onEngineLoaded(name, "Engine loaded successfully");
    }
}

void EngineManager::notifyEngineUnloaded(const std::string& name) {
    if (m_onEngineUnloaded) {
        m_onEngineUnloaded(name, "Engine unloaded");
    }
}

void EngineManager::notifyEngineError(const std::string& name, const std::string& error) {
    if (m_onEngineError) {
        m_onEngineError(name, error);
    }
}

} // namespace MindSploit::Core
