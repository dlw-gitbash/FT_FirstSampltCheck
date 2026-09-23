#include "FT_Data.h"
#include <QRegularExpression>
#include <QDebug>

QString ftStrField(const FtJson& obj, const char* key, const QString& fallback)
{
    auto it = obj.find(key);
    if (it != obj.end() && it->is_string())
        return QString::fromStdString(it->get<std::string>());
    return fallback;
}

int ftIntField(const FtJson& obj, const char* key, int fallback)
{
    auto it = obj.find(key);
    if (it != obj.end() && it->is_number_integer())
        return it->get<int>();
    return fallback;
}

bool ftBoolField(const FtJson& obj, const char* key, bool fallback)
{
    auto it = obj.find(key);
    if (it != obj.end() && it->is_boolean())
        return it->get<bool>();
    return fallback;
}

QStringList ftSplitWs(const QString& s)
{
    return s.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
}

bool ftIsHexByteToken(const QString& token)
{
    static const QRegularExpression hexRe(QStringLiteral("^[0-9A-Fa-f]{2}$"));
    return hexRe.match(token).hasMatch();
}

bool ftAllHexByteTokens(const QStringList& tokens)
{
    for (const QString& t : tokens)
        if (!ftIsHexByteToken(t))
            return false;
    return true;
}

QByteArray ftHexBytesToArray(const QStringList& tokens)
{
    QByteArray ba;
    for (const QString& t : tokens) {
        bool ok = false;
        ba.append(static_cast<char>(t.toInt(&ok, 16)));
    }
    return ba;
}

QString ftFunctionDataTypeName(const FT_FunctionData& data)
{
    if (std::holds_alternative<FT_TboxConfig>(data))
        return QStringLiteral("TBoxCommand");
    if (std::holds_alternative<FT_IicWriteConfig>(data))
        return QStringLiteral("IicWrite");
    if (std::holds_alternative<FT_IicWriteReadConfig>(data))
        return QStringLiteral("IicWriteRead");
    return QStringLiteral("Empty");
}

static FtJson advanceToJson(const FT_AdvanceConfig& cfg)
{
    FtJson j;
    j["toResult"]     = ftResultModeToString(cfg.toResult).toStdString();
    j["target"]       = cfg.target.toStdString();
    j["targetMin"]    = cfg.targetMin.toStdString();
    j["targetMax"]    = cfg.targetMax.toStdString();
    j["passMessage"]  = cfg.passMessage.toStdString();
    j["failMessage"]  = cfg.failMessage.toStdString();
    j["failTo"]       = ftFailActionToString(cfg.failTo).toStdString();
    return j;
}

static FT_AdvanceConfig advanceFromJson(const FtJson& j)
{
    FT_AdvanceConfig cfg;
    cfg.toResult    = ftResultModeFromString(ftStrField(j, "toResult"));
    cfg.target      = ftStrField(j, "target");
    cfg.targetMin   = ftStrField(j, "targetMin");
    cfg.targetMax   = ftStrField(j, "targetMax");
    cfg.passMessage = ftStrField(j, "passMessage");
    cfg.failMessage = ftStrField(j, "failMessage");
    cfg.failTo      = ftFailActionFromString(ftStrField(j, "failTo"));
    return cfg;
}

FtJson ftFunctionDataToJson(const FT_FunctionData& data)
{
    if (std::holds_alternative<FT_TboxConfig>(data)) {
        const FT_TboxConfig& t = std::get<FT_TboxConfig>(data);
        FtJson j;
        j["type"]     = "TBoxCommand";
        j["command"]  = t.command.toStdString();
        j["payload"]  = t.payload.toStdString();
        j["advance"]  = advanceToJson(t.advance);
        return j;
    }
    if (std::holds_alternative<FT_IicWriteConfig>(data)) {
        const FT_IicWriteConfig& cfg = std::get<FT_IicWriteConfig>(data);
        FtJson j;
        j["type"]     = "IicWrite";
        j["port"]     = cfg.port;
        j["reg"]      = cfg.reg.toStdString();
        j["payload"]  = cfg.payload.toStdString();
        j["advance"]  = advanceToJson(cfg.advance);
        return j;
    }
    if (std::holds_alternative<FT_IicWriteReadConfig>(data)) {
        const FT_IicWriteReadConfig& cfg = std::get<FT_IicWriteReadConfig>(data);
        FtJson j;
        j["type"]       = "IicWriteRead";
        j["port"]       = cfg.port;
        j["reg"]        = cfg.reg.toStdString();
        j["payload"]    = cfg.payload.toStdString();
        j["readLength"] = cfg.readLength;
        j["advance"]    = advanceToJson(cfg.advance);
        return j;
    }
    return FtJson::object();
}

FT_FunctionData ftFunctionDataFromJson(const FtJson& j)
{
    const QString ty = ftStrField(j, "type");
    if (ty == QLatin1String("TBoxCommand")) {
        FT_TboxConfig t;
        t.command  = ftStrField(j, "command");
        t.payload  = ftStrField(j, "payload");
        auto advIt = j.find("advance");
        if (advIt != j.end() && advIt->is_object())
            t.advance = advanceFromJson(*advIt);
        else
            t.advance = FT_AdvanceConfig{};
        return t;
    }
    if (ty == QLatin1String("IicWrite")) {
        FT_IicWriteConfig cfg;
        cfg.port    = ftIntField(j, "port");
        cfg.reg     = ftStrField(j, "reg");
        cfg.payload = ftStrField(j, "payload");
        auto advIt  = j.find("advance");
        if (advIt != j.end() && advIt->is_object())
            cfg.advance = advanceFromJson(*advIt);
        else
            cfg.advance = FT_AdvanceConfig{};
        return cfg;
    }
    if (ty == QLatin1String("IicWriteRead")) {
        FT_IicWriteReadConfig cfg;
        cfg.port       = ftIntField(j, "port");
        cfg.reg        = ftStrField(j, "reg");
        cfg.payload    = ftStrField(j, "payload");
        cfg.readLength = ftIntField(j, "readLength", 1);
        auto advIt     = j.find("advance");
        if (advIt != j.end() && advIt->is_object())
            cfg.advance = advanceFromJson(*advIt);
        else
            cfg.advance = FT_AdvanceConfig{};
        return cfg;
    }
    return std::monostate{};
}

FtJson ftLinesToJson(const FT_FunctionLines& lines)
{
    FtJson arr = FtJson::array();
    for (const QVector<FT_FunctionData>& line : lines) {
        FtJson lineArr = FtJson::array();
        for (const FT_FunctionData& fn : line)
            lineArr.push_back(ftFunctionDataToJson(fn));
        arr.push_back(std::move(lineArr));
    }
    return arr;
}

FT_FunctionLines ftLinesFromJson(const FtJson& j)
{
    FT_FunctionLines lines;
    if (!j.is_array())
        return lines;

    for (const FtJson& lineJson : j) {
        if (!lineJson.is_array())
            continue;
        QVector<FT_FunctionData> line;
        for (const FtJson& fnJson : lineJson) {
            if (!fnJson.is_object())
                continue;
            FT_FunctionData data = ftFunctionDataFromJson(fnJson);
            if (ftFunctionDataTypeName(data) != QLatin1String("Empty"))
                line.append(std::move(data));
        }
        lines.append(std::move(line));
    }
    return lines;
}

FtJson ftItemToJson(const FT_FunctionItemConfig& config)
{
    FtJson j;
    j["enabled"] = config.enabled;
    j["title"]   = config.title.toStdString();
    j["delayMs"] = config.delayMs;
    j["lines"]   = ftLinesToJson(config.lines);
    return j;
}

FT_FunctionItemConfig ftItemFromJson(const FtJson& j)
{
    FT_FunctionItemConfig cfg;
    cfg.enabled = ftBoolField(j, "enabled", true);
    cfg.title   = ftStrField(j, "title");
    cfg.delayMs = ftIntField(j, "delayMs", 1000);

    auto linesIt = j.find("lines");
    if (linesIt != j.end() && linesIt->is_array()) {
        cfg.lines = ftLinesFromJson(*linesIt);
    } else {
        // 兼容 v1:item 只有单个 "function" 对象 → 单行单命令
        auto funcIt = j.find("function");
        if (funcIt != j.end() && funcIt->is_object()) {
            FT_FunctionData data = ftFunctionDataFromJson(*funcIt);
            if (ftFunctionDataTypeName(data) != QLatin1String("Empty"))
                cfg.lines = FT_FunctionLines{QVector<FT_FunctionData>{std::move(data)}};
        }
    }
    return cfg;
}

FtJson ftDocumentToJson(const FT_FunctionDocument& doc)
{
    FtJson j;
    j["version"] = kFtConfigVersion;
    FtJson itemsArr = FtJson::array();
    for (const FT_FunctionItemConfig& cfg : doc)
        itemsArr.push_back(ftItemToJson(cfg));
    j["functions"] = itemsArr;
    return j;
}

FT_FunctionDocument ftDocumentFromJson(const FtJson& j)
{
    FT_FunctionDocument doc;
    auto funcsIt = j.find("functions");
    if (funcsIt != j.end() && funcsIt->is_array()) {
        for (const FtJson& itemJson : *funcsIt) {
            if (itemJson.is_object())
                doc.append(ftItemFromJson(itemJson));
        }
    }
    return doc;
}