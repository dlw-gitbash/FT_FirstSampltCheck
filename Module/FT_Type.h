#ifndef FT_TYPE_H
#define FT_TYPE_H

#include <QString>
#include <QVector>
#include <variant>

// =====================================================
// FT_Type — 纯类型定义模块（Header-only）
// 依赖：Qt Core、C++17
// 职责：所有数据结构、枚举、文档类型定义
// =====================================================

constexpr int kFtConfigVersion = 1;

enum class FtResultMode
{
    DontCare,
    Equal,
    Range,
    StartWith
};

enum class FtFailAction
{
    Stop,
    ContinueThis,
    ContinueAll
};

struct FT_AdvanceConfig
{
    FtResultMode toResult = FtResultMode::DontCare;
    QString target;
    QString targetMin;
    QString targetMax;
    QString passMessage;
    QString failMessage;
    FtFailAction failTo = FtFailAction::Stop;
};

struct FT_TboxConfig
{
    QString command;
    QString payload;
    FT_AdvanceConfig advance;
};

struct FT_IicWriteConfig
{
    int    port    = 0;
    QString reg;
    QString payload;
    FT_AdvanceConfig advance;
};

struct FT_IicWriteReadConfig
{
    int    port       = 0;
    QString reg;
    QString payload;
    int    readLength = 1;
    FT_AdvanceConfig advance;
};

using FT_FunctionData = std::variant<std::monostate, FT_TboxConfig, FT_IicWriteConfig, FT_IicWriteReadConfig>;

struct FT_FunctionItemConfig
{
    bool     enabled = true;
    QString  title;
    FT_FunctionData functionData;
    int      delayMs = 1000;
};

using FT_FunctionDocument = QVector<FT_FunctionItemConfig>;

inline QString ftResultModeToString(FtResultMode m)
{
    switch (m) {
    case FtResultMode::Equal:     return QStringLiteral("Equal");
    case FtResultMode::Range:     return QStringLiteral("Range");
    case FtResultMode::StartWith: return QStringLiteral("StartWith");
    case FtResultMode::DontCare:  default: return QStringLiteral("DontCare");
    }
}

inline FtResultMode ftResultModeFromString(const QString& s)
{
    const QString v = s.trimmed();
    if (v == QLatin1String("Equal"))     return FtResultMode::Equal;
    if (v == QLatin1String("Range"))     return FtResultMode::Range;
    if (v == QLatin1String("StartWith")) return FtResultMode::StartWith;
    return FtResultMode::DontCare;
}

inline QString ftFailActionToString(FtFailAction a)
{
    switch (a) {
    case FtFailAction::ContinueThis: return QStringLiteral("ContinueThis");
    case FtFailAction::ContinueAll:  return QStringLiteral("ContinueAll");
    case FtFailAction::Stop:         default: return QStringLiteral("Stop");
    }
}

inline FtFailAction ftFailActionFromString(const QString& s)
{
    const QString v = s.trimmed();
    if (v == QLatin1String("ContinueThis")) return FtFailAction::ContinueThis;
    if (v == QLatin1String("ContinueAll"))  return FtFailAction::ContinueAll;
    return FtFailAction::Stop;
}

#endif // FT_TYPE_H