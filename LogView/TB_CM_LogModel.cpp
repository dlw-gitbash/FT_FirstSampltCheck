#include "TB_CM_LogModel.h"

#include <QDateTime>
#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>

#include <algorithm>

namespace {

constexpr int DefaultCapacity = 20000;
constexpr int FlushIntervalMs = 16;

} // namespace

TB_CM_LogModel::TB_CM_LogModel(QObject* parent)
    : QAbstractListModel(parent)
    , slots_(size_t(DefaultCapacity))
{
    slots_.shrink_to_fit();
    pending_.reserve(1024);
    flushTimer_.setSingleShot(true);
    flushTimer_.setInterval(FlushIntervalMs);
    connect(&flushTimer_, &QTimer::timeout, this, &TB_CM_LogModel::flushPending);
}
void TB_CM_LogModel::appendLog(const QString& message, quint64 timestampMs, int level)
{
    TB_CM_LogEntry entry;
    entry.message = message;
    entry.timestampMs = timestampMs;
    entry.level = level;
    appendLog(entry);
}
void TB_CM_LogModel::appendLog(const TB_CM_LogEntry& entry)
{
    {
        const QMutexLocker locker(&mutex_);
        pending_.push_back(entry);
        trimPending();
    }
    if (QThread::currentThread() == thread()) {
        scheduleFlush();
        return;
    }
    QMetaObject::invokeMethod(this, &TB_CM_LogModel::scheduleFlush, Qt::QueuedConnection);
}
void TB_CM_LogModel::scheduleFlush()
{
    if (!flushTimer_.isActive()) {
        flushTimer_.start();
    }
}
void TB_CM_LogModel::trimPending()
{
    const size_t limit = size_t(std::max(1, capacity()));
    if (pending_.size() > limit) {
        pending_.erase(pending_.begin(), pending_.begin() + qsizetype(pending_.size() - limit));
    }
}
void TB_CM_LogModel::flushPending()
{
    std::vector<TB_CM_LogEntry> batch;
    {
        const QMutexLocker locker(&mutex_);
        batch.swap(pending_);
    }
    if (batch.empty()) {
        return;
    }
    commit(std::move(batch));
}
void TB_CM_LogModel::commit(std::vector<TB_CM_LogEntry>&& entries)
{
    const size_t capacity = slots_.size();
    if (capacity == 0 || entries.empty()) {
        return;
    }
    const qint64 oldEnd = nextSeq_;
    size_t begin = 0;
    if (entries.size() > capacity) {
        begin = entries.size() - capacity;
    }
    for (size_t i = begin; i < entries.size(); ++i) {
        TB_CM_Record& slot = slots_[size_t(nextSeq_ % qint64(capacity))];
        slot = TB_CM_Record();
        slot.message = std::move(entries[i].message);
        slot.category = std::move(entries[i].category);
        slot.timestampMs = entries[i].timestampMs;
        slot.level = entries[i].level;
        slot.sequence = nextSeq_;
        ++nextSeq_;
        if (nextSeq_ - firstSeq_ > qint64(capacity)) {
            firstSeq_ = nextSeq_ - qint64(capacity);
        }
    }
    const qint64 end = nextSeq_;
    if (end <= firstSeq_ || end <= oldEnd) {
        emit countsChanged(visibleCount(), totalCount());
        return;
    }
    std::vector<qint64> added;
    added.reserve(size_t(end - std::max(firstSeq_, oldEnd)));
    for (qint64 seq = std::max(firstSeq_, oldEnd); seq < end; ++seq) {
        if (matches(slots_[size_t(seq % qint64(capacity))])) {
            added.push_back(seq);
        }
    }
    int removedRows = 0;
    while (removedRows < int(visible_.size()) && visible_[size_t(removedRows)] < firstSeq_) {
        ++removedRows;
    }
    if (removedRows > 0) {
        beginRemoveRows(QModelIndex(), 0, removedRows - 1);
        visible_.erase(visible_.begin(), visible_.begin() + removedRows);
        endRemoveRows();
    }
    if (!added.empty()) {
        const int firstRow = int(visible_.size());
        beginInsertRows(QModelIndex(), firstRow, firstRow + int(added.size()) - 1);
        visible_.insert(visible_.end(), added.begin(), added.end());
        endInsertRows();
        emit rowsAppended(firstRow, firstRow + int(added.size()) - 1);
    }
    emit countsChanged(visibleCount(), totalCount());
}
void TB_CM_LogModel::clear()
{
    flushTimer_.stop();
    std::vector<TB_CM_LogEntry> dropped;
    {
        const QMutexLocker locker(&mutex_);
        dropped.swap(pending_);
        for (TB_CM_Record& record : slots_) {
            record = TB_CM_Record();
        }
        firstSeq_ = 0;
        nextSeq_ = 0;
        visible_.clear();
    }
    beginResetModel();
    endResetModel();
    emit countsChanged(0, 0);
}
void TB_CM_LogModel::setFilter(const TB_CM_Filter& filter)
{
    filter_ = filter;
    compileFilter();
    emit filterChanged();
    rebuildVisible();
}
void TB_CM_LogModel::compileFilter()
{
    regexValid_ = true;
    if (filter_.text.isEmpty() || filter_.mode != TB_CM_MatchMode::Regex) {
        return;
    }
    regex_.setPatternOptions(filter_.caseSensitive ? QRegularExpression::NoPatternOption
                                                   : QRegularExpression::CaseInsensitiveOption);
    regex_.setPattern(filter_.text);
    // 非法正则退化为子串匹配，避免视图瞬间变空
    regexValid_ = regex_.isValid();
}
bool TB_CM_LogModel::matches(const TB_CM_Record& record) const
{
    if (filter_.text.isEmpty()) {
        return true;
    }
    if (filter_.mode == TB_CM_MatchMode::Regex && regexValid_) {
        if (regex_.match(record.message).hasMatch()) {
            return true;
        }
        return filter_.searchCategory && !record.category.isEmpty()
            && regex_.match(record.category).hasMatch();
    }
    const Qt::CaseSensitivity cs = filter_.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (record.message.contains(filter_.text, cs)) {
        return true;
    }
    return filter_.searchCategory && record.category.contains(filter_.text, cs);
}
void TB_CM_LogModel::rebuildVisible()
{
    beginResetModel();
    visible_.clear();
    visible_.reserve(size_t(totalCount()));
    for (qint64 seq = firstSeq_; seq < nextSeq_; ++seq) {
        if (matches(slots_[size_t(seq % qint64(slots_.size()))])) {
            visible_.push_back(seq);
        }
    }
    endResetModel();
    emit countsChanged(visibleCount(), totalCount());
}
void TB_CM_LogModel::invalidateLayoutCache()
{
    for (qint64 seq = firstSeq_; seq < nextSeq_; ++seq) {
        TB_CM_Record& record = slots_[size_t(seq % qint64(slots_.size()))];
        record.cachedWidth = -1;
        record.cachedHeight = -1;
    }
}
const TB_CM_Record* TB_CM_LogModel::recordAt(int row) const
{
    if (row < 0 || row >= int(visible_.size())) {
        return nullptr;
    }
    return &slots_[size_t(visible_[size_t(row)] % qint64(slots_.size()))];
}
int TB_CM_LogModel::capacity() const
{
    return int(slots_.size());
}
int TB_CM_LogModel::totalCount() const
{
    return int(nextSeq_ - firstSeq_);
}
int TB_CM_LogModel::visibleCount() const
{
    return int(visible_.size());
}
int TB_CM_LogModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : visibleCount();
}
QVariant TB_CM_LogModel::data(const QModelIndex& index, int role) const
{
    const TB_CM_Record* record = recordAt(index.row());
    if (record == nullptr) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
    case MessageRole:
        return record->message;
    case LevelRole:
        return record->level;
    case CategoryRole:
        return record->category;
    case TimestampRole:
        return qint64(record->timestampMs);
    case Qt::ToolTipRole: {
        const QString time = QDateTime::fromMSecsSinceEpoch(qint64(record->timestampMs))
                                 .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        QString tip = QStringLiteral("%1  [%2]").arg(time, levelName(record->level).toString());
        if (!record->category.isEmpty()) {
            tip += QStringLiteral("  [%1]").arg(record->category);
        }
        if (!record->message.isEmpty()) {
            tip += QLatin1Char('\n') + record->message;
        }
        return tip;
    }
    default:
        return {};
    }
}
Qt::ItemFlags TB_CM_LogModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
