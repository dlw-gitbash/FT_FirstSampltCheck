#pragma once

#include <QString>
#include <QDateTime>
#include <QColor>
#include <QVector>
#include <QHash>
#include <QMetaType>
#include <cstdint>
#include <atomic>

enum class LogLevel : quint8 {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5
};

struct LogEntry {
    qint64     timestamp;   // ms since epoch，使用 qint64 避免 QDateTime 频繁构造
    LogLevel   level;
    QString    logger;      // 日志来源模块/类名
    QString    message;     // 日志正文
    QString    threadName;  // 线程名
    int        lineNumber;  // 行号（-1 表示无）
    quint32    index;       // 全局递增序号

    LogEntry()
        : timestamp(0)
        , level(LogLevel::Info)
        , lineNumber(-1)
        , index(0)
    {}
};

namespace LogLevelHelper {

inline const char* levelName(LogLevel lv) {
    switch (lv) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    }
    return "UNKN";
}

inline QColor levelColor(LogLevel lv) {
    switch (lv) {
    case LogLevel::Trace: return QColor("#808080");
    case LogLevel::Debug: return QColor("#00AA00");
    case LogLevel::Info:  return QColor("#000000");
    case LogLevel::Warn:  return QColor("#CC8800");
    case LogLevel::Error: return QColor("#CC0000");
    case LogLevel::Fatal: return QColor("#FFFFFF");
    }
    return QColor("#000000");
}

inline QColor levelBgColor(LogLevel lv) {
    switch (lv) {
    case LogLevel::Trace: return QColor("#F5F5F5");
    case LogLevel::Debug: return QColor("#F0FFF0");
    case LogLevel::Info:  return QColor("#FFFFFF");
    case LogLevel::Warn:  return QColor("#FFF8E1");
    case LogLevel::Error: return QColor("#FFEAEA");
    case LogLevel::Fatal: return QColor("#CC0000");
    }
    return QColor("#FFFFFF");
}

inline LogLevel levelFromString(const QString& s) {
    static QHash<QString, LogLevel> map = {
        {"TRACE", LogLevel::Trace}, {"T", LogLevel::Trace},
        {"DEBUG", LogLevel::Debug}, {"D", LogLevel::Debug},
        {"INFO",  LogLevel::Info},  {"I", LogLevel::Info},
        {"WARN",  LogLevel::Warn},  {"W", LogLevel::Warn},
        {"ERROR", LogLevel::Error}, {"E", LogLevel::Error},
        {"FATAL", LogLevel::Fatal}, {"F", LogLevel::Fatal},
    };
    return map.value(s.toUpper(), LogLevel::Info);
}

} // namespace LogLevelHelper

Q_DECLARE_METATYPE(LogEntry)