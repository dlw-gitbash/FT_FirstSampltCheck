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

// 调试开关:排查高度/宽度问题时把下面的值改成 1。
// 打开后每条日志都要打开/追加/关闭一次 ft_height_debug.log,
// 而 resize 热路径上日志很密,开着会明显拖慢界面(窗口缩放/最大化还原尤其明显),
// 所以默认关闭;FT_LOG() 在关闭时会被整体替换成空语句,连字符串拼接都不会发生。
#ifndef FT_LOG_ENABLED
#define FT_LOG_ENABLED 0
#endif

#if FT_LOG_ENABLED
#define FT_LOG(tag, msg) FtLog::write(QString::fromUtf8(tag), QString(msg))
#else
#define FT_LOG(tag, msg) do {} while (0)
#endif

#endif // FT_LOG_H