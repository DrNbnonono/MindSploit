#include "network_utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <thread>
#include <future>
#include <iomanip>
#include <cstring>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#include <icmpapi.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <fcntl.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

namespace MindSploit::Utils {

// 静态成员初始化
bool NetworkUtils::s_initialized = false;
int NetworkUtils::s_lastErrorCode = 0;
std::string NetworkUtils::s_lastErrorMessage;

// IPAddress 方法实现
bool IPAddress::isValid() const {
    return NetworkUtils::isValidIP(address);
}

bool IPAddress::isPrivate() const {
    if (isIPv6) {
        // IPv6私有地址检查
        return address.substr(0, 4) == "fc00" || address.substr(0, 4) == "fd00" ||
               address.substr(0, 4) == "fe80" || address == "::1";
    } else {
        // IPv4私有地址检查
        struct sockaddr_in sa;
        if (inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) != 1) {
            return false;
        }

        uint32_t ip = ntohl(sa.sin_addr.s_addr);
        return (ip >= 0x0A000000 && ip <= 0x0AFFFFFF) ||  // 10.0.0.0/8
               (ip >= 0xAC100000 && ip <= 0xAC1FFFFF) ||  // 172.16.0.0/12
               (ip >= 0xC0A80000 && ip <= 0xC0A8FFFF);    // 192.168.0.0/16
    }
}

bool IPAddress::isLoopback() const {
    if (isIPv6) {
        return address == "::1";
    } else {
        return address.substr(0, 4) == "127.";
    }
}

// PortRange 方法实现
std::vector<uint16_t> PortRange::toVector() const {
    std::vector<uint16_t> result;
    for (uint16_t port = start; port <= end; ++port) {
        result.push_back(port);
    }
    return result;
}

void NetworkUtils::setLastError(int code, const std::string& message) {
    s_lastErrorCode = code;
    s_lastErrorMessage = message;
}

bool NetworkUtils::initialize() {
    if (s_initialized) {
        return true;
    }

#ifdef _WIN32
    if (!initializeWinsock()) {
        return false;
    }
#endif

    s_initialized = true;
    return true;
}

void NetworkUtils::cleanup() {
    if (!s_initialized) {
        return;
    }

#ifdef _WIN32
    cleanupWinsock();
#endif

    s_initialized = false;
}

bool NetworkUtils::isValidIP(const std::string& ip) {
    return isValidIPv4(ip) || isValidIPv6(ip);
}

bool NetworkUtils::isValidIPv4(const std::string& ip) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
}

bool NetworkUtils::isValidIPv6(const std::string& ip) {
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, ip.c_str(), &(sa.sin6_addr)) == 1;
}

IPAddress NetworkUtils::resolveHostname(const std::string& hostname) {
    IPAddress result;

    // 如果已经是IP地址，直接返回
    if (isValidIP(hostname)) {
        result.address = hostname;
        result.isIPv6 = isValidIPv6(hostname);
        return result;
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (status != 0) {
        setLastError(status, "Failed to resolve hostname: " + hostname);
        return result;
    }

    if (res->ai_family == AF_INET) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
        result.address = ip_str;
        result.isIPv6 = false;
    } else if (res->ai_family == AF_INET6) {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)res->ai_addr;
        char ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_str, INET6_ADDRSTRLEN);
        result.address = ip_str;
        result.isIPv6 = true;
    }

    freeaddrinfo(res);
    return result;
}

std::string NetworkUtils::reverseResolve(const IPAddress& ip) {
    struct sockaddr_storage addr;
    socklen_t addr_len;

    if (ip.isIPv6) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&addr;
        memset(addr6, 0, sizeof(*addr6));
        addr6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, ip.address.c_str(), &addr6->sin6_addr);
        addr_len = sizeof(*addr6);
    } else {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&addr;
        memset(addr4, 0, sizeof(*addr4));
        addr4->sin_family = AF_INET;
        inet_pton(AF_INET, ip.address.c_str(), &addr4->sin_addr);
        addr_len = sizeof(*addr4);
    }

    char hostname[NI_MAXHOST];
    int result = getnameinfo((struct sockaddr*)&addr, addr_len,
                           hostname, sizeof(hostname), nullptr, 0, 0);

    if (result == 0) {
        return std::string(hostname);
    }

    return ip.address; // 返回原IP地址
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

std::vector<uint16_t> NetworkUtils::parsePortList(const std::string& list) {
    std::vector<uint16_t> ports;
    std::stringstream ss(list);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // 去除空格
        item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());

        if (item.empty()) continue;

        // 检查是否是范围 (例如: 80-90)
        size_t dash_pos = item.find('-');
        if (dash_pos != std::string::npos) {
            try {
                int start = std::stoi(item.substr(0, dash_pos));
                int end = std::stoi(item.substr(dash_pos + 1));

                if (isValidPort(start) && isValidPort(end) && start <= end) {
                    for (int port = start; port <= end; ++port) {
                        ports.push_back(static_cast<uint16_t>(port));
                    }
                }
            } catch (const std::exception&) {
                // 忽略无效的端口范围
            }
        } else {
            // 单个端口
            try {
                int port = std::stoi(item);
                if (isValidPort(port)) {
                    ports.push_back(static_cast<uint16_t>(port));
                }
            } catch (const std::exception&) {
                // 忽略无效的端口
            }
        }
    }

    // 去重并排序
    std::sort(ports.begin(), ports.end());
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());

    return ports;
}

ConnectionResult NetworkUtils::testTCPConnection(const IPAddress& target, uint16_t port,
                                               std::chrono::milliseconds timeout) {
    ConnectionResult result;

    if (!isInitialized()) {
        result.errorMessage = "NetworkUtils not initialized";
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 创建套接字
    int sock = socket(target.isIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        result.errorMessage = "Failed to create socket";
        result.errorCode = errno;
        return result;
    }

    // 设置非阻塞模式
    #ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    #else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    #endif

    // 准备地址结构
    struct sockaddr_storage addr;
    socklen_t addr_len;

    if (target.isIPv6) {
        struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&addr;
        memset(addr6, 0, sizeof(*addr6));
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(port);
        inet_pton(AF_INET6, target.address.c_str(), &addr6->sin6_addr);
        addr_len = sizeof(*addr6);
    } else {
        struct sockaddr_in* addr4 = (struct sockaddr_in*)&addr;
        memset(addr4, 0, sizeof(*addr4));
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        inet_pton(AF_INET, target.address.c_str(), &addr4->sin_addr);
        addr_len = sizeof(*addr4);
    }

    // 尝试连接
    int connect_result = connect(sock, (struct sockaddr*)&addr, addr_len);

    if (connect_result == 0) {
        // 立即连接成功
        result.success = true;
    } else {
        #ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
        #else
        if (errno == EINPROGRESS) {
        #endif
            // 连接正在进行中，使用select等待
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(sock, &write_fds);

            struct timeval tv;
            tv.tv_sec = timeout.count() / 1000;
            tv.tv_usec = (timeout.count() % 1000) * 1000;

            int select_result = select(sock + 1, nullptr, &write_fds, nullptr, &tv);

            if (select_result > 0 && FD_ISSET(sock, &write_fds)) {
                // 检查连接是否成功
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len);

                if (error == 0) {
                    result.success = true;
                } else {
                    result.errorCode = error;
                    result.errorMessage = "Connection failed";
                }
            } else {
                result.errorMessage = "Connection timeout";
            }
        } else {
            result.errorCode = errno;
            result.errorMessage = "Connection failed immediately";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    #ifdef _WIN32
    closesocket(sock);
    #else
    close(sock);
    #endif

    return result;
}

ConnectionResult NetworkUtils::pingHost(const IPAddress& target, std::chrono::milliseconds timeout) {
    ConnectionResult result;

    if (!isInitialized()) {
        result.errorMessage = "NetworkUtils not initialized";
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();

#ifdef _WIN32
    // Windows ICMP实现
    HANDLE hIcmpFile = IcmpCreateFile();
    if (hIcmpFile == INVALID_HANDLE_VALUE) {
        result.errorMessage = "Failed to create ICMP handle";
        return result;
    }

    char sendData[32] = "MindSploit ping test data";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
    void* replyBuffer = malloc(replySize);

    if (!replyBuffer) {
        IcmpCloseHandle(hIcmpFile);
        result.errorMessage = "Failed to allocate reply buffer";
        return result;
    }

    // 转换IP地址
    IPAddr destAddr = inet_addr(target.address.c_str());
    if (destAddr == INADDR_NONE) {
        free(replyBuffer);
        IcmpCloseHandle(hIcmpFile);
        result.errorMessage = "Invalid IP address";
        return result;
    }

    DWORD dwRetVal = IcmpSendEcho(hIcmpFile, destAddr, sendData, sizeof(sendData),
                                  nullptr, replyBuffer, replySize, timeout.count());

    if (dwRetVal != 0) {
        PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)replyBuffer;
        if (pEchoReply->Status == IP_SUCCESS) {
            result.success = true;
            result.responseTime = std::chrono::milliseconds(pEchoReply->RoundTripTime);
        } else {
            result.errorMessage = "ICMP echo failed with status: " + std::to_string(pEchoReply->Status);
        }
    } else {
        result.errorMessage = "IcmpSendEcho failed";
    }

    free(replyBuffer);
    IcmpCloseHandle(hIcmpFile);

#else
    // Linux ICMP实现 (需要root权限或CAP_NET_RAW)
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        // 如果无法创建原始套接字，尝试使用TCP ping
        result = testTCPConnection(target, 80, timeout);
        if (!result.success) {
            result.errorMessage = "ICMP ping requires root privileges, TCP ping also failed";
        }
        return result;
    }

    // 设置超时
    struct timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 构造ICMP包
    struct icmp icmp_hdr;
    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = ICMP_ECHO;
    icmp_hdr.icmp_code = 0;
    icmp_hdr.icmp_id = getpid();
    icmp_hdr.icmp_seq = 1;
    icmp_hdr.icmp_cksum = 0;
    icmp_hdr.icmp_cksum = calculateChecksum(&icmp_hdr, sizeof(icmp_hdr));

    // 目标地址
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, target.address.c_str(), &dest_addr.sin_addr);

    // 发送ICMP包
    ssize_t sent = sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0,
                         (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    if (sent < 0) {
        close(sock);
        result.errorMessage = "Failed to send ICMP packet";
        return result;
    }

    // 接收回复
    char recv_buf[1024];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                               (struct sockaddr*)&from_addr, &from_len);

    close(sock);

    if (received > 0) {
        result.success = true;
        auto end = std::chrono::high_resolution_clock::now();
        result.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    } else {
        result.errorMessage = "No ICMP reply received";
    }
#endif

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

std::string NetworkUtils::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

uint16_t NetworkUtils::calculateChecksum(const void* data, size_t length) {
    const uint16_t* buf = static_cast<const uint16_t*>(data);
    uint32_t sum = 0;

    // 计算16位字的和
    while (length > 1) {
        sum += *buf++;
        length -= 2;
    }

    // 处理奇数字节
    if (length == 1) {
        sum += *(const uint8_t*)buf;
    }

    // 处理进位
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

std::string NetworkUtils::getErrorString(int errorCode) {
#ifdef _WIN32
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, nullptr);

    std::string result(static_cast<char*>(lpMsgBuf));
    LocalFree(lpMsgBuf);
    return result;
#else
    return std::string(strerror(errorCode));
#endif
}

bool NetworkUtils::isInitialized() {
    return s_initialized;
}

std::string NetworkUtils::getLastErrorString() {
    return s_lastErrorMessage;
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



} // namespace MindSploit::Utils
