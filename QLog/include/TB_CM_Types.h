#pragma once

#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QString>
#include <QStringView>
#include <QtGlobal>

#include <array>
#include <utility>

enum class Level : quint8 {
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

inline constexpr int LevelCount = 6;

inline QStringView levelName(Level level) noexcept
{
    switch (level) {
    case Level::Trace:
        return u"TRACE";
    case Level::Debug:
        return u"DEBUG";
    case Level::Info:
        return u"INFO";
    case Level::Warning:
        return u"WARN";
    case Level::Error:
        return u"ERROR";
    case Level::Fatal:
        return u"FATAL";
    }
    return u"LOG";
}

enum class MatchMode : quint8 {
    Substring,
    Regex,
};

struct Filter {
    QString text;
    MatchMode mode = MatchMode::Substring;
    bool caseSensitive = false;
    bool searchCategory = true;
};

struct Record {
    Level level = Level::Info;
    qint64 timestampMs = 0;
    qint64 sequence = 0;
    QString category;
    QString message;
    mutable QString cachedTime;
    mutable int cachedTextWidth = -1;
    mutable int cachedHeight = -1;
};

inline Record makeRecord(Level level, QString category, QString message, qint64 timestampMs = 0)
{
    Record record;
    record.level = level;
    record.timestampMs = timestampMs > 0 ? timestampMs : QDateTime::currentMSecsSinceEpoch();
    record.category = std::move(category);
    record.message = std::move(message);
    return record;
}

struct Theme {
    QColor background;
    QColor alternateBackground;
    QColor text;
    QColor textMuted;
    QColor category;
    QColor separator;
    QColor selection;
    QColor hover;
    QColor matchBackground;
    QColor matchText;
    QColor scrollBar;
    QColor scrollBarHover;
    QColor inputBackground;
    QColor checkedBackground;
    std::array<QColor, LevelCount> levelColors;
    QFont font;
    static Theme dark();
    static Theme light();
};
