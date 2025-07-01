#pragma once
#include <QMainWindow>

class QTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    QTextEdit* visualArea; // 可视化区
    QTextEdit* terminalArea; // 终端区
};
