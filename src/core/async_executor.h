#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>

/**
 * @brief 异步执行器 - 统一管理工具的异步执行和进度监控
 * 
 * 功能特性：
 * - 多任务并发执行
 * - 实时进度监控
 * - 资源使用控制
 * - 任务队列管理
 * - 错误处理和重试
 * - 执行统计和日志
 */
class AsyncExecutor : public QObject {
    Q_OBJECT

public:
    // 任务状态
    enum TaskStatus {
        PENDING,            // 等待执行
        RUNNING,            // 正在执行
        COMPLETED,          // 执行完成
        FAILED,             // 执行失败
        CANCELLED,          // 已取消
        TIMEOUT             // 执行超时
    };

    // 任务优先级
    enum TaskPriority {
        LOW_PRIORITY = 1,
        NORMAL_PRIORITY = 5,
        HIGH_PRIORITY = 10,
        CRITICAL_PRIORITY = 15
    };

    // 执行任务结构
    struct ExecutionTask {
        QString id;                 // 任务ID
        QString moduleName;         // 模块名称
        QString command;            // 执行命令
        QStringList arguments;      // 命令参数
        QJsonObject options;        // 执行选项
        TaskPriority priority;      // 任务优先级
        int timeout;                // 超时时间（秒）
        int maxRetries;             // 最大重试次数
        QString workingDirectory;   // 工作目录
        QHash<QString, QString> environment; // 环境变量
        QJsonObject metadata;       // 任务元数据
        
        // 运行时状态
        TaskStatus status;          // 当前状态
        QDateTime startTime;        // 开始时间
        QDateTime endTime;          // 结束时间
        int currentRetry;           // 当前重试次数
        QString lastError;          // 最后错误信息
        QJsonObject result;         // 执行结果
        int progress;               // 进度百分比
    };

    static AsyncExecutor& instance();
    
    // 任务管理
    QString submitTask(const ExecutionTask& task);
    bool cancelTask(const QString& taskId);
    bool pauseTask(const QString& taskId);
    bool resumeTask(const QString& taskId);
    void cancelAllTasks();
    void pauseAllTasks();
    void resumeAllTasks();

    // 任务查询
    ExecutionTask getTask(const QString& taskId) const;
    QList<ExecutionTask> getAllTasks() const;
    QList<ExecutionTask> getTasksByStatus(TaskStatus status) const;
    QList<ExecutionTask> getTasksByModule(const QString& moduleName) const;
    TaskStatus getTaskStatus(const QString& taskId) const;
    int getTaskProgress(const QString& taskId) const;

    // 队列管理
    void setMaxConcurrentTasks(int maxTasks);
    int getMaxConcurrentTasks() const;
    int getRunningTaskCount() const;
    int getPendingTaskCount() const;
    void clearCompletedTasks();
    void clearAllTasks();

    // 资源控制
    void setMaxMemoryUsage(qint64 maxMemoryMB);
    void setMaxCpuUsage(int maxCpuPercent);
    void enableResourceMonitoring(bool enable);
    QJsonObject getResourceUsage() const;

    // 执行统计
    QJsonObject getExecutionStatistics() const;
    QJsonArray getExecutionHistory() const;
    void exportExecutionLog(const QString& filePath) const;

    // 配置管理
    void setDefaultTimeout(int seconds);
    void setDefaultRetries(int retries);
    void setProgressUpdateInterval(int milliseconds);
    void enableDetailedLogging(bool enable);

signals:
    void taskSubmitted(const QString& taskId);
    void taskStarted(const QString& taskId);
    void taskProgressUpdated(const QString& taskId, int progress);
    void taskCompleted(const QString& taskId, const QJsonObject& result);
    void taskFailed(const QString& taskId, const QString& error);
    void taskCancelled(const QString& taskId);
    void queueStatusChanged(int pending, int running, int completed);
    void resourceUsageUpdated(const QJsonObject& usage);

private slots:
    void processTaskQueue();
    void onTaskProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTaskProcessError(QProcess::ProcessError error);
    void onProgressTimer();
    void onResourceMonitorTimer();

private:
    AsyncExecutor(QObject* parent = nullptr);
    ~AsyncExecutor() = default;
    AsyncExecutor(const AsyncExecutor&) = delete;
    AsyncExecutor& operator=(const AsyncExecutor&) = delete;

    void startNextTask();
    void executeTask(ExecutionTask& task);
    void completeTask(const QString& taskId, bool success, const QString& error = "");
    void retryTask(const QString& taskId);
    void updateTaskProgress(const QString& taskId, int progress);
    void monitorResourceUsage();
    bool checkResourceLimits() const;
    QString generateTaskId() const;
    void logTaskEvent(const QString& taskId, const QString& event, const QString& details = "");

    QHash<QString, ExecutionTask> m_tasks;
    QQueue<QString> m_taskQueue;
    QHash<QString, QProcess*> m_runningProcesses;
    QHash<QString, QElapsedTimer> m_taskTimers;
    
    QTimer* m_queueTimer;
    QTimer* m_progressTimer;
    QTimer* m_resourceTimer;
    
    int m_maxConcurrentTasks;
    int m_defaultTimeout;
    int m_defaultRetries;
    int m_progressUpdateInterval;
    qint64 m_maxMemoryUsage;
    int m_maxCpuUsage;
    bool m_resourceMonitoringEnabled;
    bool m_detailedLogging;
    
    mutable QMutex m_tasksMutex;
    QJsonArray m_executionHistory;
    
    static AsyncExecutor* s_instance;
    static const int DEFAULT_MAX_CONCURRENT_TASKS;
    static const int DEFAULT_TIMEOUT_SECONDS;
    static const int DEFAULT_MAX_RETRIES;
    static const int DEFAULT_PROGRESS_INTERVAL;
};

/**
 * @brief 进度监控器 - 智能分析工具输出并估算进度
 */
class ProgressMonitor : public QObject {
    Q_OBJECT

public:
    // 进度估算策略
    enum ProgressStrategy {
        TIME_BASED,         // 基于时间估算
        OUTPUT_BASED,       // 基于输出分析
        PATTERN_BASED,      // 基于模式匹配
        HYBRID              // 混合策略
    };

    // 进度模式定义
    struct ProgressPattern {
        QString toolName;           // 工具名称
        QRegularExpression pattern; // 进度模式
        QString progressGroup;      // 进度捕获组
        QString totalGroup;         // 总数捕获组
        int estimatedDuration;      // 预估持续时间
        ProgressStrategy strategy;  // 估算策略
    };

    explicit ProgressMonitor(QObject* parent = nullptr);
    
    // 模式管理
    void addProgressPattern(const ProgressPattern& pattern);
    void removeProgressPattern(const QString& toolName);
    void loadBuiltinPatterns();
    void loadPatternsFromFile(const QString& filePath);

    // 进度分析
    int analyzeProgress(const QString& toolName, const QString& output, 
                       qint64 elapsedTime = 0) const;
    int estimateTimeBasedProgress(qint64 elapsedTime, int estimatedDuration) const;
    int extractOutputProgress(const QString& output, const ProgressPattern& pattern) const;

    // 统计学习
    void recordActualDuration(const QString& toolName, qint64 actualDuration);
    void updateEstimates();
    QJsonObject getProgressStatistics() const;

signals:
    void progressUpdated(const QString& toolName, int progress);

private:
    void setupBuiltinPatterns();
    int calculateHybridProgress(const QString& toolName, const QString& output, 
                               qint64 elapsedTime) const;

    QHash<QString, ProgressPattern> m_patterns;
    QHash<QString, QList<qint64>> m_durationHistory;
    QHash<QString, int> m_estimatedDurations;
};

/**
 * @brief 任务调度器 - 智能任务调度和负载均衡
 */
class TaskScheduler : public QObject {
    Q_OBJECT

public:
    // 调度策略
    enum SchedulingStrategy {
        FIFO,               // 先进先出
        PRIORITY,           // 优先级调度
        ROUND_ROBIN,        // 轮询调度
        LOAD_BALANCED,      // 负载均衡
        RESOURCE_AWARE      // 资源感知调度
    };

    explicit TaskScheduler(QObject* parent = nullptr);
    
    // 调度配置
    void setSchedulingStrategy(SchedulingStrategy strategy);
    void setLoadBalancingEnabled(bool enabled);
    void setResourceAwareScheduling(bool enabled);
    
    // 任务调度
    QString scheduleNextTask(const QList<AsyncExecutor::ExecutionTask>& pendingTasks);
    void updateTaskPriorities();
    void balanceLoad();

    // 性能优化
    void enableTaskGrouping(bool enabled);
    void setTaskAffinityRules(const QHash<QString, QStringList>& rules);
    void optimizeScheduling();

private:
    QString selectByPriority(const QList<AsyncExecutor::ExecutionTask>& tasks);
    QString selectByLoadBalance(const QList<AsyncExecutor::ExecutionTask>& tasks);
    QString selectByResourceUsage(const QList<AsyncExecutor::ExecutionTask>& tasks);
    int calculateTaskWeight(const AsyncExecutor::ExecutionTask& task) const;

    SchedulingStrategy m_strategy;
    bool m_loadBalancingEnabled;
    bool m_resourceAwareEnabled;
    bool m_taskGroupingEnabled;
    QHash<QString, QStringList> m_affinityRules;
};
