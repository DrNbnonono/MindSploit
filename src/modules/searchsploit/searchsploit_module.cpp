#include "searchsploit_module.h"
#include <QDebug>

SearchsploitModule::SearchsploitModule(QObject *parent)
    : ModuleInterface(parent)
    , m_process(nullptr)
    , m_status(READY)
{
}

SearchsploitModule::~SearchsploitModule() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

QString SearchsploitModule::getVersion() const {
    QString searchsploitPath = getToolPath("searchsploit");
    if (searchsploitPath.isEmpty()) {
        return "Not installed";
    }
    return "Latest";
}

QJsonObject SearchsploitModule::getDefaultOptions() const {
    QJsonObject options;
    options["query"] = "";
    options["exact"] = false;
    options["case_sensitive"] = false;
    options["platform"] = "";
    options["type"] = "";
    options["max_results"] = 50;
    return options;
}

bool SearchsploitModule::setOption(const QString& key, const QVariant& value) {
    m_options[key] = QJsonValue::fromVariant(value);
    return true;
}

QVariant SearchsploitModule::getOption(const QString& key) const {
    return m_options[key].toVariant();
}

bool SearchsploitModule::validateOptions() const {
    return m_options.contains("query") && !m_options["query"].toString().isEmpty();
}

bool SearchsploitModule::isAvailable() const {
    return checkDependency("searchsploit");
}

bool SearchsploitModule::canExecute() const {
    return isAvailable() && validateOptions() && m_status == READY;
}

void SearchsploitModule::execute() {
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
            this, &SearchsploitModule::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &SearchsploitModule::onProcessError);
    
    qDebug() << "Starting searchsploit:" << program << args;
    m_process->start(program, args);
    
    if (!m_process->waitForStarted(5000)) {
        m_status = FAILED;
        m_lastError = "Failed to start searchsploit process";
        emit executionFailed(m_lastError);
        return;
    }
    
    emit outputReceived("Searchsploit search started...");
}

void SearchsploitModule::stop() {
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(5000)) {
            m_process->kill();
        }
    }
    
    m_status = STOPPED;
    emit statusChanged(m_status);
}

QStringList SearchsploitModule::getSupportedTargets() const {
    return QStringList() << "Software names" << "CVE numbers" << "Keywords" << "Platform names";
}

QString SearchsploitModule::getUsage() const {
    return "searchsploit [options] <query>\n"
           "Example: searchsploit apache 2.4";
}

QStringList SearchsploitModule::getExamples() const {
    return QStringList()
        << "searchsploit apache"
        << "searchsploit CVE-2021-44228"
        << "searchsploit --exact \"Apache 2.4.49\""
        << "searchsploit --platform linux kernel";
}

QString SearchsploitModule::getHelp() const {
    return "Searchsploit is a command line search tool for Exploit Database.\n\n"
           "Options:\n"
           "  --exact: Exact match\n"
           "  --case: Case sensitive search\n"
           "  --platform: Filter by platform\n"
           "  --type: Filter by exploit type";
}

void SearchsploitModule::searchByKeyword(const QString& keyword) {
    setOption("query", keyword);
    execute();
}

void SearchsploitModule::searchByCVE(const QString& cve) {
    setOption("query", cve);
    execute();
}

QString SearchsploitModule::buildCommand() const {
    QString searchsploitPath = getToolPath("searchsploit");
    QStringList args;
    
    if (m_options["exact"].toBool()) {
        args << "--exact";
    }
    
    if (m_options["case_sensitive"].toBool()) {
        args << "--case";
    }
    
    QString platform = m_options["platform"].toString();
    if (!platform.isEmpty()) {
        args << "--platform" << platform;
    }
    
    args << m_options["query"].toString();
    
    return searchsploitPath + " " + args.join(" ");
}

void SearchsploitModule::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::CrashExit) {
        m_status = FAILED;
        m_lastError = "Searchsploit process crashed";
        emit executionFailed(m_lastError);
        return;
    }
    
    if (exitCode == 0) {
        m_status = COMPLETED;
        QString output = m_process->readAllStandardOutput();
        parseSearchResults(output);
        emit executionCompleted(m_results);
    } else {
        m_status = FAILED;
        m_lastError = QString("Searchsploit exited with code %1").arg(exitCode);
        emit executionFailed(m_lastError);
    }
    
    emit statusChanged(m_status);
}

void SearchsploitModule::onProcessError(QProcess::ProcessError error) {
    m_status = FAILED;
    m_lastError = "Searchsploit process error";
    emit executionFailed(m_lastError);
    emit statusChanged(m_status);
}

void SearchsploitModule::parseSearchResults(const QString& output) {
    QJsonObject results;
    QJsonArray exploits;

    // Searchsploit输出格式分析
    // 标准格式：Title | Path | Date
    // 示例：Apache 2.4.49 - Path Traversal | exploits/linux/remote/50383.py | 2021-10-05

    QStringList lines = output.split('\n');
    bool inResultsSection = false;

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();

        // 跳过头部信息，找到结果开始标记
        if (trimmedLine.contains("Exploit Title") || trimmedLine.contains("Path")) {
            inResultsSection = true;
            continue;
        }

        // 跳过分隔线
        if (trimmedLine.startsWith('-') || trimmedLine.isEmpty()) {
            continue;
        }

        // 解析结果行
        if (inResultsSection && trimmedLine.contains('|')) {
            QStringList parts = trimmedLine.split('|');
            if (parts.size() >= 2) {
                QJsonObject exploit;
                exploit["title"] = parts[0].trimmed();
                exploit["path"] = parts[1].trimmed();

                // 提取额外信息
                if (parts.size() >= 3) {
                    exploit["date"] = parts[2].trimmed();
                }

                // 解析CVE信息（如果存在）
                QString title = parts[0].trimmed();
                QRegularExpression cveRegex(R"(CVE-(\d{4}-\d{4,}))");
                QRegularExpressionMatch cveMatch = cveRegex.match(title);
                if (cveMatch.hasMatch()) {
                    exploit["cve"] = cveMatch.captured(0);
                }

                // 解析平台信息
                QString path = parts[1].trimmed();
                QStringList pathParts = path.split('/');
                if (pathParts.size() >= 3) {
                    exploit["platform"] = pathParts[1]; // linux/windows/multiple等
                    exploit["type"] = pathParts[2];     // remote/local/webapps等
                }

                // 提取EDB-ID
                QRegularExpression edbRegex(R"((\d+)\.(py|txt|c|rb|pl|sh)$)");
                QRegularExpressionMatch edbMatch = edbRegex.match(path);
                if (edbMatch.hasMatch()) {
                    exploit["edb_id"] = edbMatch.captured(1);
                    exploit["file_type"] = edbMatch.captured(2);
                }

                exploits.append(exploit);
            }
        }
    }

    // 解析统计信息
    QRegularExpression statsRegex(R"((\d+) exploit\(s\) found)");
    QRegularExpressionMatch statsMatch = statsRegex.match(output);
    int foundCount = 0;
    if (statsMatch.hasMatch()) {
        foundCount = statsMatch.captured(1).toInt();
    }

    results["exploits"] = exploits;
    results["count"] = exploits.size();
    results["total_found"] = foundCount;
    results["query"] = m_options["query"].toString();
    results["search_time"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_results = results;
}

void SearchsploitModule::resetState() {
    m_status = READY;
    m_lastError.clear();
    m_results = QJsonObject();
}
