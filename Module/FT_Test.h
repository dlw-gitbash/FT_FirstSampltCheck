#ifndef FT_TEST_H
#define FT_TEST_H

#include <QObject>
#include "FT_Type.h"

class FT_Test : public QObject
{
    Q_OBJECT
public:
    explicit FT_Test(QObject* parent = nullptr);

signals:
    void testStarted();
    void testFinished(bool passed);
    void logMessage(const QString& msg);

public slots:
    void runDocument(const FT_FunctionDocument& doc);
    void stop();

private:
    bool m_running = false;
};

#endif // FT_TEST_H