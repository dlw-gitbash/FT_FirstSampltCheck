#include "FT_FunctionFactory.h"
#include "FT_FunctionRow.h"
#include "FT_TboxFunctionRow.h"

FT_FunctionRow* FtFunctionFactory::create(const QString& typeName)
{
    if (typeName == QLatin1String("TBoxCommand"))
        return new FT_TboxFunctionRow();

    return nullptr;
}

QString FtFunctionFactory::defaultTypeName()
{
    return QStringLiteral("TBoxCommand");
}