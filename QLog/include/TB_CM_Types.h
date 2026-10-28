#pragma once

#include <QString>
#include <QStringView>
#include <QtGlobal>

enum TB_CM_Level : int {
    L_TRACE = 0,
    L_DEBUG,
    L_INFO,
    L_WARN,
    L_ERROR,
    L_FATAL,
};

inline constexpr int TB_CM_LevelCount = L_FATAL + 1;

inline QStringView levelName(int level) noexcept
{
    switch (level) {
    case L_TRACE:
        return u"TRACE";
    case L_DEBUG:
        return u"DEBUG";
    case L_WARN:
        return u"WARN";
    case L_ERROR:
        return u"ERROR";
    case L_FATAL:
        return u"FATAL";
    default:
        return u"INFO";
    }
}

struct TB_CM_LogEntry {
    QString message;
    QString category;
    quint64 timestampMs = 0;
    int level = L_INFO;
};
