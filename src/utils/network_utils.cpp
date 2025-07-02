#include "network_utils.h"
#include <iostream>
#include <sstream>
#include <regex>

namespace MindSploit::Utils {

// 静态成员初始化
bool NetworkUtils::s_initialized = false;
int NetworkUtils::s_lastErrorCode = 0;
std::string NetworkUtils::s_lastErrorMessage;

void NetworkUtils::setLastError(int code, const std::string& message) {
    s_lastErrorCode = code;
    s_lastErrorMessage = message;
}

bool NetworkUtils::initialize() {
#ifdef _WIN32
    return initializeWinsock();
#else
    s_initialized = true;
    return true;
#endif
}

void NetworkUtils::cleanup() {
#ifdef _WIN32
    cleanupWinsock();
#endif
    s_initialized = false;
}

bool NetworkUtils::isValidIP(const std::string& ip) {
    return isValidIPv4(ip) || isValidIPv6(ip);
}

bool NetworkUtils::isValidIPv4(const std::string& ip) {
    std::regex ipv4_regex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
    std::smatch match;
    
    if (!std::regex_match(ip, match, ipv4_regex)) {
        return false;
    }
    
    for (int i = 1; i <= 4; ++i) {
        int octet = std::stoi(match[i].str());
        if (octet < 0 || octet > 255) {
            return false;
        }
    }
    
    return true;
}

bool NetworkUtils::isValidIPv6(const std::string& ip) {
    // 简化的IPv6验证
    return ip.find(':') != std::string::npos && ip.length() >= 2;
}

IPAddress NetworkUtils::resolveHostname(const std::string& hostname) {
    // 简化实现，实际应该使用getaddrinfo
    if (isValidIP(hostname)) {
        return IPAddress(hostname);
    }
    
    // 这里应该实现DNS解析
    return IPAddress();
}

std::vector<IPAddress> NetworkUtils::parseIPRange(const std::string& range) {
    std::vector<IPAddress> result;
    
    // 检查是否是CIDR格式
    if (range.find('/') != std::string::npos) {
        return parseCIDR(range);
    }
    
    // 检查是否是范围格式 (例如: 192.168.1.1-192.168.1.10)
    size_t dashPos = range.find('-');
    if (dashPos != std::string::npos) {
        std::string startIP = range.substr(0, dashPos);
        std::string endIP = range.substr(dashPos + 1);
        
        // 简化实现，只处理最后一个八位组的范围
        if (isValidIPv4(startIP) && isValidIPv4(endIP)) {
            // 这里应该实现IP范围解析
            result.push_back(IPAddress(startIP));
            result.push_back(IPAddress(endIP));
        }
    } else {
        // 单个IP地址
        if (isValidIP(range)) {
            result.push_back(IPAddress(range));
        }
    }
    
    return result;
}

std::vector<IPAddress> NetworkUtils::parseCIDR(const std::string& cidr) {
    std::vector<IPAddress> result;
    
    size_t slashPos = cidr.find('/');
    if (slashPos == std::string::npos) {
        return result;
    }
    
    std::string baseIP = cidr.substr(0, slashPos);
    int prefixLength = std::stoi(cidr.substr(slashPos + 1));
    
    if (!isValidIPv4(baseIP) || prefixLength < 0 || prefixLength > 32) {
        return result;
    }
    
    // 简化实现，只返回网络地址
    result.push_back(IPAddress(baseIP));
    
    return result;
}

bool NetworkUtils::isValidPort(int port) {
    return port >= 1 && port <= 65535;
}

std::vector<PortRange> NetworkUtils::parsePortRange(const std::string& range) {
    std::vector<PortRange> result;
    
    std::istringstream iss(range);
    std::string token;
    
    while (std::getline(iss, token, ',')) {
        // 去除空格
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        
        size_t dashPos = token.find('-');
        if (dashPos != std::string::npos) {
            // 端口范围
            int start = std::stoi(token.substr(0, dashPos));
            int end = std::stoi(token.substr(dashPos + 1));
            
            if (isValidPort(start) && isValidPort(end) && start <= end) {
                result.emplace_back(start, end);
            }
        } else {
            // 单个端口
            int port = std::stoi(token);
            if (isValidPort(port)) {
                result.emplace_back(port);
            }
        }
    }
    
    return result;
}

ConnectionResult NetworkUtils::testTCPConnection(const IPAddress& target, uint16_t port, 
                                               std::chrono::milliseconds timeout) {
    ConnectionResult result;
    
    // 简化实现
    if (!isValidIP(target.address) || !isValidPort(port)) {
        result.errorMessage = "Invalid target or port";
        return result;
    }
    
    // 这里应该实现实际的TCP连接测试
    result.success = true;
    result.responseTime = std::chrono::milliseconds(10);
    
    return result;
}

ConnectionResult NetworkUtils::pingHost(const IPAddress& target, std::chrono::milliseconds timeout) {
    ConnectionResult result;
    
    // 简化实现
    if (!isValidIP(target.address)) {
        result.errorMessage = "Invalid target";
        return result;
    }
    
    // 这里应该实现实际的ICMP ping
    result.success = true;
    result.responseTime = std::chrono::milliseconds(5);
    
    return result;
}

std::vector<NetworkInterface> NetworkUtils::getNetworkInterfaces() {
    std::vector<NetworkInterface> interfaces;
    
#ifdef _WIN32
    return getWindowsInterfaces();
#else
    return getLinuxInterfaces();
#endif
}

IPAddress NetworkUtils::getLocalIP() {
    auto interfaces = getNetworkInterfaces();
    
    for (const auto& iface : interfaces) {
        if (!iface.isLoopback && iface.isUp && !iface.addresses.empty()) {
            for (const auto& addr : iface.addresses) {
                if (!addr.isIPv6 && !addr.isLoopback()) {
                    return addr;
                }
            }
        }
    }
    
    return IPAddress("127.0.0.1");
}

std::string NetworkUtils::formatDuration(std::chrono::milliseconds duration) {
    auto ms = duration.count();
    
    if (ms < 1000) {
        return std::to_string(ms) + "ms";
    } else if (ms < 60000) {
        return std::to_string(ms / 1000.0) + "s";
    } else {
        return std::to_string(ms / 60000.0) + "m";
    }
}

#ifdef _WIN32
bool NetworkUtils::initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        setLastError(result, "WSAStartup failed");
        return false;
    }
    
    s_initialized = true;
    return true;
}

void NetworkUtils::cleanupWinsock() {
    if (s_initialized) {
        WSACleanup();
    }
}

std::vector<NetworkInterface> NetworkUtils::getWindowsInterfaces() {
    std::vector<NetworkInterface> interfaces;
    
    // 简化实现，返回本地回环接口
    NetworkInterface loopback;
    loopback.name = "Loopback";
    loopback.description = "Software Loopback Interface";
    loopback.addresses.push_back(IPAddress("127.0.0.1"));
    loopback.isUp = true;
    loopback.isLoopback = true;
    
    interfaces.push_back(loopback);
    
    return interfaces;
}
#else
std::vector<NetworkInterface> NetworkUtils::getLinuxInterfaces() {
    std::vector<NetworkInterface> interfaces;
    
    // 简化实现，返回本地回环接口
    NetworkInterface loopback;
    loopback.name = "lo";
    loopback.description = "Loopback Interface";
    loopback.addresses.push_back(IPAddress("127.0.0.1"));
    loopback.isUp = true;
    loopback.isLoopback = true;
    
    interfaces.push_back(loopback);
    
    return interfaces;
}
#endif

// IPAddress 方法实现
bool IPAddress::isValid() const {
    return NetworkUtils::isValidIP(address);
}

bool IPAddress::isPrivate() const {
    if (isIPv6) return false;
    
    // 检查私有IP地址范围
    if (address.substr(0, 3) == "10.") return true;
    if (address.substr(0, 8) == "192.168.") return true;
    if (address.substr(0, 7) == "172.16." || 
        address.substr(0, 7) == "172.17." ||
        address.substr(0, 7) == "172.31.") return true;
    
    return false;
}

bool IPAddress::isLoopback() const {
    return address == "127.0.0.1" || address == "::1";
}

// PortRange 方法实现
std::vector<uint16_t> PortRange::toVector() const {
    std::vector<uint16_t> ports;
    for (uint16_t port = start; port <= end; ++port) {
        ports.push_back(port);
    }
    return ports;
}

} // namespace MindSploit::Utils
