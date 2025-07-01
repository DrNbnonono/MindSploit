#pragma once
#include <QMainWindow>

class QTextEdit;
class QPushButton;
class QSplitter;
class TerminalWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTerminalCommand(const QString& command);
    void onShowHistory();
    void onShowProjects();
    void onShowBanner();

private:
    void setupUI();
    void setupMenuBar();
    void initializeDataStorage();
    void saveWindowState();
    void restoreWindowState();
    void executeBuiltinCommand(const QString& command);
    void executePentestCommand(const QString& command);
    void updateVisualArea(const QString& content);
    void updateModuleResults(const QString& moduleName, const QJsonObject& results);
    QString formatNmapResults(const QJsonObject& results);
    QString formatPortScanResults(const QJsonObject& results);
    QString formatSearchsploitResults(const QJsonObject& results);
    QString formatCurlResults(const QJsonObject& results);

    // UI组件
    QSplitter* m_mainSplitter;
    QTextEdit* m_visualArea; // 可视化区
    TerminalWidget* m_terminal; // 专业终端组件

    // 菜单和工具栏
    QPushButton* m_historyButton;
    QPushButton* m_projectsButton;
    QPushButton* m_bannerButton;
};
