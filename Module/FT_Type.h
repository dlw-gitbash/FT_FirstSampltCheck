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

constexpr int kFtConfigVersion = 2;

// 命令工厂类型名(FtFunctionFactory::create / 菜单 / 拖拽 MIME 等共用)
inline constexpr const char kFtTypeTbox[]         = "TBoxCommand";
inline constexpr const char kFtTypeIicWrite[]     = "IicWrite";
inline constexpr const char kFtTypeIicWriteRead[] = "IicWriteRead";

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

// 一个 Step(FT_FunctionItem)内的命令分组:
// 每个内层 QVector 代表一条“硬换行”行(右键/拖拽强制断点);
// 同一硬行内的多条命令在宽度不足时仍会自动软折行显示。
using FT_FunctionLines = QVector<QVector<FT_FunctionData>>;

struct FT_FunctionItemConfig
{
    bool     enabled = true;
    QString  title;
    int      delayMs = 1000;
    FT_FunctionLines lines;
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