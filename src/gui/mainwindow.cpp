#include "mainwindow.h"
#include "terminal_widget.h"
#include "core/database.h"
#include "core/config_manager.h"
#include "core/module_manager.h"
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QMessageBox>
#include <QJsonObject>
#include <QDebug>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 初始化数据存储
    initializeDataStorage();

    // 恢复窗口状态
    restoreWindowState();

    // 设置界面
    setupUI();
}

MainWindow::~MainWindow() {
    // 保存窗口状态
    saveWindowState();
}

void MainWindow::setupUI() {
    // 创建中央分割器
    m_mainSplitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(m_mainSplitter);

    // 创建可视化区域
    m_visualArea = new QTextEdit(this);
    m_visualArea->setReadOnly(true);
    m_visualArea->setFont(QFont("Consolas", 10));
    m_visualArea->setStyleSheet(
        "QTextEdit {"
        "    background-color: #2d2d30;"
        "    color: #ffffff;"
        "    border: 1px solid #3c3c3c;"
        "    selection-background-color: #264f78;"
        "}"
    );

    // 设置欢迎内容
    QString welcomeContent =
        "<h2 style='color: #00ff00;'>MindSploit - AI驱动的渗透测试框架</h2>"
        "<p style='color: #ffffff;'>欢迎使用专业的渗透测试平台！</p>"
        "<hr style='border: 1px solid #3c3c3c;'>"
        "<h3 style='color: #00ffff;'>快速开始：</h3>"
        "<ul style='color: #ffffff;'>"
        "<li>输入 <span style='color: #ffff00;'>help</span> 查看可用命令</li>"
        "<li>输入 <span style='color: #ffff00;'>banner</span> 显示项目横幅</li>"
        "<li>输入 <span style='color: #ffff00;'>nmap -sS target</span> 进行网络扫描</li>"
        "<li>使用 <span style='color: #ffff00;'>↑↓</span> 键浏览命令历史</li>"
        "</ul>"
        "<hr style='border: 1px solid #3c3c3c;'>"
        "<p style='color: #888888;'>当前时间: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "</p>";

    m_visualArea->setHtml(welcomeContent);

    // 创建专业终端组件
    m_terminal = new TerminalWidget(this);

    // 设置分割器
    m_mainSplitter->addWidget(m_visualArea);
    m_mainSplitter->addWidget(m_terminal);
    m_mainSplitter->setStretchFactor(0, 3); // 可视化区域占3/5
    m_mainSplitter->setStretchFactor(1, 2); // 终端区域占2/5

    // 设置菜单栏和工具栏
    setupMenuBar();

    // 连接终端信号
    connect(m_terminal, &TerminalWidget::commandEntered,
            this, &MainWindow::onTerminalCommand);

    // 设置窗口属性
    setWindowTitle("MindSploit - AI渗透测试框架 v1.0.0");
    setWindowIcon(QIcon(":/icons/mindsploit.ico")); // 如果有图标的话
    resize(1200, 800);

    // 设置状态栏
    statusBar()->showMessage("就绪 - 等待命令输入", 2000);
}

void MainWindow::initializeDataStorage() {
    // 初始化配置管理器
    ConfigManager::instance().initialize();

    // 初始化数据库
    if (!Database::instance().initialize()) {
        QMessageBox::critical(this, "错误",
            "数据库初始化失败！\n\n请检查应用程序是否有写入权限。");
        return;
    }

    // 初始化模块管理器
    ModuleManager& moduleManager = ModuleManager::instance();

    // 连接模块管理器信号
    connect(&moduleManager, &ModuleManager::moduleExecutionStarted,
            [this](const QString& name) {
                m_terminal->appendInfo(QString("模块 %1 开始执行...").arg(name));
            });

    connect(&moduleManager, &ModuleManager::moduleExecutionCompleted,
            [this](const QString& name, const QJsonObject& results) {
                m_terminal->appendSuccess(QString("模块 %1 执行完成").arg(name));

                // 更新可视化区域显示结果
                updateModuleResults(name, results);
            });

    connect(&moduleManager, &ModuleManager::moduleExecutionFailed,
            [this](const QString& name, const QString& error) {
                m_terminal->appendError(QString("模块 %1 执行失败: %2").arg(name, error));
            });

    // 设置当前项目
    QString lastProject = ConfigManager::instance().getLastProject();
    Database::instance().setCurrentProject(lastProject);

    qDebug() << "数据存储和模块管理器初始化完成，当前项目:" << lastProject;

    // 显示可用模块信息
    QStringList availableModules = moduleManager.getAvailableModules();
    m_terminal->appendInfo(QString("已加载 %1 个模块: %2")
                          .arg(availableModules.size())
                          .arg(availableModules.join(", ")));
}

void MainWindow::setupMenuBar() {
    // 创建菜单栏
    QMenuBar* menuBar = this->menuBar();

    // 文件菜单
    QMenu* fileMenu = menuBar->addMenu("文件(&F)");
    fileMenu->addAction("新建项目", this, [this]() {
        // TODO: 实现新建项目功能
        m_terminal->appendInfo("新建项目功能开发中...");
    });
    fileMenu->addAction("打开项目", this, [this]() {
        // TODO: 实现打开项目功能
        m_terminal->appendInfo("打开项目功能开发中...");
    });
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close);

    // 视图菜单
    QMenu* viewMenu = menuBar->addMenu("视图(&V)");
    viewMenu->addAction("显示历史", this, &MainWindow::onShowHistory);
    viewMenu->addAction("显示项目", this, &MainWindow::onShowProjects);
    viewMenu->addAction("显示横幅", this, &MainWindow::onShowBanner);
    viewMenu->addSeparator();
    viewMenu->addAction("清空终端", m_terminal, &TerminalWidget::clear);

    // 工具菜单
    QMenu* toolsMenu = menuBar->addMenu("工具(&T)");
    toolsMenu->addAction("Nmap扫描", this, [this]() {
        m_terminal->appendInfo("请在终端中输入: nmap -sS <target>");
    });
    toolsMenu->addAction("AI助手", this, [this]() {
        m_terminal->appendInfo("AI助手功能开发中...");
    });

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu("帮助(&H)");
    helpMenu->addAction("关于", this, [this]() {
        QMessageBox::about(this, "关于 MindSploit",
            "MindSploit v1.0.0\n"
            "AI驱动的渗透测试框架\n\n"
            "开发团队: MindSploit Team\n"
            "技术栈: Qt6 + C++ + SQLite");
    });
}

void MainWindow::onTerminalCommand(const QString& command) {
    // 更新状态栏
    statusBar()->showMessage(QString("执行命令: %1").arg(command), 3000);

    // 保存命令到数据库
    Database::instance().addCommandHistory(command, "", Database::instance().getCurrentProject());

    // 判断命令类型并执行
    if (command.startsWith("help") || command.startsWith("clear") ||
        command.startsWith("banner") || command.startsWith("history") ||
        command.startsWith("projects") || command.startsWith("version")) {
        executeBuiltinCommand(command);
    } else {
        executePentestCommand(command);
    }
}

void MainWindow::executeBuiltinCommand(const QString& command) {
    if (command == "help") {
        QString helpText =
            "MindSploit 可用命令:\n\n"
            "基础命令:\n"
            "  help                    - 显示此帮助信息\n"
            "  clear                   - 清空终端\n"
            "  banner                  - 显示项目横幅\n"
            "  version                 - 显示版本信息\n"
            "  history                 - 显示命令历史\n"
            "  projects                - 显示项目列表\n\n"
            "渗透测试命令:\n"
            "  nmap [options] <target> - 网络扫描\n"
            "  scan <target>           - 快速扫描\n"
            "  exploit <module>        - 使用漏洞利用模块\n"
            "  search <keyword>        - 搜索模块或漏洞\n\n"
            "AI助手:\n"
            "  ai <question>           - 询问AI助手\n"
            "  analyze <result>        - AI分析结果\n\n"
            "使用 ↑↓ 键浏览命令历史，Tab 键自动补全";

        m_terminal->appendOutput(helpText, "#00ffff");
        updateVisualArea("<h3>帮助信息</h3><p>命令帮助已在终端显示</p>");

    } else if (command == "clear") {
        m_terminal->clear();

    } else if (command == "banner") {
        m_terminal->showBanner();

    } else if (command == "version") {
        m_terminal->appendInfo("MindSploit v1.0.0 - AI驱动的渗透测试框架");
        m_terminal->appendInfo("构建时间: " + QString(__DATE__) + " " + QString(__TIME__));
        m_terminal->appendInfo("Qt版本: " + QString(QT_VERSION_STR));

    } else if (command == "history") {
        onShowHistory();

    } else if (command == "projects") {
        onShowProjects();
    }
}

void MainWindow::executePentestCommand(const QString& command) {
    // 使用模块管理器执行命令
    ModuleManager& moduleManager = ModuleManager::instance();

    // 解析命令
    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        m_terminal->appendError("空命令");
        return;
    }

    QString moduleName = parts.first();
    QStringList args = parts.mid(1);

    // 检查模块是否存在
    if (!moduleManager.getAvailableModules().contains(moduleName)) {
        // 尝试一些命令别名
        if (moduleName == "scan" && !args.isEmpty()) {
            moduleName = "portscan";
            // 重新组织参数
            QJsonObject options;
            options["target"] = args.first();
            if (args.size() > 1) {
                options["start_port"] = args[1].toInt();
            }
            if (args.size() > 2) {
                options["end_port"] = args[2].toInt();
            }

            if (moduleManager.executeModule(moduleName, options)) {
                return;
            }
        }

        m_terminal->appendError("未知模块: " + moduleName);
        m_terminal->appendInfo("可用模块: " + moduleManager.getAvailableModules().join(", "));
        return;
    }

    // 检查模块是否可用
    if (!moduleManager.isModuleAvailable(moduleName)) {
        QStringList missing = moduleManager.getMissingDependencies(moduleName);
        m_terminal->appendError(QString("模块 %1 不可用").arg(moduleName));
        if (!missing.isEmpty()) {
            m_terminal->appendWarning("缺少依赖: " + missing.join(", "));
        }
        return;
    }

    // 构建选项
    QJsonObject options;

    // 解析命令行参数
    for (int i = 0; i < args.size(); ++i) {
        QString arg = args[i];

        if (arg.startsWith("-")) {
            // 选项参数
            QString key = arg.mid(1);
            if (i + 1 < args.size() && !args[i + 1].startsWith("-")) {
                options[key] = args[i + 1];
                i++; // 跳过值
            } else {
                options[key] = true; // 布尔选项
            }
        } else {
            // 位置参数
            if (i == 0) {
                options["target"] = arg;
            } else if (i == 1 && moduleName == "portscan") {
                options["start_port"] = arg.toInt();
            } else if (i == 2 && moduleName == "portscan") {
                options["end_port"] = arg.toInt();
            }
        }
    }

    // 执行模块
    if (moduleManager.executeModule(moduleName, options)) {
        m_terminal->appendInfo(QString("正在执行模块: %1").arg(moduleName));
    } else {
        m_terminal->appendError(QString("模块 %1 执行失败").arg(moduleName));
    }
}

void MainWindow::onShowHistory() {
    // 获取命令历史
    QList<QJsonObject> history = Database::instance().getCommandHistory("", 20);

    m_terminal->appendInfo("显示最近20条命令历史:");

    if (history.isEmpty()) {
        m_terminal->appendWarning("暂无命令历史记录");
        return;
    }

    for (const auto& cmd : history) {
        QString timeStr = cmd["created_at"].toString();
        QString cmdStr = cmd["command"].toString();
        m_terminal->appendOutput(QString("[%1] %2").arg(timeStr, cmdStr), "#cccccc");
    }

    // 更新可视化区域
    QString historyHtml = "<h3>命令历史记录</h3><table border='1' style='border-collapse: collapse; color: white;'>";
    historyHtml += "<tr><th>时间</th><th>命令</th></tr>";

    for (const auto& cmd : history) {
        historyHtml += QString("<tr><td>%1</td><td>%2</td></tr>")
            .arg(cmd["created_at"].toString())
            .arg(cmd["command"].toString().toHtmlEscaped());
    }
    historyHtml += "</table>";

    updateVisualArea(historyHtml);
}

void MainWindow::onShowProjects() {
    // 获取项目列表
    QList<QJsonObject> projects = Database::instance().getProjects();

    m_terminal->appendInfo("显示项目列表:");

    if (projects.isEmpty()) {
        m_terminal->appendWarning("暂无项目记录");
        return;
    }

    for (const auto& project : projects) {
        QString projectInfo = QString("项目: %1 | 描述: %2 | 创建时间: %3")
            .arg(project["name"].toString())
            .arg(project["description"].toString())
            .arg(project["created_at"].toString());
        m_terminal->appendOutput(projectInfo, "#00ffff");
    }

    // 更新可视化区域
    QString projectsHtml = "<h3>项目管理</h3><table border='1' style='border-collapse: collapse; color: white;'>";
    projectsHtml += "<tr><th>项目名称</th><th>描述</th><th>创建时间</th></tr>";

    for (const auto& project : projects) {
        projectsHtml += QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
            .arg(project["name"].toString().toHtmlEscaped())
            .arg(project["description"].toString().toHtmlEscaped())
            .arg(project["created_at"].toString());
    }
    projectsHtml += "</table>";

    updateVisualArea(projectsHtml);
}

void MainWindow::onShowBanner() {
    m_terminal->showBanner();
}

void MainWindow::updateVisualArea(const QString& content) {
    QString styledContent = QString(
        "<div style='background-color: #2d2d30; color: #ffffff; padding: 10px;'>"
        "%1"
        "</div>"
    ).arg(content);

    m_visualArea->setHtml(styledContent);
}

void MainWindow::updateModuleResults(const QString& moduleName, const QJsonObject& results) {
    QString content;

    if (moduleName == "nmap") {
        content = formatNmapResults(results);
    } else if (moduleName == "portscan") {
        content = formatPortScanResults(results);
    } else if (moduleName == "searchsploit") {
        content = formatSearchsploitResults(results);
    } else if (moduleName == "curl") {
        content = formatCurlResults(results);
    } else {
        // 通用结果显示
        content = QString("<h3>%1 执行结果</h3>").arg(moduleName);
        content += "<pre style='color: #ffffff; background-color: #1e1e1e; padding: 10px;'>";
        content += QJsonDocument(results).toJson();
        content += "</pre>";
    }

    updateVisualArea(content);

    // 保存结果到数据库
    Database::instance().addScanResult(
        results["target"].toString(),
        moduleName,
        results,
        Database::instance().getCurrentProject()
    );
}

QString MainWindow::formatNmapResults(const QJsonObject& results) {
    QString content = "<h3>Nmap扫描结果</h3>";

    QJsonArray hosts = results["hosts"].toArray();
    if (hosts.isEmpty()) {
        content += "<p>未发现主机</p>";
        return content;
    }

    for (const QJsonValue& hostValue : hosts) {
        QJsonObject host = hostValue.toObject();
        QString hostName = host["host"].toString();

        content += QString("<h4>主机: %1</h4>").arg(hostName);
        content += "<table border='1' style='border-collapse: collapse; color: white; width: 100%;'>";
        content += "<tr><th>端口</th><th>协议</th><th>状态</th><th>服务</th></tr>";

        QJsonArray ports = host["ports"].toArray();
        for (const QJsonValue& portValue : ports) {
            QJsonObject port = portValue.toObject();
            QString state = port["state"].toString();
            QString color = (state == "open") ? "#00ff00" : "#ff0000";

            content += QString("<tr><td>%1</td><td>%2</td><td style='color: %3;'>%4</td><td>%5</td></tr>")
                      .arg(port["port"].toInt())
                      .arg(port["protocol"].toString())
                      .arg(color)
                      .arg(state)
                      .arg(port["service"].toString());
        }

        content += "</table><br>";
    }

    return content;
}

QString MainWindow::formatPortScanResults(const QJsonObject& results) {
    QString content = "<h3>端口扫描结果</h3>";

    QString target = results["target"].toString();
    int totalPorts = results["total_ports"].toInt();
    int openCount = results["open_count"].toInt();

    content += QString("<p><strong>目标:</strong> %1</p>").arg(target);
    content += QString("<p><strong>扫描端口:</strong> %1</p>").arg(totalPorts);
    content += QString("<p><strong>开放端口:</strong> %1</p>").arg(openCount);

    QJsonArray openPorts = results["open_ports"].toArray();
    if (!openPorts.isEmpty()) {
        content += "<table border='1' style='border-collapse: collapse; color: white; width: 100%;'>";
        content += "<tr><th>端口</th><th>协议</th><th>服务</th></tr>";

        for (const QJsonValue& portValue : openPorts) {
            QJsonObject port = portValue.toObject();
            content += QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                      .arg(port["port"].toInt())
                      .arg(port["protocol"].toString())
                      .arg(port["service"].toString());
        }

        content += "</table>";
    } else {
        content += "<p>未发现开放端口</p>";
    }

    return content;
}

QString MainWindow::formatSearchsploitResults(const QJsonObject& results) {
    QString content = "<h3>Searchsploit搜索结果</h3>";

    QString query = results["query"].toString();
    int count = results["count"].toInt();

    content += QString("<p><strong>搜索关键词:</strong> %1</p>").arg(query);
    content += QString("<p><strong>找到漏洞:</strong> %1 个</p>").arg(count);

    QJsonArray exploits = results["exploits"].toArray();
    if (!exploits.isEmpty()) {
        content += "<table border='1' style='border-collapse: collapse; color: white; width: 100%;'>";
        content += "<tr><th>漏洞标题</th><th>路径</th></tr>";

        for (const QJsonValue& exploitValue : exploits) {
            QJsonObject exploit = exploitValue.toObject();
            content += QString("<tr><td>%1</td><td>%2</td></tr>")
                      .arg(exploit["title"].toString().toHtmlEscaped())
                      .arg(exploit["path"].toString().toHtmlEscaped());
        }

        content += "</table>";
    } else {
        content += "<p>未找到相关漏洞</p>";
    }

    return content;
}

QString MainWindow::formatCurlResults(const QJsonObject& results) {
    QString content = "<h3>HTTP请求结果</h3>";

    QString url = results["url"].toString();
    QString method = results["method"].toString();
    int statusCode = results["status_code"].toInt();

    content += QString("<p><strong>URL:</strong> %1</p>").arg(url);
    content += QString("<p><strong>方法:</strong> %1</p>").arg(method);

    if (statusCode > 0) {
        QString statusColor = (statusCode >= 200 && statusCode < 300) ? "#00ff00" : "#ff0000";
        content += QString("<p><strong>状态码:</strong> <span style='color: %1;'>%2</span></p>")
                  .arg(statusColor).arg(statusCode);
    }

    QString response = results["response"].toString();
    if (!response.isEmpty()) {
        content += "<h4>响应内容:</h4>";
        content += "<pre style='color: #ffffff; background-color: #1e1e1e; padding: 10px; max-height: 400px; overflow-y: auto;'>";
        content += response.toHtmlEscaped();
        content += "</pre>";
    }

    return content;
}

void MainWindow::saveWindowState() {
    ConfigManager& config = ConfigManager::instance();
    config.setWindowGeometry(saveGeometry());
    config.setWindowState(saveState());
}

void MainWindow::restoreWindowState() {
    ConfigManager& config = ConfigManager::instance();

    QByteArray geometry = config.getWindowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    QByteArray state = config.getWindowState();
    if (!state.isEmpty()) {
        restoreState(state);
    }
}