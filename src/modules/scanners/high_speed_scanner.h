#pragma once

#include "../../core/module_interface.h"
#include <QProcess>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTemporaryFile>

/**
 * @brief 高速扫描器基类 - 支持ZMap、XMap、Masscan等大规模扫描工具
 * 
 * 特殊处理：
 * - 大规模输出的流式处理
 * - 内存优化和分批处理
 * - 实时进度监控
 * - 结果文件管理
 * - 扫描速率控制
 */
class HighSpeedScanner : public ModuleInterface {
    Q_OBJECT

public:
    // 扫描器类型
    enum ScannerType {
        ZMAP,           // ZMap - IPv4网络扫描
        XMAP,           // XMap - 多协议扫描
        MASSCAN         // Masscan - 高速端口扫描
    };

    // 输出处理模式
    enum OutputMode {
        MEMORY_MODE,    // 内存模式 - 小规模扫描
        STREAM_MODE,    // 流式模式 - 大规模扫描
        FILE_MODE       // 文件模式 - 超大规模扫描
    };

    explicit HighSpeedScanner(ScannerType type, QObject *parent = nullptr);
    ~HighSpeedScanner() override;

    // 基本信息接口实现
    QString getName() const override;
    QString getVersion() const override;
    QString getDescription() const override;
    QString getAuthor() const override { return "Various"; }
    ModuleType getType() const override { return SCANNER; }
    QStringList getDependencies() const override;

    // 参数配置接口实现
    QJsonObject getDefaultOptions() const override;
    bool setOption(const QString& key, const QVariant& value) override;
    QVariant getOption(const QString& key) const override;
    QStringList getRequiredOptions() const override;
    bool validateOptions() const override;

    // 执行控制接口实现
    bool isAvailable() const override;
    bool canExecute() const override;
    void execute() override;
    void stop() override;
    ModuleStatus getStatus() const override { return m_status; }

    // 结果处理接口实现
    QJsonObject getResults() const override;
    QString getLastError() const override { return m_lastError; }
    QStringList getSupportedTargets() const override;

    // 帮助信息接口实现
    QString getUsage() const override;
    QStringList getExamples() const override;
    QString getHelp() const override;

    // 高速扫描特有方法
    void setOutputMode(OutputMode mode);
    void setMaxResults(int maxResults);
    void setScanRate(int packetsPerSecond);
    void setOutputFile(const QString& filePath);
    void setBandwidth(const QString& bandwidth);
    void setInterface(const QString& interface);
    
    // 大规模扫描优化
    void enableStreamProcessing(bool enable);
    void setChunkSize(int size);
    void setProgressInterval(int seconds);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProgressTimer();
    void onResultsProcessed(const QJsonArray& results);

private:
    class ResultProcessor;
    friend class ResultProcessor;
    
    void setupScanner();
    QString buildCommand() const;
    void startResultProcessing();
    void processOutputChunk(const QString& chunk);
    void parseResults(const QString& output);
    void estimateProgress();
    void optimizeForLargeScale();
    void resetState();
    
    // 扫描器特定方法
    QString buildZmapCommand() const;
    QString buildXmapCommand() const;
    QString buildMasscanCommand() const;
    void parseZmapOutput(const QString& output);
    void parseXmapOutput(const QString& output);
    void parseMasscanOutput(const QString& output);

    ScannerType m_scannerType;
    OutputMode m_outputMode;
    ModuleStatus m_status;
    QString m_lastError;
    QJsonObject m_options;
    QJsonObject m_results;
    
    // 进程管理
    QProcess* m_process;
    QTimer* m_progressTimer;
    QString m_outputBuffer;
    QString m_errorBuffer;
    
    // 大规模扫描优化
    QThread* m_processingThread;
    ResultProcessor* m_resultProcessor;
    QTemporaryFile* m_tempOutputFile;
    QMutex m_resultsMutex;
    
    // 性能监控
    qint64 m_startTime;
    qint64 m_lastProgressTime;
    int m_processedResults;
    int m_totalEstimated;
    int m_chunkSize;
    int m_maxResults;
    
    // 配置常量
    static const QHash<ScannerType, QString> SCANNER_NAMES;
    static const QHash<ScannerType, QStringList> SCANNER_DEPENDENCIES;
    static const int DEFAULT_CHUNK_SIZE;
    static const int DEFAULT_MAX_RESULTS;
};

/**
 * @brief 结果处理工作线程 - 处理大规模扫描结果
 */
class HighSpeedScanner::ResultProcessor : public QObject {
    Q_OBJECT

public:
    explicit ResultProcessor(ScannerType type, const QString& outputFile, QObject* parent = nullptr);

public slots:
    void processResults();
    void stopProcessing();

signals:
    void resultsProcessed(const QJsonArray& results);
    void processingCompleted();
    void processingError(const QString& error);

private:
    void processZmapResults(QFile& file);
    void processXmapResults(QFile& file);
    void processMasscanResults(QFile& file);
    QJsonObject parseResultLine(const QString& line, ScannerType type);

    ScannerType m_scannerType;
    QString m_outputFile;
    bool m_stopRequested;
    int m_chunkSize;
};

// ZMap专用模块
class ZmapModule : public HighSpeedScanner {
    Q_OBJECT
public:
    explicit ZmapModule(QObject *parent = nullptr) : HighSpeedScanner(ZMAP, parent) {}
};

// XMap专用模块
class XmapModule : public HighSpeedScanner {
    Q_OBJECT
public:
    explicit XmapModule(QObject *parent = nullptr) : HighSpeedScanner(XMAP, parent) {}
};

// Masscan专用模块
class MasscanModule : public HighSpeedScanner {
    Q_OBJECT
public:
    explicit MasscanModule(QObject *parent = nullptr) : HighSpeedScanner(MASSCAN, parent) {}
};
