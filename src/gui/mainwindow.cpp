#include "mainwindow.h"
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    visualArea = new QTextEdit(this);
    visualArea->setReadOnly(true);
    visualArea->setText("Hello Visual Area!");

    terminalArea = new QTextEdit(this);
    terminalArea->setReadOnly(true);
    terminalArea->setText("Hello Terminal Area!");

    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(visualArea);
    splitter->addWidget(terminalArea);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);
    setWindowTitle("MindSploit");
    resize(800, 600);
}

MainWindow::~MainWindow() {} 