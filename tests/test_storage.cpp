#include <iostream>
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "../src/core/database.h"
#include "../src/core/config_manager.h"

void testDatabase() {
    std::cout << "=== 测试数据库功能 ===" << std::endl;
    
    // 初始化数据库
    Database& db = Database::instance();
    if (!db.initialize(":memory:")) { // 使用内存数据库测试
        std::cout << "数据库初始化失败" << std::endl;
        return;
    }
    
    // 测试项目管理
    std::cout << "测试项目管理..." << std::endl;
    db.createProject("test_project", "测试项目");
    db.setCurrentProject("test_project");
    
    auto projects = db.getProjects();
    std::cout << "项目数量: " << projects.size() << std::endl;
    
    // 测试命令历史
    std::cout << "测试命令历史..." << std::endl;
    db.addCommandHistory("nmap -sS 192.168.1.1", "扫描结果...", "test_project");
    db.addCommandHistory("curl http://example.com", "HTTP响应...", "test_project");
    
    auto history = db.getCommandHistory("test_project", 10);
    std::cout << "命令历史数量: " << history.size() << std::endl;
    
    for (const auto& cmd : history) {
        std::cout << "命令: " << cmd["command"].toString().toStdString() << std::endl;
    }
    
    // 测试扫描结果
    std::cout << "测试扫描结果..." << std::endl;
    QJsonObject scanResult;
    scanResult["target"] = "192.168.1.1";
    scanResult["open_ports"] = QJsonArray{22, 80, 443};
    scanResult["os"] = "Linux";
    
    db.addScanResult("192.168.1.1", "nmap", scanResult, "test_project");
    
    auto results = db.getScanResults("test_project");
    std::cout << "扫描结果数量: " << results.size() << std::endl;
    
    // 测试AI对话
    std::cout << "测试AI对话..." << std::endl;
    db.addAIConversation("如何扫描这个目标？", "建议使用nmap进行端口扫描", "test_project");
    
    auto conversations = db.getAIConversations("test_project");
    std::cout << "AI对话数量: " << conversations.size() << std::endl;
    
    // 测试报告
    std::cout << "测试报告..." << std::endl;
    QJsonObject reportData;
    reportData["title"] = "渗透测试报告";
    reportData["target"] = "192.168.1.1";
    reportData["findings"] = QJsonArray{"开放端口", "潜在漏洞"};
    
    db.saveReport("test_report", reportData, "test_project");
    
    auto report = db.getReport("test_report", "test_project");
    std::cout << "报告标题: " << report["title"].toString().toStdString() << std::endl;
    
    // 测试统计
    std::cout << "测试统计..." << std::endl;
    int cmdCount = db.getCommandCount("test_project");
    int scanCount = db.getScanCount("test_project");
    std::cout << "命令数量: " << cmdCount << ", 扫描数量: " << scanCount << std::endl;
    
    std::cout << "数据库测试完成！" << std::endl;
}

void testConfigManager() {
    std::cout << "\n=== 测试配置管理 ===" << std::endl;
    
    ConfigManager& config = ConfigManager::instance();
    config.initialize();
    
    // 测试基本配置
    std::cout << "测试基本配置..." << std::endl;
    config.setValue("test/string", "测试字符串");
    config.setValue("test/number", 42);
    config.setValue("test/boolean", true);
    
    QString str = config.getValue("test/string").toString();
    int num = config.getValue("test/number").toInt();
    bool flag = config.getValue("test/boolean").toBool();
    
    std::cout << "字符串: " << str.toStdString() << std::endl;
    std::cout << "数字: " << num << std::endl;
    std::cout << "布尔值: " << (flag ? "true" : "false") << std::endl;
    
    // 测试AI配置
    std::cout << "测试AI配置..." << std::endl;
    config.setAIApiKey("openai", "sk-test-key");
    config.setAIModel("openai", "gpt-4");
    config.setAIEndpoint("openai", "https://api.openai.com/v1");
    
    QString apiKey = config.getAIApiKey("openai");
    QString model = config.getAIModel("openai");
    QString endpoint = config.getAIEndpoint("openai");
    
    std::cout << "API Key: " << apiKey.toStdString() << std::endl;
    std::cout << "Model: " << model.toStdString() << std::endl;
    std::cout << "Endpoint: " << endpoint.toStdString() << std::endl;
    
    // 测试工具路径
    std::cout << "测试工具路径..." << std::endl;
    config.setToolPath("nmap", "/usr/bin/nmap");
    config.setToolPath("curl", "/usr/bin/curl");
    
    QString nmapPath = config.getToolPath("nmap");
    QString curlPath = config.getToolPath("curl");
    
    std::cout << "Nmap路径: " << nmapPath.toStdString() << std::endl;
    std::cout << "Curl路径: " << curlPath.toStdString() << std::endl;
    
    auto tools = config.getAvailableTools();
    std::cout << "可用工具数量: " << tools.size() << std::endl;
    
    // 测试界面配置
    std::cout << "测试界面配置..." << std::endl;
    config.setTheme("dark");
    config.setAutoSaveInterval(300);
    
    QString theme = config.getTheme();
    int interval = config.getAutoSaveInterval();
    
    std::cout << "主题: " << theme.toStdString() << std::endl;
    std::cout << "自动保存间隔: " << interval << "秒" << std::endl;
    
    // 测试项目配置
    std::cout << "测试项目配置..." << std::endl;
    config.setLastProject("test_project");
    QString lastProject = config.getLastProject();
    std::cout << "最后项目: " << lastProject.toStdString() << std::endl;
    
    std::cout << "配置管理测试完成！" << std::endl;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("MindSploit");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("MindSploit Team");
    
    std::cout << "MindSploit 数据存储测试" << std::endl;
    std::cout << "========================" << std::endl;
    
    try {
        testDatabase();
        testConfigManager();
        
        std::cout << "\n所有测试完成！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
