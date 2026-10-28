#pragma once

#include <TB_CM_Types.h>

#include <QMutex>
#include <QObject>

#include <vector>

class TB_CM_Router final : public QObject
{
    Q_OBJECT

public:
    class Sink
    {
    public:
        virtual ~Sink() = default;
        virtual void onLogBatch(std::vector<Record>& batch) = 0;
    };

    static TB_CM_Router& instance();

    void post(Record record);
    void post(Level level, QString category, QString message);
    void addSink(Sink* sink);
    void removeSink(Sink* sink);
    int sinkCount() const;

private:
    explicit TB_CM_Router(QObject* parent = nullptr);

    void scheduleFlush();
    void flush();
    void onSinkDestroyed();

private:
    mutable QMutex mutex_;
    std::vector<Record> pending_;
    std::vector<Sink*> sinks_;
    bool flushScheduled_ = false;
};
