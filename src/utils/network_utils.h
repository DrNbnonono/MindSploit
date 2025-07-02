#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace MindSploit::Utils {

// IP地址结构
struct IPAddress {
    std::string address;
    bool isIPv6 = false;
    
    IPAddress() = default;
    IPAddress(const std::string& addr) : address(addr), isIPv6(addr.find(':') != std::string::npos) {}
    
    bool isValid() const;
    bool isPrivate() const;
    bool isLoopback() const;
    std::string toString() const { return address; }
};

// 端口范围结构
struct PortRange {
    uint16_t start;
    uint16_t end;
    
    PortRange(uint16_t port) : start(port), end(port) {}
    PortRange(uint16_t s, uint16_t e) : start(s), end(e) {}
    
    bool contains(uint16_t port) const { return port >= start && port <= end; }
    size_t size() const { return end - start + 1; }
    std::vector<uint16_t> toVector() const;
};

// 网络接口信息
struct NetworkInterface {
    std::string name;
    std::string description;
    std::vector<IPAddress> addresses;
    std::string macAddress;
    bool isUp = false;
    bool isLoopback = false;
    uint64_t bytesReceived = 0;
    uint64_t bytesSent = 0;
};

// 连接测试结果
struct ConnectionResult {
    bool success = false;
    std::chrono::milliseconds responseTime{0};
    std::string errorMessage;
    int errorCode = 0;
};

// 网络工具类
class NetworkUtils {
public:
    // 初始化和清理
    static bool initialize();
    static void cleanup();
    
    // IP地址相关
    static bool isValidIP(const std::string& ip);
    static bool isValidIPv4(const std::string& ip);
    static bool isValidIPv6(const std::string& ip);
    static IPAddress resolveHostname(const std::string& hostname);
    static std::string reverseResolve(const IPAddress& ip);
    
    // IP范围解析
    static std::vector<IPAddress> parseIPRange(const std::string& range);
    static std::vector<IPAddress> parseCIDR(const std::string& cidr);
    static std::vector<IPAddress> parseIPList(const std::string& list);
    
    // 端口相关
    static bool isValidPort(int port);
    static std::vector<PortRange> parsePortRange(const std::string& range);
    static std::vector<uint16_t> parsePortList(const std::string& list);
    
    // 连接测试
    static ConnectionResult testTCPConnection(const IPAddress& target, uint16_t port, 
                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));
    static ConnectionResult testUDPConnection(const IPAddress& target, uint16_t port,
                                            std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));
    static ConnectionResult pingHost(const IPAddress& target,
                                   std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));
    
    // 网络接口
    static std::vector<NetworkInterface> getNetworkInterfaces();
    static NetworkInterface getDefaultInterface();
    static IPAddress getLocalIP();
    static std::vector<IPAddress> getAllLocalIPs();
    
    // 路由相关
    static IPAddress getDefaultGateway();
    static std::vector<IPAddress> getRouteToHost(const IPAddress& target);
    
    // 网络扫描辅助
    static std::string grabBanner(const IPAddress& target, uint16_t port,
                                 std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));
    static std::string detectService(uint16_t port, const std::string& banner = "");
    
    // 原始套接字相关
    static int createRawSocket(int protocol);
    static bool sendRawPacket(int socket, const void* data, size_t length, const IPAddress& target);
    static bool receiveRawPacket(int socket, void* buffer, size_t bufferSize, size_t& received);
    
    // 工具方法
    static uint16_t calculateChecksum(const void* data, size_t length);
    static std::string formatDuration(std::chrono::milliseconds duration);
    static std::string formatBytes(uint64_t bytes);
    static std::string getErrorString(int errorCode);

private:
    // 内部辅助方法
    static bool isInitialized();
    static void setLastError(int code, const std::string& message);
    static std::string getLastErrorString();
    
    // 平台相关实现
#ifdef _WIN32
    static bool initializeWinsock();
    static void cleanupWinsock();
    static std::vector<NetworkInterface> getWindowsInterfaces();
#else
    static std::vector<NetworkInterface> getLinuxInterfaces();
#endif

private:
    static bool s_initialized;
    static int s_lastErrorCode;
    static std::string s_lastErrorMessage;
};

// RAII套接字包装器
class Socket {
public:
    Socket(int domain, int type, int protocol = 0);
    ~Socket();
    
    // 禁用拷贝
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    // 移动构造
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    
    // 基本操作
    bool bind(const IPAddress& address, uint16_t port);
    bool connect(const IPAddress& address, uint16_t port, std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));
    bool listen(int backlog = 5);
    Socket accept();
    
    // 数据传输
    ssize_t send(const void* data, size_t length);
    ssize_t receive(void* buffer, size_t bufferSize);
    ssize_t sendTo(const void* data, size_t length, const IPAddress& address, uint16_t port);
    ssize_t receiveFrom(void* buffer, size_t bufferSize, IPAddress& fromAddress, uint16_t& fromPort);
    
    // 选项设置
    bool setNonBlocking(bool nonBlocking = true);
    bool setReuseAddress(bool reuse = true);
    bool setTimeout(std::chrono::milliseconds timeout);
    bool setReceiveTimeout(std::chrono::milliseconds timeout);
    bool setSendTimeout(std::chrono::milliseconds timeout);
    
    // 状态查询
    bool isValid() const { return m_socket != -1; }
    int getHandle() const { return m_socket; }
    bool isConnected() const;
    IPAddress getLocalAddress() const;
    uint16_t getLocalPort() const;
    IPAddress getRemoteAddress() const;
    uint16_t getRemotePort() const;
    
    // 关闭
    void close();

private:
    int m_socket = -1;
    bool m_connected = false;
};

} // namespace MindSploit::Utils
