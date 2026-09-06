#ifndef FT_FUNCTIONFACTORY_H
#define FT_FUNCTIONFACTORY_H

#include <QString>

class FT_FunctionRow;

namespace FtFunctionFactory
{
    FT_FunctionRow* create(const QString& typeName);
    QString defaultTypeName();
}

#endif // FT_FUNCTIONFACTORY_H