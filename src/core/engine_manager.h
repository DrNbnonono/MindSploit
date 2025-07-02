#pragma once

#include "../engines/engine_interface.h"
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace MindSploit::Core {

// 引擎注册信息
struct EngineRegistration {
    std::string name;
    std::string description;
    std::string version;
    std::unique_ptr<EngineFactory> factory;
    std::vector<std::string> supportedCommands;
    bool isLoaded = false;
    std::unique_ptr<EngineInterface> instance;
};

// 引擎管理器
class EngineManager {
public:
    EngineManager();
    ~EngineManager();

    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // 引擎注册
    bool registerEngine(const std::string& name, std::unique_ptr<EngineFactory> factory);
    bool unregisterEngine(const std::string& name);
    
    // 引擎管理
    bool loadEngine(const std::string& name);
    bool unloadEngine(const std::string& name);
    bool reloadEngine(const std::string& name);
    
    // 命令执行
    ExecutionResult executeCommand(const std::string& command, const CommandContext& context);
    bool isCommandSupported(const std::string& command) const;
    std::string getEngineForCommand(const std::string& command) const;
    
    // 查询接口
    std::vector<std::string> getAvailableEngines() const;
    std::vector<std::string> getLoadedEngines() const;
    std::vector<std::string> getSupportedCommands() const;
    std::vector<std::string> getEngineCommands(const std::string& engineName) const;
    
    // 引擎信息
    EngineRegistration* getEngineInfo(const std::string& name);
    EngineInterface* getEngine(const std::string& name);
    EngineStatus getEngineStatus(const std::string& name) const;
    
    // 依赖检查
    bool checkAllDependencies() const;
    std::map<std::string, std::vector<std::string>> getMissingDependencies() const;
    
    // 配置管理
    bool setEngineOption(const std::string& engineName, const std::string& key, const std::string& value);
    std::string getEngineOption(const std::string& engineName, const std::string& key) const;
    std::map<std::string, std::string> getEngineOptions(const std::string& engineName) const;
    
    // 帮助系统
    std::string getEngineHelp(const std::string& engineName) const;
    std::string getCommandHelp(const std::string& command) const;
    std::string getAllEnginesHelp() const;
    
    // 事件回调
    using EngineEventCallback = std::function<void(const std::string&, const std::string&)>;
    void setEngineLoadCallback(EngineEventCallback callback);
    void setEngineUnloadCallback(EngineEventCallback callback);
    void setEngineErrorCallback(EngineEventCallback callback);

private:
    // 内部方法
    void registerBuiltinEngines();
    bool validateEngineFactory(const std::string& name, EngineFactory* factory);
    void notifyEngineLoaded(const std::string& name);
    void notifyEngineUnloaded(const std::string& name);
    void notifyEngineError(const std::string& name, const std::string& error);
    
    // 命令路由
    void buildCommandRouting();
    void updateCommandRouting(const std::string& engineName);

private:
    std::map<std::string, std::unique_ptr<EngineRegistration>> m_engines;
    std::map<std::string, std::string> m_commandToEngine; // 命令到引擎的映射
    
    // 事件回调
    EngineEventCallback m_onEngineLoaded;
    EngineEventCallback m_onEngineUnloaded;
    EngineEventCallback m_onEngineError;
    
    bool m_initialized = false;
};

// 引擎自动注册器
template<typename EngineClass>
class EngineAutoRegister {
public:
    EngineAutoRegister(const std::string& name) {
        // 在静态初始化时注册引擎工厂
        auto factory = std::make_unique<typename EngineClass::Factory>();
        // 这里需要一个全局的引擎管理器实例来注册
        // 或者使用延迟注册机制
    }
};

// 引擎注册宏
#define REGISTER_ENGINE_AUTO(EngineClass, EngineName) \
    namespace { \
        static EngineAutoRegister<EngineClass> g_##EngineClass##_register(EngineName); \
    }

// 引擎工厂模板
template<typename EngineClass>
class EngineFactoryTemplate : public EngineFactory {
public:
    std::unique_ptr<EngineInterface> createEngine() override {
        return std::make_unique<EngineClass>();
    }
    
    std::string getEngineName() const override {
        return EngineClass::ENGINE_NAME;
    }
};

} // namespace MindSploit::Core
