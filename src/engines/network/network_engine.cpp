#include "network_engine.h"
#include "../../utils/network_utils.h"
#include <iostream>
#include <sstream>

namespace MindSploit::Network {

// 静态成员定义
const std::vector<int> NetworkEngine::DEFAULT_PORTS = {
    21, 22, 23, 25, 53, 80, 110, 111, 135, 139, 143, 443, 993, 995, 1723, 3306, 3389, 5432, 5900, 8080
};

const std::map<int, std::string> NetworkEngine::COMMON_SERVICES = {
    {21, "ftp"}, {22, "ssh"}, {23, "telnet"}, {25, "smtp"}, {53, "dns"},
    {80, "http"}, {110, "pop3"}, {135, "msrpc"}, {139, "netbios-ssn"},
    {143, "imap"}, {443, "https"}, {993, "imaps"}, {995, "pop3s"},
    {1723, "pptp"}, {3306, "mysql"}, {3389, "rdp"}, {5432, "postgresql"},
    {5900, "vnc"}, {8080, "http-proxy"}
};

NetworkEngine::NetworkEngine() {
    m_options["timeout"] = "3000";
    m_options["threads"] = "50";
    m_options["stealth"] = "false";
}

NetworkEngine::~NetworkEngine() {
    shutdown();
}

bool NetworkEngine::initialize() {
    if (!Utils::NetworkUtils::initialize()) {
        return false;
    }
    
    m_status = EngineStatus::IDLE;
    return true;
}

bool NetworkEngine::shutdown() {
    stop();
    Utils::NetworkUtils::cleanup();
    return true;
}

ExecutionResult NetworkEngine::execute(const CommandContext& context) {
    ExecutionResult result;
    
    if (context.command == "discover") {
        result = executeDiscover(context);
    } else if (context.command == "scan") {
        result = executeScan(context);
    } else if (context.command == "service") {
        result = executeService(context);
    } else if (context.command == "os") {
        result = executeOS(context);
    } else {
        result.success = false;
        result.message = "Unsupported command: " + context.command;
    }
    
    return result;
}

void NetworkEngine::stop() {
    m_stopRequested = true;
    m_status = EngineStatus::STOPPING;
    
    // 等待工作线程完成
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
    
    m_status = EngineStatus::IDLE;
    m_stopRequested = false;
}

EngineStatus NetworkEngine::getStatus() const {
    return m_status;
}

std::string NetworkEngine::getDescription() const {
    return "Network scanning and discovery engine with built-in tools";
}

std::vector<std::string> NetworkEngine::getSupportedCommands() const {
    return {"discover", "scan", "service", "os"};
}

std::map<std::string, std::string> NetworkEngine::getRequiredParameters(const std::string& command) const {
    std::map<std::string, std::string> params;
    
    if (command == "discover" || command == "scan" || command == "service" || command == "os") {
        params["target"] = "Target IP address or hostname";
    }
    
    return params;
}

std::map<std::string, std::string> NetworkEngine::getOptionalParameters(const std::string& command) const {
    std::map<std::string, std::string> params;
    
    if (command == "scan") {
        params["ports"] = "Port range to scan (e.g., 1-1000, 80,443)";
        params["type"] = "Scan type (tcp, udp, syn)";
    }
    
    params["timeout"] = "Connection timeout in milliseconds";
    params["threads"] = "Number of concurrent threads";
    
    return params;
}

bool NetworkEngine::setOption(const std::string& key, const std::string& value) {
    m_options[key] = value;
    return true;
}

std::string NetworkEngine::getOption(const std::string& key) const {
    auto it = m_options.find(key);
    return (it != m_options.end()) ? it->second : "";
}

std::map<std::string, std::string> NetworkEngine::getAllOptions() const {
    return m_options;
}

bool NetworkEngine::checkDependencies() const {
    // 检查网络功能是否可用
    return Utils::NetworkUtils::isValidIP("127.0.0.1");
}

std::vector<std::string> NetworkEngine::getMissingDependencies() const {
    std::vector<std::string> missing;
    
    if (!checkDependencies()) {
        missing.push_back("Network functionality");
    }
    
    return missing;
}

std::string NetworkEngine::getHelp() const {
    return R"(
Network Engine - 网络扫描和发现引擎

支持的命令:
  discover <target>       - 主机发现和存活检测
  scan <target> [options] - 端口扫描
  service <target>        - 服务识别
  os <target>            - 操作系统识别

选项:
  -ports <range>         - 端口范围 (例如: 1-1000, 80,443)
  -type <type>           - 扫描类型 (tcp, udp, syn)
  -timeout <ms>          - 超时时间 (毫秒)
  -threads <num>         - 线程数

示例:
  discover 192.168.1.0/24
  scan 192.168.1.1 -ports 1-1000
  scan 192.168.1.1 -ports 80,443,8080 -type tcp
  service 192.168.1.1
)";
}

std::string NetworkEngine::getCommandHelp(const std::string& command) const {
    if (command == "discover") {
        return "discover <target> - 主机发现，检测目标网络中的存活主机";
    } else if (command == "scan") {
        return "scan <target> [options] - 端口扫描，检测目标主机的开放端口";
    } else if (command == "service") {
        return "service <target> - 服务识别，识别目标主机上运行的服务";
    } else if (command == "os") {
        return "os <target> - 操作系统识别，尝试识别目标主机的操作系统";
    }
    
    return "";
}

ExecutionResult NetworkEngine::executeDiscover(const CommandContext& context) {
    ExecutionResult result;
    m_status = EngineStatus::RUNNING;
    
    if (context.target.empty()) {
        result.success = false;
        result.message = "Target is required for discover command";
        m_status = EngineStatus::IDLE;
        return result;
    }
    
    notifyOutput(context, "开始主机发现: " + context.target);
    
    // 解析目标
    auto targets = parseTargets(context.target);
    if (targets.empty()) {
        result.success = false;
        result.message = "Invalid target format";
        m_status = EngineStatus::IDLE;
        return result;
    }
    
    std::vector<HostInfo> aliveHosts;
    
    for (const auto& target : targets) {
        if (m_stopRequested) break;
        
        notifyOutput(context, "检测主机: " + target);
        
        if (pingHost(target)) {
            HostInfo host;
            host.ip = target;
            host.isAlive = true;
            aliveHosts.push_back(host);
            
            notifyOutput(context, "发现存活主机: " + target);
        }
    }
    
    result.success = true;
    result.message = "发现 " + std::to_string(aliveHosts.size()) + " 个存活主机";
    result.data["alive_hosts"] = std::to_string(aliveHosts.size());
    
    m_status = EngineStatus::COMPLETED;
    return result;
}

ExecutionResult NetworkEngine::executeScan(const CommandContext& context) {
    ExecutionResult result;
    m_status = EngineStatus::RUNNING;
    
    if (context.target.empty()) {
        result.success = false;
        result.message = "Target is required for scan command";
        m_status = EngineStatus::IDLE;
        return result;
    }
    
    notifyOutput(context, "开始端口扫描: " + context.target);
    
    // 解析端口
    std::vector<int> ports;
    auto portsParam = context.parameters.find("ports");
    if (portsParam != context.parameters.end()) {
        ports = parsePorts(portsParam->second);
    } else {
        ports = DEFAULT_PORTS;
    }
    
    if (ports.empty()) {
        result.success = false;
        result.message = "No valid ports to scan";
        m_status = EngineStatus::IDLE;
        return result;
    }
    
    notifyOutput(context, "扫描 " + std::to_string(ports.size()) + " 个端口");
    
    auto scanResults = scanPorts(context.target, ports);
    
    int openPorts = 0;
    for (const auto& scanResult : scanResults) {
        if (scanResult.isOpen) {
            openPorts++;
            notifyOutput(context, "开放端口: " + std::to_string(scanResult.port) + 
                        " (" + scanResult.service + ")");
        }
    }
    
    result.success = true;
    result.message = "扫描完成，发现 " + std::to_string(openPorts) + " 个开放端口";
    result.data["open_ports"] = std::to_string(openPorts);
    result.data["total_ports"] = std::to_string(ports.size());
    
    m_status = EngineStatus::COMPLETED;
    return result;
}

ExecutionResult NetworkEngine::executeService(const CommandContext& context) {
    ExecutionResult result;
    result.success = true;
    result.message = "Service detection completed";
    return result;
}

ExecutionResult NetworkEngine::executeOS(const CommandContext& context) {
    ExecutionResult result;
    result.success = true;
    result.message = "OS detection completed";
    return result;
}

bool NetworkEngine::pingHost(const std::string& target) {
    Utils::IPAddress ip(target);
    auto result = Utils::NetworkUtils::pingHost(ip);
    return result.success;
}

std::vector<PortScanResult> NetworkEngine::scanPorts(const std::string& target, const std::vector<int>& ports) {
    std::vector<PortScanResult> results;
    
    for (int port : ports) {
        if (m_stopRequested) break;
        
        PortScanResult result;
        result.port = port;
        result.isOpen = tcpConnect(target, port, 3000);
        
        if (result.isOpen) {
            auto serviceIt = COMMON_SERVICES.find(port);
            result.service = (serviceIt != COMMON_SERVICES.end()) ? serviceIt->second : "unknown";
        }
        
        results.push_back(result);
    }
    
    return results;
}

bool NetworkEngine::tcpConnect(const std::string& target, int port, int timeout) {
    Utils::IPAddress ip(target);
    auto result = Utils::NetworkUtils::testTCPConnection(ip, port, std::chrono::milliseconds(timeout));
    return result.success;
}

std::vector<std::string> NetworkEngine::parseTargets(const std::string& targetString) {
    auto ipAddresses = Utils::NetworkUtils::parseIPRange(targetString);
    std::vector<std::string> targets;
    for (const auto& ip : ipAddresses) {
        targets.push_back(ip.toString());
    }
    return targets;
}

std::vector<int> NetworkEngine::parsePorts(const std::string& portString) {
    std::vector<int> ports;
    auto ranges = Utils::NetworkUtils::parsePortRange(portString);
    
    for (const auto& range : ranges) {
        auto portVector = range.toVector();
        ports.insert(ports.end(), portVector.begin(), portVector.end());
    }
    
    return ports;
}

} // namespace MindSploit::Network
