#ifndef FT_DATA_H
#define FT_DATA_H

#include "FT_Type.h"
#include <nlohmann/json.hpp>

using FtJson = nlohmann::ordered_json;

QString ftStrField(const FtJson& obj, const char* key, const QString& fallback = {});
int     ftIntField(const FtJson& obj, const char* key, int fallback = 0);
bool    ftBoolField(const FtJson& obj, const char* key, bool fallback = false);

QStringList ftSplitWs(const QString& s);
bool        ftIsHexByteToken(const QString& token);
bool        ftAllHexByteTokens(const QStringList& tokens);
QByteArray  ftHexBytesToArray(const QStringList& tokens);

QString ftFunctionDataTypeName(const FT_FunctionData& data);

FtJson ftFunctionDataToJson(const FT_FunctionData& data);
FT_FunctionData ftFunctionDataFromJson(const FtJson& j);

FtJson ftItemToJson(const FT_FunctionItemConfig& config);
FT_FunctionItemConfig ftItemFromJson(const FtJson& j);

FtJson ftDocumentToJson(const FT_FunctionDocument& doc);
FT_FunctionDocument ftDocumentFromJson(const FtJson& j);

#endif // FT_DATA_H