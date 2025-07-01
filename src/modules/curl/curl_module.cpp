#include "curl_module.h"
#include <QDebug>

const QStringList CurlModule::HTTP_METHODS = {
    "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH"
};

CurlModule::CurlModule(QObject *parent)
    : ModuleInterface(parent)
    , m_process(nullptr)
    , m_status(READY)
{
}

CurlModule::~CurlModule() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

QString CurlModule::getVersion() const {
    QString curlPath = getToolPath("curl");
    if (curlPath.isEmpty()) {
        return "Not installed";
    }
    
    QProcess process;
    process.start(curlPath, QStringList() << "--version");
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    if (!lines.isEmpty()) {
        QString firstLine = lines.first();
        QStringList parts = firstLine.split(' ');
        if (parts.size() >= 2) {
            return parts[1];
        }
    }
    
    return "Unknown";
}

QJsonObject CurlModule::getDefaultOptions() const {
    QJsonObject options;
    options["url"] = "";
    options["method"] = "GET";
    options["headers"] = QJsonArray();
    options["data"] = "";
    options["user_agent"] = "MindSploit/1.0";
    options["follow_redirects"] = true;
    options["insecure"] = false;
    options["timeout"] = 30;
    options["proxy"] = "";
    options["output_file"] = "";
    options["include_headers"] = true;
    options["verbose"] = false;
    
    return options;
}

bool CurlModule::setOption(const QString& key, const QVariant& value) {
    m_options[key] = QJsonValue::fromVariant(value);
    return true;
}

QVariant CurlModule::getOption(const QString& key) const {
    return m_options[key].toVariant();
}

bool CurlModule::validateOptions() const {
    if (!m_options.contains("url") || m_options["url"].toString().isEmpty()) {
        return false;
    }
    
    QString method = m_options["method"].toString();
    if (!HTTP_METHODS.contains(method.toUpper())) {
        return false;
    }
    
    return true;
}

bool CurlModule::isAvailable() const {
    return checkDependency("curl");
}

bool CurlModule::canExecute() const {
    return isAvailable() && validateOptions() && m_status == READY;
}

void CurlModule::execute() {
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
            this, &CurlModule::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &CurlModule::onProcessError);
    
    qDebug() << "Starting curl:" << program << args;
    m_process->start(program, args);
    
    if (!m_process->waitForStarted(5000)) {
        m_status = FAILED;
        m_lastError = "Failed to start curl process";
        emit executionFailed(m_lastError);
        return;
    }
    
    emit outputReceived("Curl request started...");
}

void CurlModule::stop() {
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(5000)) {
            m_process->kill();
        }
    }
    
    m_status = STOPPED;
    emit statusChanged(m_status);
}

QStringList CurlModule::getSupportedTargets() const {
    return QStringList() << "HTTP URLs" << "HTTPS URLs" << "FTP URLs" << "File URLs";
}

QString CurlModule::getUsage() const {
    return "curl [options] <url>\n"
           "Example: curl -X POST -d \"data\" https://example.com/api";
}

QStringList CurlModule::getExamples() const {
    return QStringList()
        << "curl https://example.com"
        << "curl -X POST -d \"username=admin&password=123\" https://example.com/login"
        << "curl -H \"Authorization: Bearer token\" https://api.example.com/data"
        << "curl -o output.html https://example.com";
}

QString CurlModule::getHelp() const {
    return "Curl is a command line tool for transferring data with URLs.\n\n"
           "Common options:\n"
           "  -X: HTTP method (GET, POST, PUT, DELETE, etc.)\n"
           "  -H: Add custom header\n"
           "  -d: Send data in POST request\n"
           "  -o: Write output to file\n"
           "  -L: Follow redirects\n"
           "  -k: Allow insecure SSL connections\n"
           "  -v: Verbose output\n"
           "  --proxy: Use proxy server";
}

void CurlModule::setUrl(const QString& url) {
    setOption("url", url);
}

void CurlModule::setMethod(const QString& method) {
    setOption("method", method.toUpper());
}

void CurlModule::simpleGet(const QString& url) {
    resetState();
    setOption("url", url);
    setOption("method", "GET");
    execute();
}

void CurlModule::postData(const QString& url, const QString& data) {
    resetState();
    setOption("url", url);
    setOption("method", "POST");
    setOption("data", data);
    execute();
}

QString CurlModule::buildCommand() const {
    QString curlPath = getToolPath("curl");
    QStringList args;
    
    // HTTP方法
    QString method = m_options["method"].toString();
    if (method != "GET") {
        args << "-X" << method;
    }
    
    // 请求头
    QJsonArray headers = m_options["headers"].toArray();
    for (const QJsonValue& header : headers) {
        args << "-H" << header.toString();
    }
    
    // User-Agent
    QString userAgent = m_options["user_agent"].toString();
    if (!userAgent.isEmpty()) {
        args << "-A" << userAgent;
    }
    
    // 数据
    QString data = m_options["data"].toString();
    if (!data.isEmpty()) {
        args << "-d" << data;
    }
    
    // 跟随重定向
    if (m_options["follow_redirects"].toBool()) {
        args << "-L";
    }
    
    // 忽略SSL错误
    if (m_options["insecure"].toBool()) {
        args << "-k";
    }
    
    // 超时
    int timeout = m_options["timeout"].toInt();
    if (timeout > 0) {
        args << "--max-time" << QString::number(timeout);
    }
    
    // 代理
    QString proxy = m_options["proxy"].toString();
    if (!proxy.isEmpty()) {
        args << "--proxy" << proxy;
    }
    
    // 包含响应头
    if (m_options["include_headers"].toBool()) {
        args << "-i";
    }
    
    // 详细输出
    if (m_options["verbose"].toBool()) {
        args << "-v";
    }
    
    // 输出文件
    QString outputFile = m_options["output_file"].toString();
    if (!outputFile.isEmpty()) {
        args << "-o" << outputFile;
    }
    
    // URL
    args << m_options["url"].toString();
    
    return curlPath + " " + args.join(" ");
}

void CurlModule::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::CrashExit) {
        m_status = FAILED;
        m_lastError = "Curl process crashed";
        emit executionFailed(m_lastError);
        return;
    }
    
    if (exitCode == 0) {
        m_status = COMPLETED;
        QString output = m_process->readAllStandardOutput();
        parseResponse(output);
        emit executionCompleted(m_results);
    } else {
        m_status = FAILED;
        m_lastError = QString("Curl exited with code %1").arg(exitCode);
        emit executionFailed(m_lastError);
    }
    
    emit statusChanged(m_status);
}

void CurlModule::onProcessError(QProcess::ProcessError error) {
    m_status = FAILED;
    m_lastError = "Curl process error";
    emit executionFailed(m_lastError);
    emit statusChanged(m_status);
}

void CurlModule::parseResponse(const QString& output) {
    QJsonObject results;
    
    // 简单的响应解析
    results["response"] = output;
    results["url"] = m_options["url"].toString();
    results["method"] = m_options["method"].toString();
    results["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 尝试提取HTTP状态码
    QRegularExpression statusRe(R"(HTTP/[\d\.]+\s+(\d+))");
    QRegularExpressionMatch match = statusRe.match(output);
    if (match.hasMatch()) {
        results["status_code"] = match.captured(1).toInt();
    }
    
    m_results = results;
}

void CurlModule::resetState() {
    m_status = READY;
    m_lastError.clear();
    m_results = QJsonObject();
}
