#ifndef FT_LOG_H
#define FT_LOG_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutexLocker>
#include <QMutex>
#include <QString>
#include <QDebug>
#include <QCoreApplication>

class FtLog
{
public:
    static void write(const QString& tag, const QString& msg)
    {
        static QMutex mtx;
        QMutexLocker lk(&mtx);

        QFile f(logPath());
        if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            return;

        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
           << " [" << tag << "] " << msg << "\n";
        f.close();
    }

    static QString logPath()
    {
        static QString p = QCoreApplication::applicationDirPath() + "/ft_height_debug.log";
        return p;
    }

    static void clear()
    {
        QFile::remove(logPath());
    }
};

#define FT_LOG(tag, msg) FtLog::write(QString::fromUtf8(tag), QString(msg))

#endif // FT_LOG_H