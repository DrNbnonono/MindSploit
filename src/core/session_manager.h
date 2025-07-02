#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <chrono>

namespace MindSploit::Core {

// 会话状态
enum class SessionStatus {
    ACTIVE,     // 活跃状态
    INACTIVE,   // 非活跃状态
    SUSPENDED,  // 挂起状态
    EXPIRED     // 已过期
};

// 会话类型
enum class SessionType {
    INTERACTIVE,    // 交互式会话
    BATCH,         // 批处理会话
    REMOTE,        // 远程会话
    BACKGROUND     // 后台会话
};

// 会话信息
struct Session {
    std::string id;
    std::string name;
    SessionType type;
    SessionStatus status;
    std::chrono::system_clock::time_point createdTime;
    std::chrono::system_clock::time_point lastAccessTime;
    std::map<std::string, std::string> variables;
    std::map<std::string, std::string> options;
    std::string workingDirectory;
    std::string currentEngine;
    std::vector<std::string> commandHistory;
    std::map<std::string, std::string> metadata;
};

// 会话配置
struct SessionConfig {
    std::string name;
    SessionType type = SessionType::INTERACTIVE;
    int maxIdleTime = 3600; // 最大空闲时间(秒)
    int maxLifetime = 86400; // 最大生存时间(秒)
    bool persistHistory = true;
    bool autoSave = true;
    std::map<std::string, std::string> defaultOptions;
    std::map<std::string, std::string> defaultVariables;
};

// 会话管理器
class SessionManager {
public:
    SessionManager();
    ~SessionManager();

    // 初始化和清理
    bool initialize();
    void shutdown();
    
    // 会话生命周期
    std::string createSession(const SessionConfig& config = SessionConfig{});
    bool destroySession(const std::string& sessionId);
    bool switchSession(const std::string& sessionId);
    bool suspendSession(const std::string& sessionId);
    bool resumeSession(const std::string& sessionId);
    
    // 会话查询
    Session* getCurrentSession();
    Session* getSession(const std::string& sessionId);
    std::vector<std::string> getSessionIds() const;
    std::vector<Session*> getAllSessions();
    std::vector<Session*> getActiveSessions();
    
    // 会话状态管理
    bool isSessionActive(const std::string& sessionId) const;
    SessionStatus getSessionStatus(const std::string& sessionId) const;
    void updateSessionAccess(const std::string& sessionId);
    void cleanupExpiredSessions();
    
    // 变量管理
    bool setVariable(const std::string& name, const std::string& value, 
                    const std::string& sessionId = "");
    std::string getVariable(const std::string& name, const std::string& sessionId = "") const;
    bool unsetVariable(const std::string& name, const std::string& sessionId = "");
    std::map<std::string, std::string> getAllVariables(const std::string& sessionId = "") const;
    
    // 选项管理
    bool setOption(const std::string& name, const std::string& value,
                  const std::string& sessionId = "");
    std::string getOption(const std::string& name, const std::string& sessionId = "") const;
    bool unsetOption(const std::string& name, const std::string& sessionId = "");
    std::map<std::string, std::string> getAllOptions(const std::string& sessionId = "") const;
    
    // 命令历史
    void addCommandToHistory(const std::string& command, const std::string& sessionId = "");
    std::vector<std::string> getCommandHistory(const std::string& sessionId = "") const;
    void clearCommandHistory(const std::string& sessionId = "");
    
    // 工作目录管理
    bool setWorkingDirectory(const std::string& path, const std::string& sessionId = "");
    std::string getWorkingDirectory(const std::string& sessionId = "") const;
    
    // 当前引擎管理
    bool setCurrentEngine(const std::string& engineName, const std::string& sessionId = "");
    std::string getCurrentEngine(const std::string& sessionId = "") const;
    
    // 会话持久化
    bool saveSession(const std::string& sessionId);
    bool loadSession(const std::string& sessionId);
    bool saveAllSessions();
    bool loadAllSessions();
    
    // 会话导入导出
    bool exportSession(const std::string& sessionId, const std::string& filePath);
    bool importSession(const std::string& filePath);
    
    // 会话统计
    size_t getSessionCount() const;
    size_t getActiveSessionCount() const;
    std::chrono::system_clock::time_point getSessionCreatedTime(const std::string& sessionId) const;
    std::chrono::system_clock::time_point getSessionLastAccessTime(const std::string& sessionId) const;
    
    // 会话搜索
    std::vector<std::string> findSessionsByName(const std::string& pattern) const;
    std::vector<std::string> findSessionsByVariable(const std::string& name, 
                                                   const std::string& value) const;

private:
    // 内部方法
    std::string generateSessionId();
    bool validateSessionId(const std::string& sessionId) const;
    void initializeDefaultSession();
    Session* getSessionInternal(const std::string& sessionId);
    void updateSessionStatus(Session* session);
    
    // 持久化相关
    std::string getSessionFilePath(const std::string& sessionId) const;
    bool saveSessionToFile(const Session& session, const std::string& filePath);
    bool loadSessionFromFile(const std::string& filePath, Session& session);
    
    // 清理相关
    void startCleanupTimer();
    void stopCleanupTimer();

private:
    std::map<std::string, std::unique_ptr<Session>> m_sessions;
    std::string m_currentSessionId;
    std::string m_defaultSessionId;
    
    // 配置
    std::string m_sessionDirectory;
    bool m_autoPersist = true;
    int m_cleanupInterval = 300; // 清理间隔(秒)
    
    // 统计信息
    size_t m_totalSessionsCreated = 0;
    std::chrono::system_clock::time_point m_lastCleanup;
    
    bool m_initialized = false;
};

// 会话作用域变量管理器
class ScopedSessionVariable {
public:
    ScopedSessionVariable(SessionManager* manager, const std::string& name, 
                         const std::string& value, const std::string& sessionId = "");
    ~ScopedSessionVariable();

private:
    SessionManager* m_manager;
    std::string m_name;
    std::string m_sessionId;
    std::string m_originalValue;
    bool m_hadOriginalValue;
};

} // namespace MindSploit::Core
