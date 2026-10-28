#include <TB_CM_LogWidget.h>

#include <TB_CM_FilterBar.h>
#include <TB_CM_Model.h>
#include <TB_CM_View.h>

#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>

TB_CM_LogWidget::TB_CM_LogWidget(QWidget* parent)
    : QWidget(parent)
    , model_(new TB_CM_Model(this))
    , view_(new TB_CM_View(this))
    , filterBar_(new TB_CM_FilterBar(this))
{
    view_->setModel(model_);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(filterBar_);
    layout->addWidget(view_, 1);
    flushTimer_.setSingleShot(true);
    flushTimer_.setInterval(16);
    connect(&flushTimer_, &QTimer::timeout, this, &TB_CM_LogWidget::flushPending);
    connect(filterBar_, &TB_CM_FilterBar::filterChanged, this, &TB_CM_LogWidget::onFilterBarChanged);
    connect(filterBar_, &TB_CM_FilterBar::wrapChanged, this,
            &TB_CM_LogWidget::onFilterBarWrapChanged);
    connect(filterBar_, &TB_CM_FilterBar::followChanged, this,
            &TB_CM_LogWidget::onFilterBarFollowChanged);
    connect(filterBar_, &TB_CM_FilterBar::clearRequested, this, &TB_CM_LogWidget::clear);
    connect(filterBar_, &TB_CM_FilterBar::scrollToBottomRequested, this,
            &TB_CM_LogWidget::onScrollToBottomRequested);
    connect(view_, &TB_CM_View::clearRequested, this, &TB_CM_LogWidget::clear);
    connect(view_, &TB_CM_View::followTailChanged, this,
            &TB_CM_LogWidget::onViewFollowTailChanged);
    connect(model_, &TB_CM_Model::countsChanged, this, &TB_CM_LogWidget::onCountsChanged);
    setTheme(Theme::dark());
}
TB_CM_LogWidget::~TB_CM_LogWidget()
{
    detachFromRouter();
}
void TB_CM_LogWidget::append(Level level, QStringView category, QString message)
{
    std::vector<Record> batch;
    batch.push_back(makeRecord(level, category.toString(), std::move(message)));
    enqueue(std::move(batch));
}
void TB_CM_LogWidget::append(const Record& record)
{
    std::vector<Record> batch;
    batch.push_back(record);
    enqueue(std::move(batch));
}
void TB_CM_LogWidget::appendBatch(std::vector<Record> records)
{
    enqueue(std::move(records));
}
void TB_CM_LogWidget::enqueue(std::vector<Record> records)
{
    if (records.empty()) {
        return;
    }
    const size_t capacity = size_t(std::max(1, model_->capacity()));
    if (records.size() > capacity) {
        records.erase(records.begin(), records.begin() + qsizetype(records.size() - capacity));
    }
    pending_.insert(pending_.end(), std::make_move_iterator(records.begin()),
                    std::make_move_iterator(records.end()));
    trimPending();
    if (!flushTimer_.isActive()) {
        flushTimer_.start();
    }
}
void TB_CM_LogWidget::onLogBatch(std::vector<Record>& batch)
{
    if (batch.empty()) {
        return;
    }
    // 同一批次会分发给多个 Sink，这里只能拷贝
    pending_.insert(pending_.end(), batch.begin(), batch.end());
    trimPending();
    if (!flushTimer_.isActive()) {
        flushTimer_.start();
    }
}
void TB_CM_LogWidget::trimPending()
{
    const size_t capacity = size_t(std::max(1, model_->capacity()));
    if (pending_.size() > capacity) {
        pending_.erase(pending_.begin(), pending_.begin() + qsizetype(pending_.size() - capacity));
    }
}
void TB_CM_LogWidget::flushPending()
{
    if (flushTimer_.isActive()) {
        flushTimer_.stop();
    }
    if (pending_.empty()) {
        return;
    }
    std::vector<Record> batch;
    batch.swap(pending_);
    model_->appendBatch(std::move(batch));
}
void TB_CM_LogWidget::clear()
{
    flushTimer_.stop();
    pending_.clear();
    model_->clear();
}
void TB_CM_LogWidget::setCapacity(int capacity)
{
    model_->setCapacity(capacity);
    trimPending();
}
int TB_CM_LogWidget::capacity() const
{
    return model_->capacity();
}
void TB_CM_LogWidget::setTheme(const Theme& theme)
{
    theme_ = theme;
    filterBar_->setTheme(theme);
    view_->setTheme(theme);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, theme.background);
    palette.setColor(QPalette::Base, theme.background);
    palette.setColor(QPalette::Text, theme.text);
    setPalette(palette);
    setAutoFillBackground(true);
}
void TB_CM_LogWidget::setWrapEnabled(bool enabled)
{
    view_->setWrapEnabled(enabled);
    if (filterBar_->isWrapEnabled() != enabled) {
        QSignalBlocker blocker(filterBar_);
        filterBar_->setWrapEnabled(enabled);
    }
}
bool TB_CM_LogWidget::isWrapEnabled() const
{
    return view_->isWrapEnabled();
}
void TB_CM_LogWidget::setFollowTail(bool enabled)
{
    view_->setFollowTail(enabled);
    if (filterBar_->isFollowTail() != enabled) {
        QSignalBlocker blocker(filterBar_);
        filterBar_->setFollowTail(enabled);
    }
}
bool TB_CM_LogWidget::isFollowTail() const
{
    return view_->isFollowTail();
}
void TB_CM_LogWidget::setFlushInterval(int milliseconds)
{
    flushTimer_.setInterval(std::max(1, milliseconds));
}
void TB_CM_LogWidget::scrollToBottom()
{
    view_->setFollowTail(true);
    view_->scrollToBottom();
}
void TB_CM_LogWidget::attachToRouter()
{
    if (attached_) {
        return;
    }
    TB_CM_Router::instance().addSink(this);
    attached_ = true;
}
void TB_CM_LogWidget::detachFromRouter()
{
    if (!attached_) {
        return;
    }
    TB_CM_Router::instance().removeSink(this);
    attached_ = false;
}
void TB_CM_LogWidget::onCountsChanged(int visible, int total)
{
    filterBar_->setCounts(visible, total);
    emit countsChanged(visible, total);
}
void TB_CM_LogWidget::onFilterBarChanged()
{
    model_->setFilter(filterBar_->filter());
}
void TB_CM_LogWidget::onFilterBarWrapChanged(bool enabled)
{
    setWrapEnabled(enabled);
}
void TB_CM_LogWidget::onFilterBarFollowChanged(bool enabled)
{
    setFollowTail(enabled);
}
void TB_CM_LogWidget::onViewFollowTailChanged(bool enabled)
{
    {
        QSignalBlocker blocker(filterBar_);
        filterBar_->setFollowTail(enabled);
    }
    emit followTailChanged(enabled);
}
void TB_CM_LogWidget::onScrollToBottomRequested()
{
    scrollToBottom();
}
