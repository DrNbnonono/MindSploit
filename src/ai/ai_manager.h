#pragma once
#include <string>

namespace MindSploit::AI {

class AIManager {
public:
    AIManager();
    ~AIManager();

    // 初始化和清理
    bool initialize();
    void shutdown();

    // AI查询处理
    std::string processQuery(const std::string& query);

    // 状态查询
    bool isAvailable() const;

private:
    bool m_initialized = false;
};

} // namespace MindSploit::AI