#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>

namespace MindSploit {

// 引擎状态枚举
enum class EngineStatus {
    IDLE,           // 空闲状态
    INITIALIZING,   // 初始化中
    RUNNING,        // 运行中
    PAUSED,         // 暂停
    STOPPING,       // 停止中
    ENGINE_ERROR,   // 错误状态
    COMPLETED       // 完成
};

// 会话信息
struct SessionInfo {
    std::string sessionId;
    std::string userId;
    std::map<std::string, std::string> globalOptions;
    std::string workingDirectory;
};

// AI上下文
struct AIContext {
    bool enabled = false;
    std::string provider;
    std::string model;
    std::map<std::string, std::string> context;
};

// 命令执行上下文
struct CommandContext {
    std::string command;                           // 原始命令
    std::map<std::string, std::string> parameters; // 解析后的参数
    std::string target;                           // 目标地址/URL
    SessionInfo session;                          // 会话信息
    AIContext aiContext;                          // AI上下文
    std::function<void(const std::string&)> outputCallback; // 输出回调
    std::function<void(const std::string&)> errorCallback;  // 错误回调
};

// 执行结果
struct ExecutionResult {
    bool success = false;
    std::string message;
    std::map<std::string, std::string> data;
    double executionTime = 0.0; // 执行时间(秒)
    int exitCode = 0;
};

// 引擎接口基类
class EngineInterface {
public:
    virtual ~EngineInterface() = default;

    // 基础生命周期管理
    virtual bool initialize() = 0;
    virtual bool shutdown() = 0;
    
    // 命令执行
    virtual ExecutionResult execute(const CommandContext& context) = 0;
    virtual void stop() = 0;
    
    // 状态查询
    virtual EngineStatus getStatus() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    
    // 能力查询
    virtual std::vector<std::string> getSupportedCommands() const = 0;
    virtual std::map<std::string, std::string> getRequiredParameters(const std::string& command) const = 0;
    virtual std::map<std::string, std::string> getOptionalParameters(const std::string& command) const = 0;
    
    // 配置管理
    virtual bool setOption(const std::string& key, const std::string& value) = 0;
    virtual std::string getOption(const std::string& key) const = 0;
    virtual std::map<std::string, std::string> getAllOptions() const = 0;
    
    // 依赖检查
    virtual bool checkDependencies() const = 0;
    virtual std::vector<std::string> getMissingDependencies() const = 0;
    
    // 帮助信息
    virtual std::string getHelp() const = 0;
    virtual std::string getCommandHelp(const std::string& command) const = 0;

protected:
    // 工具方法
    void notifyOutput(const CommandContext& context, const std::string& message) {
        if (context.outputCallback) {
            context.outputCallback(message);
        }
    }
    
    void notifyError(const CommandContext& context, const std::string& error) {
        if (context.errorCallback) {
            context.errorCallback(error);
        }
    }
};

// 引擎工厂接口
class EngineFactory {
public:
    virtual ~EngineFactory() = default;
    virtual std::unique_ptr<EngineInterface> createEngine() = 0;
    virtual std::string getEngineName() const = 0;
};

// 引擎注册宏
#define REGISTER_ENGINE(ClassName, EngineName) \
    class ClassName##Factory : public EngineFactory { \
    public: \
        std::unique_ptr<EngineInterface> createEngine() override { \
            return std::make_unique<ClassName>(); \
        } \
        std::string getEngineName() const override { \
            return EngineName; \
        } \
    };

} // namespace MindSploit
