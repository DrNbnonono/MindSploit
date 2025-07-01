#include "nmap_module.h"
#include "../../core/config_manager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

// 静态常量定义
const QStringList NmapModule::SCAN_TYPES = {
    "-sS",  // TCP SYN扫描
    "-sT",  // TCP连接扫描
    "-sU",  // UDP扫描
    "-sA",  // TCP ACK扫描
    "-sW",  // TCP窗口扫描
    "-sM",  // TCP Maimon扫描
    "-sN",  // TCP Null扫描
    "-sF",  // TCP FIN扫描
    "-sX"   // TCP Xmas扫描
};

const QStringList NmapModule::OUTPUT_FORMATS = {
    "normal", "xml", "grepable", "json"
};

const QStringList NmapModule::TIMING_TEMPLATES = {
    "paranoid", "sneaky", "polite", "normal", "aggressive", "insane"
};

NmapModule::NmapModule(QObject *parent)
    : ModuleInterface(parent)
    , m_process(nullptr)
    , m_progressTimer(new QTimer(this))
    , m_status(READY)
    , m_progress(0)
{
    connect(m_progressTimer, &QTimer::timeout, this, &NmapModule::onProgressTimeout);
}

NmapModule::~NmapModule() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

QString NmapModule::getVersion() const {
    QString nmapPath = getToolPath("nmap");
    if (nmapPath.isEmpty()) {
        return "Not installed";
    }
    
    QProcess process;
    process.start(nmapPath, QStringList() << "--version");
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardOutput();
    QRegularExpression re(R"(Nmap version (\d+\.\d+))");
    QRegularExpressionMatch match = re.match(output);
    
    if (match.hasMatch()) {
        return match.captured(1);
    }
    
    return "Unknown";
}

QJsonObject NmapModule::getDefaultOptions() const {
    QJsonObject options;
    options["target"] = "";
    options["scan_type"] = "-sS";  // TCP SYN扫描
    options["ports"] = "";         // 空表示默认端口
    options["timing"] = "3";       // 正常时序
    options["service_detection"] = false;
    options["os_detection"] = false;
    options["script_scan"] = false;
    options["output_format"] = "normal";
    options["max_retries"] = 3;
    options["host_timeout"] = 300;
    
    return options;
}

bool NmapModule::setOption(const QString& key, const QVariant& value) {
    m_options[key] = QJsonValue::fromVariant(value);
    return true;
}

QVariant NmapModule::getOption(const QString& key) const {
    return m_options[key].toVariant();
}

bool NmapModule::validateOptions() const {
    // 检查必需的选项
    if (!m_options.contains("target") || m_options["target"].toString().isEmpty()) {
        return false;
    }
    
    // 验证扫描类型
    QString scanType = m_options["scan_type"].toString();
    if (!SCAN_TYPES.contains(scanType)) {
        return false;
    }
    
    return true;
}

bool NmapModule::isAvailable() const {
    return checkDependency("nmap");
}

bool NmapModule::canExecute() const {
    return isAvailable() && validateOptions() && m_status == READY;
}

void NmapModule::execute() {
    if (!canExecute()) {
        emit executionFailed("Cannot execute: module not ready or invalid options");
        return;
    }
    
    resetState();
    m_status = RUNNING;
    emit statusChanged(m_status);
    
    QString command = buildCommand();
    QStringList args = command.split(' ', Qt::SkipEmptyParts);
    QString program = args.takeFirst();
    
    m_process = createProcess();
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NmapModule::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &NmapModule::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &NmapModule::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &NmapModule::onReadyReadStandardError);
    
    m_progressTimer->start(1000); // 每秒更新进度
    
    qDebug() << "Starting nmap:" << program << args;
    m_process->start(program, args);
    
    if (!m_process->waitForStarted(5000)) {
        m_status = FAILED;
        m_lastError = "Failed to start nmap process";
        emit executionFailed(m_lastError);
        return;
    }
    
    emit outputReceived("Nmap scan started...");
}

void NmapModule::stop() {
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(5000)) {
            m_process->kill();
        }
    }
    
    m_progressTimer->stop();
    m_status = STOPPED;
    emit statusChanged(m_status);
}

QStringList NmapModule::getSupportedTargets() const {
    return QStringList() << "IPv4 addresses" << "IPv6 addresses" << "Hostnames" << "CIDR ranges";
}

QString NmapModule::getUsage() const {
    return "nmap [scan_type] [options] <target>\n"
           "Example: nmap -sS -p 1-1000 192.168.1.1";
}

QStringList NmapModule::getExamples() const {
    return QStringList()
        << "nmap 192.168.1.1"
        << "nmap -sS -p 1-1000 192.168.1.0/24"
        << "nmap -sV -O target.com"
        << "nmap -A -T4 192.168.1.1-254";
}

QString NmapModule::getHelp() const {
    return "Nmap (Network Mapper) is a network discovery and security auditing tool.\n\n"
           "Common scan types:\n"
           "  -sS: TCP SYN scan (default)\n"
           "  -sT: TCP connect scan\n"
           "  -sU: UDP scan\n"
           "  -sA: TCP ACK scan\n\n"
           "Common options:\n"
           "  -p <ports>: Specify ports to scan\n"
           "  -sV: Service version detection\n"
           "  -O: OS detection\n"
           "  -A: Aggressive scan (OS, version, script, traceroute)\n"
           "  -T<0-5>: Timing template (0=paranoid, 5=insane)";
}

void NmapModule::setTarget(const QString& target) {
    setOption("target", target);
}

void NmapModule::quickScan(const QString& target) {
    resetState();
    setOption("target", target);
    setOption("scan_type", "-sS");
    setOption("ports", "");
    setOption("timing", "4");
    execute();
}

QString NmapModule::buildCommand() const {
    QString nmapPath = getToolPath("nmap");
    QStringList args;
    
    // 扫描类型
    args << m_options["scan_type"].toString();
    
    // 端口范围
    QString ports = m_options["ports"].toString();
    if (!ports.isEmpty()) {
        args << "-p" << ports;
    }
    
    // 时序模板
    args << "-T" + m_options["timing"].toString();
    
    // 服务检测
    if (m_options["service_detection"].toBool()) {
        args << "-sV";
    }
    
    // 操作系统检测
    if (m_options["os_detection"].toBool()) {
        args << "-O";
    }
    
    // 脚本扫描
    if (m_options["script_scan"].toBool()) {
        args << "-sC";
    }
    
    // 目标
    args << m_options["target"].toString();
    
    return nmapPath + " " + args.join(" ");
}

void NmapModule::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_progressTimer->stop();
    
    if (exitStatus == QProcess::CrashExit) {
        m_status = FAILED;
        m_lastError = "Nmap process crashed";
        emit executionFailed(m_lastError);
        return;
    }
    
    if (exitCode == 0) {
        m_status = COMPLETED;
        parseNmapOutput(m_outputBuffer);
        emit executionCompleted(m_results);
    } else {
        m_status = FAILED;
        m_lastError = QString("Nmap exited with code %1: %2").arg(exitCode).arg(m_errorBuffer);
        emit executionFailed(m_lastError);
    }
    
    emit statusChanged(m_status);
}

void NmapModule::onProcessError(QProcess::ProcessError error) {
    m_progressTimer->stop();
    m_status = FAILED;
    
    switch (error) {
    case QProcess::FailedToStart:
        m_lastError = "Failed to start nmap process";
        break;
    case QProcess::Crashed:
        m_lastError = "Nmap process crashed";
        break;
    case QProcess::Timedout:
        m_lastError = "Nmap process timed out";
        break;
    default:
        m_lastError = "Unknown nmap process error";
        break;
    }
    
    emit executionFailed(m_lastError);
    emit statusChanged(m_status);
}

void NmapModule::onReadyReadStandardOutput() {
    if (m_process) {
        QByteArray data = m_process->readAllStandardOutput();
        QString output = QString::fromUtf8(data);
        m_outputBuffer += output;
        emit outputReceived(output);
    }
}

void NmapModule::onReadyReadStandardError() {
    if (m_process) {
        QByteArray data = m_process->readAllStandardError();
        QString error = QString::fromUtf8(data);
        m_errorBuffer += error;
        emit errorReceived(error);
    }
}

void NmapModule::onProgressTimeout() {
    // 简单的进度估算
    m_progress = qMin(m_progress + 5, 95);
    emit progressUpdated(m_progress);
}

void NmapModule::parseNmapOutput(const QString& output) {
    // 简化的输出解析
    QJsonObject results;
    QJsonArray hosts;
    
    // 解析主机信息
    QRegularExpression hostRe(R"(Nmap scan report for (.+))");
    QRegularExpression portRe(R"((\d+)/(tcp|udp)\s+(open|closed|filtered)\s*(.*)?)");
    
    QStringList lines = output.split('\n');
    QJsonObject currentHost;
    QJsonArray currentPorts;
    
    for (const QString& line : lines) {
        QRegularExpressionMatch hostMatch = hostRe.match(line);
        if (hostMatch.hasMatch()) {
            // 保存前一个主机
            if (!currentHost.isEmpty()) {
                currentHost["ports"] = currentPorts;
                hosts.append(currentHost);
            }
            
            // 开始新主机
            currentHost = QJsonObject();
            currentHost["host"] = hostMatch.captured(1);
            currentPorts = QJsonArray();
            continue;
        }
        
        QRegularExpressionMatch portMatch = portRe.match(line);
        if (portMatch.hasMatch()) {
            QJsonObject port;
            port["port"] = portMatch.captured(1).toInt();
            port["protocol"] = portMatch.captured(2);
            port["state"] = portMatch.captured(3);
            port["service"] = portMatch.captured(4).trimmed();
            currentPorts.append(port);
        }
    }
    
    // 保存最后一个主机
    if (!currentHost.isEmpty()) {
        currentHost["ports"] = currentPorts;
        hosts.append(currentHost);
    }
    
    results["hosts"] = hosts;
    results["scan_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    results["command"] = buildCommand();
    
    m_results = results;
}

void NmapModule::resetState() {
    m_status = READY;
    m_lastError.clear();
    m_results = QJsonObject();
    m_outputBuffer.clear();
    m_errorBuffer.clear();
    m_progress = 0;
}
