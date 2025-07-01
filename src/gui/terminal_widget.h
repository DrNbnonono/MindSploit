#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCompleter>
#include <QStringListModel>
#include <QTimer>
#include <QKeyEvent>

/**
 * @brief 专业的终端组件 - 实现类似msfconsole的命令行界面
 * 
 * 功能特性：
 * - MindSploit> 命令提示符
 * - 命令历史记录和导航
 * - 自动补全功能
 * - 多彩输出显示
 * - AI助手集成
 * - 命令别名支持
 */
class TerminalWidget : public QWidget {
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget();

    // 终端控制
    void clear();
    void reset();
    void setPrompt(const QString& prompt);
    QString getPrompt() const;
    
    // 输出控制
    void appendOutput(const QString& text, const QString& color = "white");
    void appendError(const QString& text);
    void appendSuccess(const QString& text);
    void appendWarning(const QString& text);
    void appendInfo(const QString& text);
    
    // 命令相关
    void setCommandCompleter(const QStringList& commands);
    void addToHistory(const QString& command);
    QStringList getCommandHistory() const;
    
    // 状态控制
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void setCurrentDirectory(const QString& dir);
    QString getCurrentDirectory() const;
    
    // 环境信息
    void setEnvironmentInfo(const QString& info);
    void showWelcomeMessage();
    void showBanner();

signals:
    void commandEntered(const QString& command);
    void commandCompleted(const QString& command, int exitCode);
    void terminalCleared();

public slots:
    void executeCommand(const QString& command);
    void onCommandFinished(const QString& output, int exitCode);
    void focusCommandInput();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCommandInputReturnPressed();
    void onCommandInputTextChanged();
    void updatePrompt();
    void blinkCursor();

private:
    void setupUI();
    void setupCompleter();
    void setupSyntaxHighlighting();
    void navigateHistory(int direction); // -1 for up, 1 for down
    void insertPrompt();
    void scrollToBottom();
    QString formatOutput(const QString& text, const QString& color) const;
    QString getCurrentTimeStamp() const;
    
    // UI组件
    QVBoxLayout* m_mainLayout;
    QTextEdit* m_outputArea;
    QWidget* m_inputWidget;
    QHBoxLayout* m_inputLayout;
    QLabel* m_promptLabel;
    QLineEdit* m_commandInput;
    
    // 功能组件
    QCompleter* m_completer;
    QStringListModel* m_completerModel;
    QTimer* m_cursorTimer;
    
    // 状态变量
    QString m_prompt;
    QString m_currentDirectory;
    QString m_environmentInfo;
    QStringList m_commandHistory;
    int m_historyIndex;
    bool m_readOnly;
    bool m_cursorVisible;
    
    // 配置常量
    static const QString DEFAULT_PROMPT;
    static const QString WELCOME_MESSAGE;
    static const QString BANNER;
    static const int MAX_HISTORY_SIZE;
};
