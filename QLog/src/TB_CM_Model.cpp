#include <TB_CM_Model.h>

#include <QDateTime>

#include <algorithm>

TB_CM_Model::TB_CM_Model(QObject* parent)
    : QAbstractListModel(parent)
{
}
void TB_CM_Model::appendBatch(std::vector<Record>&& records)
{
    if (records.empty()) {
        return;
    }
    const qint64 oldEnd = document_.nextSeq();
    document_.appendBatch(std::move(records));
    const qint64 newFirst = document_.firstSeq();
    const qint64 end = document_.nextSeq();
    if (end <= newFirst || end <= oldEnd) {
        emit countsChanged(visibleCount(), totalCount());
        return;
    }
    std::vector<qint64> added;
    added.reserve(size_t(end - std::max(newFirst, oldEnd)));
    for (qint64 seq = std::max(newFirst, oldEnd); seq < end; ++seq) {
        if (matches(document_.at(seq))) {
            added.push_back(seq);
        }
    }
    int removedRows = 0;
    while (removedRows < int(visible_.size()) && visible_[size_t(removedRows)] < newFirst) {
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
void TB_CM_Model::clear()
{
    beginResetModel();
    document_.clear();
    visible_.clear();
    endResetModel();
    emit countsChanged(0, 0);
}
void TB_CM_Model::setCapacity(int capacity)
{
    const int previous = document_.capacity();
    document_.setCapacity(capacity);
    if (document_.capacity() != previous) {
        rebuildVisible();
    }
}
void TB_CM_Model::setFilter(const Filter& filter)
{
    filter_ = filter;
    compileFilter();
    emit filterChanged();
    rebuildVisible();
}
bool TB_CM_Model::matches(const Record& record) const
{
    if (filter_.text.isEmpty()) {
        return true;
    }
    if (filter_.mode == MatchMode::Regex && regexValid_) {
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
void TB_CM_Model::compileFilter()
{
    regexValid_ = true;
    if (filter_.text.isEmpty() || filter_.mode != MatchMode::Regex) {
        return;
    }
    regex_.setPatternOptions(filter_.caseSensitive ? QRegularExpression::NoPatternOption
                                                   : QRegularExpression::CaseInsensitiveOption);
    regex_.setPattern(filter_.text);
    // 非法正则退化为子串匹配，避免视图瞬间变空
    regexValid_ = regex_.isValid();
}
void TB_CM_Model::rebuildVisible()
{
    beginResetModel();
    visible_.clear();
    visible_.reserve(size_t(document_.size()));
    for (qint64 seq = document_.firstSeq(); seq < document_.nextSeq(); ++seq) {
        if (matches(document_.at(seq))) {
            visible_.push_back(seq);
        }
    }
    endResetModel();
    emit countsChanged(visibleCount(), totalCount());
}
void TB_CM_Model::invalidateLayoutCache()
{
    for (qint64 seq = document_.firstSeq(); seq < document_.nextSeq(); ++seq) {
        Record& record = document_.at(seq);
        record.cachedTextWidth = -1;
        record.cachedHeight = -1;
    }
}
const Record* TB_CM_Model::recordAt(int row) const
{
    if (row < 0 || row >= int(visible_.size())) {
        return nullptr;
    }
    return &document_.at(visible_[size_t(row)]);
}
int TB_CM_Model::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : visibleCount();
}
QVariant TB_CM_Model::data(const QModelIndex& index, int role) const
{
    const Record* record = recordAt(index.row());
    if (record == nullptr) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
    case MessageRole:
        return record->message;
    case LevelRole:
        return int(record->level);
    case CategoryRole:
        return record->category;
    case TimestampRole:
        return record->timestampMs;
    case Qt::ToolTipRole: {
        const QString time = QDateTime::fromMSecsSinceEpoch(record->timestampMs)
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
Qt::ItemFlags TB_CM_Model::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
