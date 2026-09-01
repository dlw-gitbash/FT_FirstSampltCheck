#pragma once
#include <QString>
#include <QStringList>

namespace FT_Core {
enum class CompareMode {
    DontCare,
    Equal,
    contains,
    uncontains,
    startswith,
    endswith,
};
enum class FailAction {
    Stop,
    RestartThis,
    RestartAll,
};
QString compareModeToString(CompareMode m);
bool    compareModeFromString(const QString &s, CompareMode *out);
QString failActionToString(FailAction a);
bool    failActionFromString(const QString &s, FailAction *out);
bool isValidHexParam(const QString &text);
QStringList splitHexTokens(const QString &text);
const char *version();
}