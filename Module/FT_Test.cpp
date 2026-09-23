#include "FT_Test.h"

FT_Test::FT_Test(QObject* parent)
    : QObject(parent)
{
}

void FT_Test::runDocument(const FT_FunctionDocument& doc)
{
    Q_UNUSED(doc);
    m_running = true;
    emit testStarted();

    emit logMessage(tr("Test execution engine not yet implemented."));

    m_running = false;
    emit testFinished(true);
}

void FT_Test::stop()
{
    m_running = false;
    emit logMessage(tr("Test stopped by user."));
}