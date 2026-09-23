#ifndef FT_PROJECT_H
#define FT_PROJECT_H

#include <QString>
#include "FT_Type.h"

class FT_FunctionList;

class FT_Project
{
public:
    explicit FT_Project(FT_FunctionList* list);

    bool isDirty() const { return m_dirty; }
    void markDirty();
    void markClean();

    FT_FunctionDocument collectDocument() const;
    bool exportToFile(const QString& path);
    bool importFromFile(const QString& path);

    QString lastError() const { return m_lastError; }

private:
    bool confirmDiscardIfDirty(const QString& actionTitle);
    bool validatePayloadHex(const FT_FunctionDocument& doc);

    FT_FunctionList* m_funcList = nullptr;
    bool             m_dirty    = false;
    bool             m_suppressDirty = false;
    QString          m_lastError;
};

#endif // FT_PROJECT_H