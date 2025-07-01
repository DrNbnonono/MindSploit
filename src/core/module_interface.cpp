#include "module_interface.h"
#include "config_manager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QString ModuleInterface::formatCommand(const QStringList& args) const {
    return args.join(" ");
}

bool ModuleInterface::checkDependency(const QString& tool) const {
    QString toolPath = getToolPath(tool);
    if (toolPath.isEmpty()) {
        return false;
    }
    
    QFileInfo fileInfo(toolPath);
    return fileInfo.exists() && fileInfo.isExecutable();
}

QString ModuleInterface::getToolPath(const QString& tool) const {
    // 首先检查配置中的路径
    QString configPath = ConfigManager::instance().getToolPath(tool);
    if (!configPath.isEmpty() && QFileInfo(configPath).exists()) {
        return configPath;
    }
    
    // 检查系统PATH中的工具
    QStringList paths = QString(qgetenv("PATH")).split(
#ifdef Q_OS_WIN
        ";"
#else
        ":"
#endif
    );
    
    QStringList extensions;
#ifdef Q_OS_WIN
    extensions << ".exe" << ".bat" << ".cmd";
#endif
    
    for (const QString& path : paths) {
        QDir dir(path);
        if (!dir.exists()) continue;
        
        // 检查不带扩展名的文件
        QString fullPath = dir.absoluteFilePath(tool);
        if (QFileInfo(fullPath).exists()) {
            return fullPath;
        }
        
        // 检查带扩展名的文件
        for (const QString& ext : extensions) {
            QString fullPathWithExt = dir.absoluteFilePath(tool + ext);
            if (QFileInfo(fullPathWithExt).exists()) {
                return fullPathWithExt;
            }
        }
    }
    
    return QString();
}

QProcess* ModuleInterface::createProcess() {
    QProcess* process = new QProcess(this);
    
    // 设置工作目录
    process->setWorkingDirectory(QDir::currentPath());
    
    // 连接信号
    connect(process, &QProcess::readyReadStandardOutput, [this, process]() {
        QByteArray data = process->readAllStandardOutput();
        emit outputReceived(QString::fromUtf8(data));
    });
    
    connect(process, &QProcess::readyReadStandardError, [this, process]() {
        QByteArray data = process->readAllStandardError();
        emit errorReceived(QString::fromUtf8(data));
    });
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode == 0) {
            emit executionCompleted(getResults());
        } else {
            emit executionFailed(QString("Process exited with code %1").arg(exitCode));
        }
    });
    
    return process;
}
