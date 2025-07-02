#include "session_manager.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace MindSploit::Core {

SessionManager::SessionManager() = default;

SessionManager::~SessionManager() {
    shutdown();
}

bool SessionManager::initialize() {
    if (m_initialized) {
        return true;
    }
    
    // 创建默认会话
    initializeDefaultSession();
    
    m_initialized = true;
    return true;
}

void SessionManager::shutdown() {
    if (m_autoPersist) {
        saveAllSessions();
    }
    
    m_sessions.clear();
    m_currentSessionId.clear();
    m_defaultSessionId.clear();
    m_initialized = false;
}

std::string SessionManager::createSession(const SessionConfig& config) {
    auto session = std::make_unique<Session>();
    session->id = generateSessionId();
    session->name = config.name.empty() ? ("session_" + std::to_string(m_totalSessionsCreated + 1)) : config.name;
    session->type = config.type;
    session->status = SessionStatus::ACTIVE;
    session->createdTime = std::chrono::system_clock::now();
    session->lastAccessTime = session->createdTime;
    session->workingDirectory = ".";
    session->options = config.defaultOptions;
    session->variables = config.defaultVariables;
    
    std::string sessionId = session->id;
    m_sessions[sessionId] = std::move(session);
    m_totalSessionsCreated++;
    
    return sessionId;
}

bool SessionManager::destroySession(const std::string& sessionId) {
    if (sessionId == m_defaultSessionId) {
        return false; // 不能删除默认会话
    }
    
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }
    
    // 如果是当前会话，切换到默认会话
    if (sessionId == m_currentSessionId) {
        switchSession(m_defaultSessionId);
    }
    
    m_sessions.erase(it);
    return true;
}

bool SessionManager::switchSession(const std::string& sessionId) {
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }
    
    m_currentSessionId = sessionId;
    updateSessionAccess(sessionId);
    
    return true;
}

Session* SessionManager::getCurrentSession() {
    return getSession(m_currentSessionId);
}

Session* SessionManager::getSession(const std::string& sessionId) {
    auto it = m_sessions.find(sessionId);
    return (it != m_sessions.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> SessionManager::getSessionIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, session] : m_sessions) {
        ids.push_back(id);
    }
    return ids;
}

bool SessionManager::setVariable(const std::string& name, const std::string& value, const std::string& sessionId) {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto session = getSession(targetSessionId);
    
    if (!session) {
        return false;
    }
    
    session->variables[name] = value;
    updateSessionAccess(targetSessionId);
    
    return true;
}

std::string SessionManager::getVariable(const std::string& name, const std::string& sessionId) const {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto it = m_sessions.find(targetSessionId);
    
    if (it == m_sessions.end()) {
        return "";
    }
    
    auto& variables = it->second->variables;
    auto varIt = variables.find(name);
    return (varIt != variables.end()) ? varIt->second : "";
}

std::map<std::string, std::string> SessionManager::getAllVariables(const std::string& sessionId) const {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto it = m_sessions.find(targetSessionId);
    
    return (it != m_sessions.end()) ? it->second->variables : std::map<std::string, std::string>{};
}

bool SessionManager::setOption(const std::string& name, const std::string& value, const std::string& sessionId) {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto session = getSession(targetSessionId);
    
    if (!session) {
        return false;
    }
    
    session->options[name] = value;
    updateSessionAccess(targetSessionId);
    
    return true;
}

std::string SessionManager::getOption(const std::string& name, const std::string& sessionId) const {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto it = m_sessions.find(targetSessionId);
    
    if (it == m_sessions.end()) {
        return "";
    }
    
    auto& options = it->second->options;
    auto optIt = options.find(name);
    return (optIt != options.end()) ? optIt->second : "";
}

std::map<std::string, std::string> SessionManager::getAllOptions(const std::string& sessionId) const {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto it = m_sessions.find(targetSessionId);
    
    return (it != m_sessions.end()) ? it->second->options : std::map<std::string, std::string>{};
}

void SessionManager::addCommandToHistory(const std::string& command, const std::string& sessionId) {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto session = getSession(targetSessionId);
    
    if (!session) {
        return;
    }
    
    // 避免重复的连续命令
    if (!session->commandHistory.empty() && session->commandHistory.back() == command) {
        return;
    }
    
    session->commandHistory.push_back(command);
    
    // 限制历史记录大小
    const size_t maxHistorySize = 1000;
    if (session->commandHistory.size() > maxHistorySize) {
        session->commandHistory.erase(session->commandHistory.begin());
    }
    
    updateSessionAccess(targetSessionId);
}

std::vector<std::string> SessionManager::getCommandHistory(const std::string& sessionId) const {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto it = m_sessions.find(targetSessionId);
    
    return (it != m_sessions.end()) ? it->second->commandHistory : std::vector<std::string>{};
}

bool SessionManager::setCurrentEngine(const std::string& engineName, const std::string& sessionId) {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto session = getSession(targetSessionId);
    
    if (!session) {
        return false;
    }
    
    session->currentEngine = engineName;
    updateSessionAccess(targetSessionId);
    
    return true;
}

std::string SessionManager::getCurrentEngine(const std::string& sessionId) const {
    std::string targetSessionId = sessionId.empty() ? m_currentSessionId : sessionId;
    auto it = m_sessions.find(targetSessionId);
    
    return (it != m_sessions.end()) ? it->second->currentEngine : "";
}

void SessionManager::updateSessionAccess(const std::string& sessionId) {
    auto session = getSession(sessionId);
    if (session) {
        session->lastAccessTime = std::chrono::system_clock::now();
    }
}

size_t SessionManager::getSessionCount() const {
    return m_sessions.size();
}

size_t SessionManager::getActiveSessionCount() const {
    size_t count = 0;
    for (const auto& [id, session] : m_sessions) {
        if (session->status == SessionStatus::ACTIVE) {
            count++;
        }
    }
    return count;
}

std::string SessionManager::generateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    oss << "session_";
    
    for (int i = 0; i < 8; ++i) {
        oss << std::hex << dis(gen);
    }
    
    return oss.str();
}

void SessionManager::initializeDefaultSession() {
    SessionConfig config;
    config.name = "default";
    config.type = SessionType::INTERACTIVE;
    
    m_defaultSessionId = createSession(config);
    m_currentSessionId = m_defaultSessionId;
}

bool SessionManager::saveAllSessions() {
    // 简化实现，实际应该保存到文件
    return true;
}

bool SessionManager::loadAllSessions() {
    // 简化实现，实际应该从文件加载
    return true;
}

} // namespace MindSploit::Core
