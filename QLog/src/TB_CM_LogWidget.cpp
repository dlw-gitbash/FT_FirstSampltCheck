#include <TB_CM_LogWidget.h>

#include <TB_CM_FilterBar.h>
#include <TB_CM_Model.h>
#include <TB_CM_Theme.h>
#include <TB_CM_View.h>

#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int FlushIntervalMs = 16;
constexpr int LogCapacity = 20000;

} // namespace

TB_CM_LogWidget::TB_CM_LogWidget(QWidget* parent)
    : QWidget(parent)
    , model_(new TB_CM_Model(this))
    , view_(new TB_CM_View(this))
    , filterBar_(new TB_CM_FilterBar(this))
    , capacity_(LogCapacity)
{
    view_->setModel(model_);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(filterBar_);
    layout->addWidget(view_, 1);
    flushTimer_.setSingleShot(true);
    flushTimer_.setInterval(FlushIntervalMs);
    connect(&flushTimer_, &QTimer::timeout, this, &TB_CM_LogWidget::flushPending);
    connect(filterBar_, &TB_CM_FilterBar::filterChanged, this,
            &TB_CM_LogWidget::onFilterChanged);
    connect(filterBar_, &TB_CM_FilterBar::wrapChanged, this, &TB_CM_LogWidget::onWrapChanged);
    connect(filterBar_, &TB_CM_FilterBar::followChanged, this, &TB_CM_LogWidget::onFollowChanged);
    connect(filterBar_, &TB_CM_FilterBar::clearRequested, this, &TB_CM_LogWidget::clear);
    connect(filterBar_, &TB_CM_FilterBar::scrollToBottomRequested, this,
            &TB_CM_LogWidget::scrollToBottom);
    connect(view_, &TB_CM_View::clearRequested, this, &TB_CM_LogWidget::clear);
    connect(view_, &TB_CM_View::followTailChanged, this,
            &TB_CM_LogWidget::onViewFollowTailChanged);
    connect(model_, &TB_CM_Model::countsChanged, this, &TB_CM_LogWidget::onCountsChanged);
    const TB_CM_Theme theme = TB_CM_Theme::dark();
    filterBar_->setTheme(theme);
    view_->setTheme(theme);
}
TB_CM_LogWidget::~TB_CM_LogWidget() = default;
void TB_CM_LogWidget::appendLog(const QString& message, quint64 timestampMs, int level)
{
    TB_CM_LogEntry entry;
    entry.message = message;
    entry.timestampMs = timestampMs;
    entry.level = level;
    appendLog(entry);
}
void TB_CM_LogWidget::appendLog(const TB_CM_LogEntry& entry)
{
    {
        const QMutexLocker locker(&mutex_);
        pending_.push_back(entry);
        const size_t limit = size_t(std::max(1, capacity_));
        if (pending_.size() > limit) {
            pending_.erase(pending_.begin(),
                           pending_.begin() + qsizetype(pending_.size() - limit));
        }
    }
    if (QThread::currentThread() == thread()) {
        scheduleFlush();
        return;
    }
    QMetaObject::invokeMethod(this, &TB_CM_LogWidget::scheduleFlush, Qt::QueuedConnection);
}
void TB_CM_LogWidget::scheduleFlush()
{
    if (!flushTimer_.isActive()) {
        flushTimer_.start();
    }
}
void TB_CM_LogWidget::flushPending()
{
    std::vector<TB_CM_LogEntry> batch;
    {
        const QMutexLocker locker(&mutex_);
        batch.swap(pending_);
    }
    if (batch.empty()) {
        return;
    }
    model_->appendBatch(std::move(batch));
}
void TB_CM_LogWidget::clear()
{
    flushTimer_.stop();
    {
        const QMutexLocker locker(&mutex_);
        pending_.clear();
    }
    model_->clear();
}
void TB_CM_LogWidget::scrollToBottom()
{
    view_->setFollowTail(true);
    view_->scrollToBottom();
}
void TB_CM_LogWidget::onCountsChanged(int visible, int total)
{
    filterBar_->setCounts(visible, total);
    emit countsChanged(visible, total);
}
void TB_CM_LogWidget::onFilterChanged()
{
    model_->setFilter(filterBar_->filter());
}
void TB_CM_LogWidget::onWrapChanged(bool enabled)
{
    view_->setWrapEnabled(enabled);
}
void TB_CM_LogWidget::onFollowChanged(bool enabled)
{
    view_->setFollowTail(enabled);
}
void TB_CM_LogWidget::onViewFollowTailChanged(bool enabled)
{
    filterBar_->setFollowTail(enabled);
}
