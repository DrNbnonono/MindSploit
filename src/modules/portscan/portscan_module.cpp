#include "portscan_module.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QThread>
#include <QDebug>

// 服务映射表
const QHash<int, QString> PortScanModule::SERVICE_MAP = {
    {21, "ftp"}, {22, "ssh"}, {23, "telnet"}, {25, "smtp"},
    {53, "dns"}, {80, "http"}, {110, "pop3"}, {143, "imap"},
    {443, "https"}, {993, "imaps"}, {995, "pop3s"}, {3389, "rdp"},
    {3306, "mysql"}, {5432, "postgresql"}, {6379, "redis"}, {27017, "mongodb"}
};

PortScanModule::PortScanModule(QObject *parent)
    : ModuleInterface(parent)
    , m_scanThread(nullptr)
    , m_scanWorker(nullptr)
    , m_status(READY)
    , m_stopRequested(false)
    , m_completedPorts(0)
    , m_totalPorts(0)
{
}

PortScanModule::~PortScanModule() {
    stop();
}

QJsonObject PortScanModule::getDefaultOptions() const {
    QJsonObject options;
    options["target"] = "";
    options["start_port"] = 1;
    options["end_port"] = 1000;
    options["protocol"] = "tcp";
    options["threads"] = 50;
    options["timeout"] = 3000;
    options["delay"] = 0;
    return options;
}

bool PortScanModule::setOption(const QString& key, const QVariant& value) {
    m_options[key] = QJsonValue::fromVariant(value);
    return true;
}

QVariant PortScanModule::getOption(const QString& key) const {
    return m_options[key].toVariant();
}

bool PortScanModule::validateOptions() const {
    if (!m_options.contains("target") || m_options["target"].toString().isEmpty()) {
        return false;
    }
    
    int startPort = m_options["start_port"].toInt();
    int endPort = m_options["end_port"].toInt();
    
    if (startPort < 1 || endPort > 65535 || startPort > endPort) {
        return false;
    }
    
    return true;
}

bool PortScanModule::canExecute() const {
    return validateOptions() && m_status == READY;
}

void PortScanModule::execute() {
    if (!canExecute()) {
        emit executionFailed("Cannot execute: invalid options or module not ready");
        return;
    }
    
    resetState();
    startScan();
}

void PortScanModule::stop() {
    m_stopRequested = true;
    
    if (m_scanWorker) {
        m_scanWorker->stopScan();
    }
    
    if (m_scanThread && m_scanThread->isRunning()) {
        m_scanThread->quit();
        m_scanThread->wait(5000);
    }
    
    m_status = STOPPED;
    emit statusChanged(m_status);
}

QStringList PortScanModule::getSupportedTargets() const {
    return QStringList() << "IPv4 addresses" << "Hostnames";
}

QString PortScanModule::getUsage() const {
    return "portscan <target> [start_port] [end_port]\n"
           "Example: portscan 192.168.1.1 1 1000";
}

QStringList PortScanModule::getExamples() const {
    return QStringList()
        << "portscan 192.168.1.1"
        << "portscan example.com 80 443"
        << "portscan 10.0.0.1 1 65535";
}

QString PortScanModule::getHelp() const {
    return "Built-in TCP/UDP port scanner.\n\n"
           "Features:\n"
           "  - TCP connect scan\n"
           "  - UDP scan (basic)\n"
           "  - Multi-threaded scanning\n"
           "  - Service identification\n"
           "  - Configurable timeout and delay";
}

void PortScanModule::quickScan(const QString& target) {
    resetState();
    setOption("target", target);
    setOption("start_port", 1);
    setOption("end_port", 1000);
    execute();
}

void PortScanModule::startScan() {
    m_status = RUNNING;
    emit statusChanged(m_status);
    
    QString target = m_options["target"].toString();
    int startPort = m_options["start_port"].toInt();
    int endPort = m_options["end_port"].toInt();
    QString protocol = m_options["protocol"].toString();
    int timeout = m_options["timeout"].toInt();
    int delay = m_options["delay"].toInt();
    
    QList<int> ports;
    for (int port = startPort; port <= endPort; ++port) {
        ports.append(port);
    }
    
    m_totalPorts = ports.size();
    m_completedPorts = 0;
    
    // 创建工作线程
    m_scanThread = new QThread(this);
    m_scanWorker = new ScanWorker(target, ports, protocol, timeout, delay);
    m_scanWorker->moveToThread(m_scanThread);
    
    // 连接信号
    connect(m_scanThread, &QThread::started, m_scanWorker, &ScanWorker::startScan);
    connect(m_scanWorker, &ScanWorker::progress, this, &PortScanModule::onScanProgress);
    connect(m_scanWorker, &ScanWorker::portResult, this, &PortScanModule::onPortResult);
    connect(m_scanWorker, &ScanWorker::scanCompleted, this, &PortScanModule::onScanCompleted);
    connect(m_scanWorker, &ScanWorker::scanError, this, &PortScanModule::onScanError);
    
    m_scanThread->start();
    
    emit outputReceived(QString("Starting port scan of %1 ports on %2...").arg(m_totalPorts).arg(target));
}

void PortScanModule::onScanProgress(int completedPorts, int totalPorts) {
    m_completedPorts = completedPorts;
    int percentage = (completedPorts * 100) / totalPorts;
    emit progressUpdated(percentage);
    
    if (completedPorts % 100 == 0) {
        emit outputReceived(QString("Progress: %1/%2 ports scanned (%3%)")
                           .arg(completedPorts).arg(totalPorts).arg(percentage));
    }
}

void PortScanModule::onPortResult(const QString& host, int port, const QString& protocol, bool isOpen, const QString& service) {
    if (isOpen) {
        QString message = QString("Port %1/%2 is OPEN").arg(port).arg(protocol);
        if (!service.isEmpty()) {
            message += QString(" (%1)").arg(service);
        }
        emit outputReceived(message);
        
        // 添加到结果中
        QMutexLocker locker(&m_resultsMutex);
        QJsonArray openPorts = m_results["open_ports"].toArray();
        QJsonObject portInfo;
        portInfo["port"] = port;
        portInfo["protocol"] = protocol;
        portInfo["service"] = service;
        openPorts.append(portInfo);
        m_results["open_ports"] = openPorts;
    }
}

void PortScanModule::onScanCompleted() {
    m_status = COMPLETED;
    
    // 完善结果信息
    m_results["target"] = m_options["target"].toString();
    m_results["total_ports"] = m_totalPorts;
    m_results["open_count"] = m_results["open_ports"].toArray().size();
    m_results["scan_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit outputReceived(QString("Scan completed. Found %1 open ports out of %2 scanned.")
                       .arg(m_results["open_count"].toInt()).arg(m_totalPorts));
    
    emit executionCompleted(m_results);
    emit statusChanged(m_status);
    
    // 清理线程
    if (m_scanThread) {
        m_scanThread->quit();
        m_scanThread->wait();
        m_scanThread->deleteLater();
        m_scanThread = nullptr;
    }
    
    if (m_scanWorker) {
        m_scanWorker->deleteLater();
        m_scanWorker = nullptr;
    }
}

void PortScanModule::onScanError(const QString& error) {
    m_status = FAILED;
    m_lastError = error;
    emit executionFailed(error);
    emit statusChanged(m_status);
}

void PortScanModule::resetState() {
    m_status = READY;
    m_lastError.clear();
    m_results = QJsonObject();
    m_results["open_ports"] = QJsonArray();
    m_stopRequested = false;
    m_completedPorts = 0;
    m_totalPorts = 0;
}

// ScanWorker实现
PortScanModule::ScanWorker::ScanWorker(const QString& target, const QList<int>& ports, 
                                      const QString& protocol, int timeout, int delay, QObject* parent)
    : QObject(parent)
    , m_target(target)
    , m_ports(ports)
    , m_protocol(protocol)
    , m_timeout(timeout)
    , m_delay(delay)
    , m_stopRequested(false)
{
}

void PortScanModule::ScanWorker::startScan() {
    int completed = 0;
    int total = m_ports.size();
    
    for (int port : m_ports) {
        if (m_stopRequested) {
            break;
        }
        
        bool isOpen = false;
        QString service;
        
        if (m_protocol == "tcp" || m_protocol == "both") {
            isOpen = scanTcpPort(m_target, port, m_timeout);
            if (isOpen) {
                service = identifyService(port, "tcp");
                emit portResult(m_target, port, "tcp", true, service);
            }
        }
        
        completed++;
        emit progress(completed, total);
        
        if (m_delay > 0) {
            QThread::msleep(m_delay);
        }
    }
    
    emit scanCompleted();
}

void PortScanModule::ScanWorker::stopScan() {
    m_stopRequested = true;
}

bool PortScanModule::ScanWorker::scanTcpPort(const QString& host, int port, int timeout) {
    QTcpSocket socket;
    socket.connectToHost(host, port);
    
    if (socket.waitForConnected(timeout)) {
        socket.disconnectFromHost();
        return true;
    }
    
    return false;
}

QString PortScanModule::ScanWorker::identifyService(int port, const QString& protocol) {
    Q_UNUSED(protocol)
    return PortScanModule::SERVICE_MAP.value(port, "unknown");
}
