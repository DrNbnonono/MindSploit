#pragma once

#include "../engine_interface.h"
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

namespace MindSploit::Network {

// 主机信息结构
struct HostInfo {
    std::string ip;
    std::string hostname;
    bool isAlive = false;
    std::vector<int> openPorts;
    std::map<int, std::string> services;
    std::string osFingerprint;
    double responseTime = 0.0;
};

// 端口扫描结果
struct PortScanResult {
    int port;
    bool isOpen = false;
    std::string service;
    std::string version;
    std::string banner;
    double responseTime = 0.0;
};

// 扫描配置
struct ScanConfig {
    std::vector<std::string> targets;
    std::vector<int> ports;
    int timeout = 3000;        // 超时时间(毫秒)
    int maxThreads = 100;      // 最大线程数
    bool enableServiceDetection = true;
    bool enableOSDetection = false;
    bool stealthMode = false;
    std::string scanType = "tcp"; // tcp, udp, syn
};

// 网络扫描引擎
class NetworkEngine : public EngineInterface {
public:
    static constexpr const char* ENGINE_NAME = "NetworkEngine";

    NetworkEngine();
    virtual ~NetworkEngine();

    // 基础接口实现
    bool initialize() override;
    bool shutdown() override;
    ExecutionResult execute(const CommandContext& context) override;
    void stop() override;
    
    EngineStatus getStatus() const override;
    std::string getName() const override { return "NetworkEngine"; }
    std::string getVersion() const override { return "2.0.0"; }
    std::string getDescription() const override;
    
    std::vector<std::string> getSupportedCommands() const override;
    std::map<std::string, std::string> getRequiredParameters(const std::string& command) const override;
    std::map<std::string, std::string> getOptionalParameters(const std::string& command) const override;
    
    bool setOption(const std::string& key, const std::string& value) override;
    std::string getOption(const std::string& key) const override;
    std::map<std::string, std::string> getAllOptions() const override;
    
    bool checkDependencies() const override;
    std::vector<std::string> getMissingDependencies() const override;
    
    std::string getHelp() const override;
    std::string getCommandHelp(const std::string& command) const override;

    // 网络扫描专用接口
    std::vector<HostInfo> discoverHosts(const std::vector<std::string>& targets);
    std::vector<PortScanResult> scanPorts(const std::string& target, const std::vector<int>& ports);
    std::string detectService(const std::string& target, int port);
    std::string detectOS(const std::string& target);

private:
    // 内部实现方法
    ExecutionResult executeDiscover(const CommandContext& context);
    ExecutionResult executeScan(const CommandContext& context);
    ExecutionResult executeService(const CommandContext& context);
    ExecutionResult executeOS(const CommandContext& context);
    
    // 核心扫描功能
    bool pingHost(const std::string& target);
    bool tcpConnect(const std::string& target, int port, int timeout);
    bool tcpSyn(const std::string& target, int port, int timeout);
    bool udpScan(const std::string& target, int port, int timeout);
    
    // 服务识别
    std::string grabBanner(const std::string& target, int port);
    std::string identifyService(const std::string& banner, int port);
    
    // 操作系统识别
    std::string performOSFingerprinting(const std::string& target);
    
    // 工具方法
    std::vector<std::string> parseTargets(const std::string& targetString);
    std::vector<int> parsePorts(const std::string& portString);
    bool isValidIP(const std::string& ip);
    std::string resolveHostname(const std::string& hostname);
    
    // 线程管理
    void workerThread(const std::string& target, const std::vector<int>& ports, 
                     std::vector<PortScanResult>& results, size_t startIndex, size_t endIndex);

private:
    std::atomic<EngineStatus> m_status{EngineStatus::IDLE};
    std::atomic<bool> m_stopRequested{false};
    ScanConfig m_config;
    std::map<std::string, std::string> m_options;
    std::vector<std::thread> m_workers;
    
    // 默认端口列表
    static const std::vector<int> DEFAULT_PORTS;
    static const std::map<int, std::string> COMMON_SERVICES;
};

// 网络工具类
class NetworkUtils {
public:
    static bool isPortOpen(const std::string& host, int port, int timeout = 3000);
    static std::string getLocalIP();
    static std::vector<std::string> getNetworkInterfaces();
    static bool isValidIPRange(const std::string& range);
    static std::vector<std::string> expandIPRange(const std::string& range);
    static uint16_t calculateChecksum(const void* data, size_t length);
    static std::string formatDuration(std::chrono::milliseconds duration);
};

} // namespace MindSploit::Network
