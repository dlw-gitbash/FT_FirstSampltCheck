#include <TB_CM_Router.h>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QThread>

#include <algorithm>

namespace {

constexpr size_t MaxPending = 200000;

} // namespace

TB_CM_Router& TB_CM_Router::instance()
{
    static TB_CM_Router router;
    return router;
}
TB_CM_Router::TB_CM_Router(QObject* parent)
    : QObject(parent)
{
    // 可能在任意线程首次构造，统一切回 GUI 线程
    QCoreApplication* app = QCoreApplication::instance();
    if (app != nullptr && QThread::currentThread() != app->thread()) {
        moveToThread(app->thread());
    }
}
void TB_CM_Router::post(Record record)
{
    bool schedule = false;
    {
        QMutexLocker locker(&mutex_);
        if (sinks_.empty()) {
            return;
        }
        pending_.push_back(std::move(record));
        if (!flushScheduled_) {
            flushScheduled_ = true;
            schedule = true;
        }
    }
    if (schedule) {
        scheduleFlush();
    }
}
void TB_CM_Router::post(Level level, QString category, QString message)
{
    post(makeRecord(level, std::move(category), std::move(message)));
}
void TB_CM_Router::scheduleFlush()
{
    QMetaObject::invokeMethod(this, [this] { flush(); }, Qt::QueuedConnection);
}
void TB_CM_Router::flush()
{
    std::vector<Record> batch;
    std::vector<Sink*> sinks;
    {
        QMutexLocker locker(&mutex_);
        flushScheduled_ = false;
        batch.swap(pending_);
        sinks = sinks_;
    }
    if (batch.size() > MaxPending) {
        batch.erase(batch.begin(), batch.begin() + qsizetype(batch.size() - MaxPending));
    }
    if (batch.empty() || sinks.empty()) {
        return;
    }
    for (Sink* sink : sinks) {
        sink->onLogBatch(batch);
    }
}
void TB_CM_Router::addSink(Sink* sink)
{
    if (sink == nullptr) {
        return;
    }
    {
        QMutexLocker locker(&mutex_);
        if (std::find(sinks_.begin(), sinks_.end(), sink) != sinks_.end()) {
            return;
        }
        sinks_.push_back(sink);
    }
    // Sink 通常是 QWidget，靠 destroyed 自动摘除，避免悬空指针
    QObject* object = dynamic_cast<QObject*>(sink);
    if (object != nullptr) {
        connect(object, &QObject::destroyed, this, &TB_CM_Router::onSinkDestroyed,
                Qt::DirectConnection);
    }
}
void TB_CM_Router::removeSink(Sink* sink)
{
    QMutexLocker locker(&mutex_);
    std::erase(sinks_, sink);
}
int TB_CM_Router::sinkCount() const
{
    QMutexLocker locker(&mutex_);
    return int(sinks_.size());
}
void TB_CM_Router::onSinkDestroyed()
{
    QObject* object = qobject_cast<QObject*>(sender());
    if (object == nullptr) {
        return;
    }
    Sink* target = nullptr;
    {
        QMutexLocker locker(&mutex_);
        for (Sink* sink : sinks_) {
            if (dynamic_cast<QObject*>(sink) == object) {
                target = sink;
                break;
            }
        }
    }
    if (target != nullptr) {
        removeSink(target);
    }
}
