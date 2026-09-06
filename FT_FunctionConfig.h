#ifndef FT_FUNCTIONCONFIG_H
#define FT_FUNCTIONCONFIG_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QByteArray>
#include <QRegularExpression>
#include <variant>
#include <string>
#include <cmath>
#include <climits>
#include <stdexcept>
#include <nlohmann/json.hpp>

using FtJson = nlohmann::ordered_json;

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
    int     delayMs = 1000;
    FT_AdvanceConfig advance;
};

using FT_FunctionRowData = std::variant<std::monostate, FT_TboxConfig>;

struct FT_FunctionCardConfig
{
    bool enabled = true;
    QString title;
    QVector<FT_FunctionRowData> rows;
};

using FT_FunctionDocument = QVector<FT_FunctionCardConfig>;

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

inline QString ftStrField(const FtJson& obj, const char* key, const QString& def = QString())
{
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_string())
        return def;
    return QString::fromStdString(obj.at(key).get<std::string>());
}

inline int ftIntField(const FtJson& obj, const char* key, int def)
{
    if (!obj.is_object() || !obj.contains(key))
        return def;
    const FtJson& v = obj.at(key);
    if (v.is_number_integer())
        return v.get<int>();
    if (v.is_number_float()) {
        const double d = v.get<double>();
        if (std::isfinite(d) && d == std::trunc(d) &&
            d >= static_cast<double>(INT_MIN) && d <= static_cast<double>(INT_MAX))
            return static_cast<int>(d);
    }
    return def;
}

inline bool ftBoolField(const FtJson& obj, const char* key, bool def)
{
    if (!obj.is_object() || !obj.contains(key) || !obj.at(key).is_boolean())
        return def;
    return obj.at(key).get<bool>();
}

inline QStringList ftSplitWs(const QString& raw)
{
    return raw.trimmed().split(QRegularExpression(QStringLiteral("\\s+")),
                               Qt::SkipEmptyParts);
}

inline bool ftIsHexByteToken(const QString& tok)
{
    if (tok.size() < 1 || tok.size() > 2)
        return false;
    for (const QChar c : tok) {
        const ushort u = c.unicode();
        const bool ok = (u >= '0' && u <= '9') ||
                        (u >= 'a' && u <= 'f') ||
                        (u >= 'A' && u <= 'F');
        if (!ok)
            return false;
    }
    return true;
}

inline bool ftAllHexByteTokens(const QString& raw, bool allowEmpty)
{
    const QString t = raw.trimmed();
    if (t.isEmpty())
        return allowEmpty;
    const QStringList parts = ftSplitWs(t);
    if (parts.isEmpty())
        return allowEmpty;
    for (const QString& p : parts) {
        if (!ftIsHexByteToken(p))
            return false;
    }
    return true;
}

inline QByteArray ftHexBytesToArray(const QString& raw)
{
    QByteArray out;
    for (const QString& p : ftSplitWs(raw)) {
        bool ok = false;
        const int v = p.toInt(&ok, 16);
        if (!ok)
            return {};
        out.append(static_cast<char>(static_cast<unsigned char>(v)));
    }
    return out;
}

inline QString ftRowDataTypeName(const FT_FunctionRowData& data)
{
    return std::visit([](const auto& v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, FT_TboxConfig>)
            return QStringLiteral("TBoxCommand");
        else
            return QString();
    }, data);
}

inline FtJson ftRowDataToJson(const FT_FunctionRowData& data)
{
    FtJson obj = FtJson::object();
    obj["typeName"] = ftRowDataTypeName(data).toStdString();

    std::visit([&obj](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, FT_TboxConfig>) {
            obj["command"] = v.command.toStdString();
            obj["payload"] = v.payload.toStdString();
            obj["delayMs"] = v.delayMs;

            FtJson adv = FtJson::object();
            adv["toResult"]    = ftResultModeToString(v.advance.toResult).toStdString();
            adv["target"]      = v.advance.target.toStdString();
            adv["targetMin"]   = v.advance.targetMin.toStdString();
            adv["targetMax"]   = v.advance.targetMax.toStdString();
            adv["passMessage"] = v.advance.passMessage.toStdString();
            adv["failMessage"] = v.advance.failMessage.toStdString();
            adv["failTo"]      = ftFailActionToString(v.advance.failTo).toStdString();
            obj["advance"] = std::move(adv);
        }
    }, data);

    return obj;
}

inline FT_FunctionRowData ftRowDataFromJson(const FtJson& obj)
{
    if (!obj.is_object())
        return std::monostate{};

    const QString typeName = ftStrField(obj, "typeName");

    if (typeName == QLatin1String("TBoxCommand")) {
        FT_TboxConfig c;
        c.command = ftStrField(obj, "command");
        c.payload = obj.contains("payload") ? ftStrField(obj, "payload")
                                            : ftStrField(obj, "argument");
        c.delayMs = obj.contains("delayMs") ? ftIntField(obj, "delayMs", 1000)
                                            : ftIntField(obj, "timeoutMs", 1000);

        if (obj.contains("advance") && obj.at("advance").is_object()) {
            const FtJson& adv = obj.at("advance");
            c.advance.toResult    = ftResultModeFromString(ftStrField(adv, "toResult"));
            c.advance.target      = ftStrField(adv, "target");
            c.advance.targetMin   = ftStrField(adv, "targetMin");
            c.advance.targetMax   = ftStrField(adv, "targetMax");
            c.advance.passMessage = ftStrField(adv, "passMessage");
            c.advance.failMessage = ftStrField(adv, "failMessage");
            c.advance.failTo      = ftFailActionFromString(ftStrField(adv, "failTo"));
        }
        return c;
    }

    return std::monostate{};
}

inline FtJson ftCardToJson(const FT_FunctionCardConfig& card)
{
    FtJson obj = FtJson::object();
    obj["enabled"] = card.enabled;
    obj["title"]   = card.title.toStdString();

    FtJson rows = FtJson::array();
    for (const FT_FunctionRowData& d : card.rows)
        rows.push_back(ftRowDataToJson(d));
    obj["rows"] = std::move(rows);
    return obj;
}

inline FT_FunctionCardConfig ftCardFromJson(const FtJson& obj)
{
    FT_FunctionCardConfig card;
    if (!obj.is_object())
        return card;

    card.enabled = ftBoolField(obj, "enabled", true);
    card.title = obj.contains("title") ? ftStrField(obj, "title")
                                       : ftStrField(obj, "name");

    if (obj.contains("rows") && obj.at("rows").is_array()) {
        for (const FtJson& rv : obj.at("rows")) {
            FT_FunctionRowData data = ftRowDataFromJson(rv);
            if (!std::holds_alternative<std::monostate>(data))
                card.rows.push_back(std::move(data));
        }
    }
    return card;
}

inline FtJson ftDocumentToJson(const FT_FunctionDocument& doc)
{
    FtJson root = FtJson::object();
    root["version"] = kFtConfigVersion;

    FtJson functions = FtJson::array();
    for (const FT_FunctionCardConfig& card : doc)
        functions.push_back(ftCardToJson(card));
    root["functions"] = std::move(functions);
    return root;
}

inline FT_FunctionDocument ftDocumentFromJson(const FtJson& root)
{
    if (!root.is_object())
        throw std::runtime_error("root is not a JSON object");

    int version = kFtConfigVersion;
    if (root.contains("version")) {
        const FtJson& ver = root.at("version");
        if (ver.is_number_integer()) {
            version = ver.get<int>();
        } else if (ver.is_number_float()) {
            const double d = ver.get<double>();
            if (!std::isfinite(d) || d != std::trunc(d))
                throw std::runtime_error("version must be an integer");
            version = static_cast<int>(d);
        } else {
            throw std::runtime_error("version must be an integer");
        }
        if (version < 1 || version > kFtConfigVersion)
            throw std::runtime_error("unsupported config version");
    }

    if (!root.contains("functions") || !root.at("functions").is_array())
        throw std::runtime_error("missing functions array");

    FT_FunctionDocument doc;
    for (const FtJson& fv : root.at("functions")) {
        if (!fv.is_object())
            continue;
        doc.push_back(ftCardFromJson(fv));
    }
    return doc;
}

#endif // FT_FUNCTIONCONFIG_H