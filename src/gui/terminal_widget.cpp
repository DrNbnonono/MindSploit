#include "terminal_widget.h"
#include <QApplication>
#include <QScrollBar>
#include <QDateTime>
#include <QKeyEvent>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QFont>
#include <QFontMetrics>

// 静态常量定义
const QString TerminalWidget::DEFAULT_PROMPT = "MindSploit";
const QString TerminalWidget::WELCOME_MESSAGE = 
    "Welcome to MindSploit - AI-Powered Penetration Testing Framework\n"
    "Type 'help' for available commands or 'banner' to show banner\n";
const QString TerminalWidget::BANNER = 
    R"(
    __  __ _           _ ____        _       _ _   
   |  \/  (_)_ __   __| / ___| _ __ | | ___ (_) |_ 
   | |\/| | | '_ \ / _` \___ \| '_ \| |/ _ \| | __|
   | |  | | | | | | (_| |___) | |_) | | (_) | | |_ 
   |_|  |_|_|_| |_|\__,_|____/| .__/|_|\___/|_|\__|
                              |_|                  
   
   AI-Powered Penetration Testing Framework v1.0.0
   https://github.com/your-repo/mindsploit
   
)";
const int TerminalWidget::MAX_HISTORY_SIZE = 1000;

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_prompt(DEFAULT_PROMPT)
    , m_currentDirectory("~")
    , m_environmentInfo("Windows")
    , m_historyIndex(-1)
    , m_readOnly(false)
    , m_cursorVisible(true)
{
    setupUI();
    setupCompleter();
    setupSyntaxHighlighting();
    
    // 设置定时器用于光标闪烁
    m_cursorTimer = new QTimer(this);
    connect(m_cursorTimer, &QTimer::timeout, this, &TerminalWidget::blinkCursor);
    m_cursorTimer->start(500); // 500ms闪烁间隔
    
    // 显示欢迎信息
    showWelcomeMessage();
    insertPrompt();
}

TerminalWidget::~TerminalWidget() = default;

void TerminalWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(0);
    
    // 创建输出区域
    m_outputArea = new QTextEdit(this);
    m_outputArea->setReadOnly(true);
    m_outputArea->setFont(QFont("Consolas", 10));
    m_outputArea->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #ffffff;"
        "    border: 1px solid #3c3c3c;"
        "    selection-background-color: #264f78;"
        "}"
    );
    
    // 创建输入区域
    m_inputWidget = new QWidget(this);
    m_inputLayout = new QHBoxLayout(m_inputWidget);
    m_inputLayout->setContentsMargins(0, 0, 0, 0);
    m_inputLayout->setSpacing(0);
    
    // 创建提示符标签
    m_promptLabel = new QLabel(this);
    m_promptLabel->setFont(QFont("Consolas", 10, QFont::Bold));
    m_promptLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #1e1e1e;"
        "    color: #00ff00;"
        "    padding: 2px 5px;"
        "    border: 1px solid #3c3c3c;"
        "    border-right: none;"
        "}"
    );
    updatePrompt();
    
    // 创建命令输入框
    m_commandInput = new QLineEdit(this);
    m_commandInput->setFont(QFont("Consolas", 10));
    m_commandInput->setStyleSheet(
        "QLineEdit {"
        "    background-color: #1e1e1e;"
        "    color: #ffffff;"
        "    border: 1px solid #3c3c3c;"
        "    border-left: none;"
        "    padding: 2px 5px;"
        "}"
    );
    
    // 组装布局
    m_inputLayout->addWidget(m_promptLabel);
    m_inputLayout->addWidget(m_commandInput);
    
    m_mainLayout->addWidget(m_outputArea, 1);
    m_mainLayout->addWidget(m_inputWidget);
    
    // 连接信号槽
    connect(m_commandInput, &QLineEdit::returnPressed, 
            this, &TerminalWidget::onCommandInputReturnPressed);
    connect(m_commandInput, &QLineEdit::textChanged,
            this, &TerminalWidget::onCommandInputTextChanged);
    
    // 安装事件过滤器
    m_commandInput->installEventFilter(this);
}

void TerminalWidget::setupCompleter() {
    // 创建命令补全器
    QStringList commands = {
        "help", "clear", "exit", "banner", "version",
        "use", "show", "set", "unset", "run",
        "nmap", "scan", "exploit", "payload",
        "search", "info", "options", "back",
        "sessions", "jobs", "kill", "resource",
        "save", "load", "history", "alias"
    };
    
    m_completerModel = new QStringListModel(commands, this);
    m_completer = new QCompleter(m_completerModel, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_commandInput->setCompleter(m_completer);
}

void TerminalWidget::setupSyntaxHighlighting() {
    // 这里可以添加语法高亮功能
    // 暂时保留接口，后续实现
}

void TerminalWidget::showWelcomeMessage() {
    appendOutput(WELCOME_MESSAGE, "#00ff00");
}

void TerminalWidget::showBanner() {
    appendOutput(BANNER, "#00ffff");
}

void TerminalWidget::appendOutput(const QString& text, const QString& color) {
    QString formattedText = formatOutput(text, color);
    m_outputArea->append(formattedText);
    scrollToBottom();
}

void TerminalWidget::appendError(const QString& text) {
    appendOutput("[ERROR] " + text, "#ff0000");
}

void TerminalWidget::appendSuccess(const QString& text) {
    appendOutput("[SUCCESS] " + text, "#00ff00");
}

void TerminalWidget::appendWarning(const QString& text) {
    appendOutput("[WARNING] " + text, "#ffff00");
}

void TerminalWidget::appendInfo(const QString& text) {
    appendOutput("[INFO] " + text, "#00ffff");
}

QString TerminalWidget::formatOutput(const QString& text, const QString& color) const {
    return QString("<span style='color: %1;'>%2</span>").arg(color, text.toHtmlEscaped());
}

void TerminalWidget::insertPrompt() {
    QString promptText = QString("<span style='color: #00ff00; font-weight: bold;'>%1> </span>")
                        .arg(m_prompt);
    m_outputArea->insertHtml(promptText);
    scrollToBottom();
}

void TerminalWidget::updatePrompt() {
    QString promptText = QString("%1> ").arg(m_prompt);
    m_promptLabel->setText(promptText);
}

void TerminalWidget::onCommandInputReturnPressed() {
    QString command = m_commandInput->text().trimmed();
    if (command.isEmpty()) {
        insertPrompt();
        return;
    }
    
    // 显示用户输入的命令
    QString commandLine = QString("<span style='color: #ffffff;'>%1</span>").arg(command.toHtmlEscaped());
    m_outputArea->insertHtml(commandLine);
    m_outputArea->append(""); // 换行
    
    // 添加到历史记录
    addToHistory(command);
    
    // 清空输入框
    m_commandInput->clear();
    m_historyIndex = -1;
    
    // 发射命令信号
    emit commandEntered(command);
}

void TerminalWidget::onCommandInputTextChanged() {
    // 这里可以添加实时命令验证或提示
}

void TerminalWidget::addToHistory(const QString& command) {
    if (!command.isEmpty() && (m_commandHistory.isEmpty() || m_commandHistory.last() != command)) {
        m_commandHistory.append(command);
        if (m_commandHistory.size() > MAX_HISTORY_SIZE) {
            m_commandHistory.removeFirst();
        }
    }
}

bool TerminalWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_commandInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        switch (keyEvent->key()) {
        case Qt::Key_Up:
            navigateHistory(-1);
            return true;
        case Qt::Key_Down:
            navigateHistory(1);
            return true;
        case Qt::Key_Tab:
            // Tab补全由QCompleter处理
            break;
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void TerminalWidget::navigateHistory(int direction) {
    if (m_commandHistory.isEmpty()) {
        return;
    }
    
    if (direction < 0) { // 向上
        if (m_historyIndex < 0) {
            m_historyIndex = m_commandHistory.size() - 1;
        } else if (m_historyIndex > 0) {
            m_historyIndex--;
        }
    } else { // 向下
        if (m_historyIndex >= 0 && m_historyIndex < m_commandHistory.size() - 1) {
            m_historyIndex++;
        } else {
            m_historyIndex = -1;
            m_commandInput->clear();
            return;
        }
    }
    
    if (m_historyIndex >= 0 && m_historyIndex < m_commandHistory.size()) {
        m_commandInput->setText(m_commandHistory[m_historyIndex]);
    }
}

void TerminalWidget::clear() {
    m_outputArea->clear();
    showWelcomeMessage();
    insertPrompt();
    emit terminalCleared();
}

void TerminalWidget::setPrompt(const QString& prompt) {
    m_prompt = prompt;
    updatePrompt();
}

QString TerminalWidget::getPrompt() const {
    return m_prompt;
}

void TerminalWidget::scrollToBottom() {
    QScrollBar *scrollBar = m_outputArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void TerminalWidget::blinkCursor() {
    // 光标闪烁效果（可选实现）
    m_cursorVisible = !m_cursorVisible;
}

void TerminalWidget::focusCommandInput() {
    m_commandInput->setFocus();
}

QString TerminalWidget::getCurrentTimeStamp() const {
    return QDateTime::currentDateTime().toString("hh:mm:ss");
}

void TerminalWidget::executeCommand(const QString& command) {
    // 这个方法由外部调用来执行命令
    onCommandInputReturnPressed();
}

void TerminalWidget::onCommandFinished(const QString& output, int exitCode) {
    // 命令执行完成后的处理
    if (exitCode == 0) {
        appendSuccess("命令执行成功");
    } else {
        appendError("命令执行失败，退出码: " + QString::number(exitCode));
    }

    if (!output.isEmpty()) {
        appendOutput(output);
    }

    insertPrompt();
    emit commandCompleted("", exitCode);
}

void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    // 处理全局按键事件
    QWidget::keyPressEvent(event);
}
