#include "FT_Core.h"
#include <QRegularExpression>

namespace FT_Core {
namespace {
struct ModeName { CompareMode mode; const char *name; };
const ModeName kModes[] = {
    { CompareMode::DontCare,   "DontCare"   },
    { CompareMode::Equal,      "equal"      },
    { CompareMode::contains,   "contains"   },
    { CompareMode::uncontains, "uncontains" },
    { CompareMode::startswith, "startswith" },
    { CompareMode::endswith,   "endswith"   },
};
struct ActionName { FailAction action; const char *name; };
const ActionName kActions[] = {
    { FailAction::Stop,        "stop"        },
    { FailAction::RestartThis, "restartthis" },
    { FailAction::RestartAll,  "restartall"  },
};
const char *const kLegacyEqual[]      = { "Equle", "equle", nullptr };
const char *const kLegacyRestartAll[] = { "RestartAll", "restartAll", nullptr };

bool matchName(const char *name, const QString &s)
{
    return QString::fromLatin1(name).compare(s, Qt::CaseInsensitive) == 0;
}
bool matchAny(const char *const *aliases, const QString &s)
{
    for (const char *const *it = aliases; *it; ++it)
        if (matchName(*it, s))
            return true;
    return false;
}
}

QString compareModeToString(CompareMode m)
{
    for (const auto &e : kModes)
        if (e.mode == m)
            return QString::fromLatin1(e.name);
    return QStringLiteral("DontCare");
}
bool compareModeFromString(const QString &s, CompareMode *out)
{
    if (s.isEmpty())
        return false;
    const QString key = s.trimmed();
    for (const auto &e : kModes) {
        if (matchName(e.name, key)) {
            if (out) *out = e.mode;
            return true;
        }
    }
    if (matchAny(kLegacyEqual, key)) {
        if (out) *out = CompareMode::Equal;
        return true;
    }
    return false;
}
QString failActionToString(FailAction a)
{
    for (const auto &e : kActions)
        if (e.action == a)
            return QString::fromLatin1(e.name);
    return QStringLiteral("stop");
}
bool failActionFromString(const QString &s, FailAction *out)
{
    if (s.isEmpty())
        return false;
    const QString key = s.trimmed();
    for (const auto &e : kActions) {
        if (matchName(e.name, key)) {
            if (out) *out = e.action;
            return true;
        }
    }
    if (matchAny(kLegacyRestartAll, key)) {
        if (out) *out = FailAction::RestartAll;
        return true;
    }
    return false;
}
bool isValidHexParam(const QString &text)
{
    static const QRegularExpression hexRe(QStringLiteral("^[0-9A-Fa-f]+$"));
    if (text.isEmpty())
        return false;
    const QStringList tokens = splitHexTokens(text);
    if (tokens.isEmpty())
        return false;
    for (const QString &t : tokens) {
        if (!hexRe.match(t).hasMatch())
            return false;
    }
    return true;
}
QStringList splitHexTokens(const QString &text)
{
    static const QRegularExpression sepRe(QStringLiteral("[\\s,;]+"));
    return text.split(sepRe, Qt::SkipEmptyParts);
}
const char *version()
{
    return "1.0.0";
}
}