#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVariant>
#include <QHash>
#include <QRegularExpression>

/**
 * @brief 统一命令构建器 - 处理不同工具的命令行参数格式差异
 * 
 * 支持功能：
 * - 多种参数格式转换
 * - 参数验证和转义
 * - 平台特定处理
 * - 命令模板系统
 */
class CommandBuilder {
public:
    // 参数格式类型
    enum ArgumentStyle {
        GNU_STYLE,          // --option value, -o value
        UNIX_STYLE,         // -option value
        WINDOWS_STYLE,      // /option value
        CUSTOM_STYLE        // 自定义格式
    };

    // 参数类型
    enum ArgumentType {
        FLAG,               // 布尔标志 (-v, --verbose)
        VALUE,              // 单值参数 (-p 80, --port 80)
        LIST,               // 列表参数 (-p 80,443,8080)
        KEY_VALUE,          // 键值对 (--header "Key: Value")
        POSITIONAL          // 位置参数 (target)
    };

    // 参数定义结构
    struct ArgumentDefinition {
        QString name;           // 参数名称
        QStringList aliases;    // 别名列表
        ArgumentType type;      // 参数类型
        bool required;          // 是否必需
        QVariant defaultValue;  // 默认值
        QString description;    // 描述
        QRegularExpression validator; // 验证正则表达式
        QString separator;      // 列表分隔符
    };

    explicit CommandBuilder(const QString& toolName, ArgumentStyle style = GNU_STYLE);
    ~CommandBuilder() = default;

    // 参数定义
    void defineArgument(const ArgumentDefinition& argDef);
    void defineFlag(const QString& name, const QStringList& aliases = {}, 
                   const QString& description = "");
    void defineValue(const QString& name, const QStringList& aliases = {}, 
                    bool required = false, const QVariant& defaultValue = QVariant(),
                    const QString& description = "");
    void defineList(const QString& name, const QStringList& aliases = {}, 
                   const QString& separator = ",", const QString& description = "");
    void definePositional(const QString& name, bool required = true, 
                         const QString& description = "");

    // 参数设置
    bool setArgument(const QString& name, const QVariant& value);
    bool setFlag(const QString& name, bool enabled = true);
    bool setList(const QString& name, const QStringList& values);
    bool setPositional(int index, const QString& value);

    // 命令构建
    QString buildCommand() const;
    QStringList buildArgumentList() const;
    QString getExecutablePath() const;
    void setExecutablePath(const QString& path);

    // 验证
    bool validateArguments() const;
    QStringList getValidationErrors() const;
    QStringList getRequiredArguments() const;
    QStringList getMissingArguments() const;

    // 模板系统
    void loadTemplate(const QString& templateName);
    void saveTemplate(const QString& templateName) const;
    QStringList getAvailableTemplates() const;

    // 帮助信息
    QString generateHelp() const;
    QString getArgumentHelp(const QString& name) const;

    // 解析现有命令
    bool parseCommand(const QString& command);
    bool parseArgumentList(const QStringList& args);

    // 工具特定配置
    void setArgumentStyle(ArgumentStyle style);
    void setCustomPrefix(const QString& prefix);
    void setCustomSeparator(const QString& separator);
    void enableQuoting(bool enable);
    void enableEscaping(bool enable);

private:
    QString formatArgument(const ArgumentDefinition& argDef, const QVariant& value) const;
    QString escapeValue(const QString& value) const;
    QString quoteValue(const QString& value) const;
    bool validateValue(const ArgumentDefinition& argDef, const QVariant& value) const;
    ArgumentDefinition* findArgument(const QString& name);
    const ArgumentDefinition* findArgument(const QString& name) const;
    QString resolveAlias(const QString& name) const;

    QString m_toolName;
    QString m_executablePath;
    ArgumentStyle m_argumentStyle;
    QString m_customPrefix;
    QString m_customSeparator;
    bool m_enableQuoting;
    bool m_enableEscaping;

    QHash<QString, ArgumentDefinition> m_arguments;
    QHash<QString, QVariant> m_values;
    QStringList m_positionalValues;
    QStringList m_validationErrors;

    static const QHash<ArgumentStyle, QString> STYLE_PREFIXES;
    static const QHash<ArgumentStyle, QString> STYLE_SEPARATORS;
};

/**
 * @brief 输出解析器 - 统一解析各种工具的输出格式
 */
class OutputParser {
public:
    // 输出格式类型
    enum OutputFormat {
        TEXT_FORMAT,        // 纯文本
        JSON_FORMAT,        // JSON格式
        XML_FORMAT,         // XML格式
        CSV_FORMAT,         // CSV格式
        CUSTOM_FORMAT       // 自定义格式
    };

    // 解析规则
    struct ParseRule {
        QString name;               // 规则名称
        QRegularExpression pattern; // 匹配模式
        QStringList captureGroups;  // 捕获组名称
        QString description;        // 描述
    };

    explicit OutputParser(OutputFormat format = TEXT_FORMAT);
    ~OutputParser() = default;

    // 解析规则定义
    void addParseRule(const ParseRule& rule);
    void addSimpleRule(const QString& name, const QString& pattern, 
                      const QStringList& groups = {});
    void loadRulesFromFile(const QString& filePath);
    void saveRulesToFile(const QString& filePath) const;

    // 解析方法
    QJsonObject parseOutput(const QString& output) const;
    QJsonArray parseMultipleResults(const QString& output) const;
    QJsonObject parseStructuredOutput(const QString& output) const;

    // 格式特定解析
    QJsonObject parseJSON(const QString& jsonText) const;
    QJsonObject parseXML(const QString& xmlText) const;
    QJsonObject parseCSV(const QString& csvText) const;

    // 实时解析
    void startStreamParsing();
    void feedData(const QString& data);
    void finishStreamParsing();

    // 配置
    void setOutputFormat(OutputFormat format);
    void setEncoding(const QString& encoding);
    void setFieldSeparator(const QString& separator);
    void enableMultiline(bool enable);

signals:
    void resultParsed(const QJsonObject& result);
    void parsingCompleted(const QJsonArray& results);
    void parsingError(const QString& error);

private:
    QJsonObject applyParseRules(const QString& text) const;
    QJsonObject extractFields(const QString& text, const ParseRule& rule) const;
    QString preprocessOutput(const QString& output) const;
    QStringList splitIntoRecords(const QString& output) const;

    OutputFormat m_outputFormat;
    QString m_encoding;
    QString m_fieldSeparator;
    bool m_enableMultiline;
    bool m_streamParsing;

    QList<ParseRule> m_parseRules;
    QString m_streamBuffer;
    QJsonArray m_streamResults;
};

/**
 * @brief 工具特定的命令构建器工厂
 */
class ToolCommandFactory {
public:
    static CommandBuilder* createNmapBuilder();
    static CommandBuilder* createSearchsploitBuilder();
    static CommandBuilder* createCurlBuilder();
    static CommandBuilder* createMasscanBuilder();
    static CommandBuilder* createSQLMapBuilder();
    static CommandBuilder* createDirbBuilder();
    static CommandBuilder* createMetasploitBuilder();

    static OutputParser* createNmapParser();
    static OutputParser* createSearchsploitParser();
    static OutputParser* createCurlParser();
    static OutputParser* createMasscanParser();
    static OutputParser* createSQLMapParser();
    static OutputParser* createDirbParser();
    static OutputParser* createMetasploitParser();

private:
    static void setupNmapArguments(CommandBuilder* builder);
    static void setupSearchsploitArguments(CommandBuilder* builder);
    static void setupCurlArguments(CommandBuilder* builder);
    static void setupMasscanArguments(CommandBuilder* builder);
    static void setupSQLMapArguments(CommandBuilder* builder);
    static void setupDirbArguments(CommandBuilder* builder);
    static void setupMetasploitArguments(CommandBuilder* builder);

    static void setupNmapParser(OutputParser* parser);
    static void setupSearchsploitParser(OutputParser* parser);
    static void setupCurlParser(OutputParser* parser);
    static void setupMasscanParser(OutputParser* parser);
    static void setupSQLMapParser(OutputParser* parser);
    static void setupDirbParser(OutputParser* parser);
    static void setupMetasploitParser(OutputParser* parser);
};
