#include "LogModel.h"
#include <QDateTime>
#include <QBrush>
#include <QFont>

LogModel::LogModel(int maxEntries, QObject* parent)
    : QAbstractTableModel(parent)
    , m_offset(0)
    , m_maxEntries(qMax(1000, maxEntries))
    , m_totalCount(0)
    , m_batchInterval(50)
    , m_flushScheduled(false)
{
    connect(&m_flushTimer, &QTimer::timeout, this, &LogModel::flushPending);
}

LogModel::~LogModel()
{
    m_flushTimer.stop();
}

int LogModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_buffer.size());
}

int LogModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};

    int row = index.row();
    int col = index.column();

    if (row < 0 || row >= static_cast<int>(m_buffer.size())) return {};

    const LogEntry& entry = m_buffer[row];

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColTimestamp: {
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(entry.timestamp);
            return dt.toString("yyyy-MM-dd HH:mm:ss.zzz");
        }
        case ColLevel:
            return QString::fromLatin1(LogLevelHelper::levelName(entry.level));
        case ColMessage:
            return entry.message;
        }
    }

    if (role == Qt::ForegroundRole) {
        return QBrush(LogLevelHelper::levelColor(entry.level));
    }

    if (role == Qt::BackgroundRole) {
        return QBrush(LogLevelHelper::levelBgColor(entry.level));
    }

    if (role == Qt::FontRole && col == ColLevel) {
        QFont f;
        f.setBold(true);
        return f;
    }

    if (role == Qt::TextAlignmentRole) {
        return QVariant(static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
    }

    if (role == Qt::UserRole) {
        return static_cast<int>(entry.level);
    }

    if (role == Qt::UserRole + 1) {
        return entry.timestamp;
    }

    if (role == Qt::UserRole + 2) {
        return entry.message;
    }

    return {};
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColTimestamp: return QStringLiteral("时间戳");
    case ColLevel:     return QStringLiteral("级别");
    case ColMessage:   return QStringLiteral("消息内容");
    }
    return {};
}

void LogModel::addEntry(LogEntry entry)
{
    entry.index = m_totalCount.fetch_add(1, std::memory_order_acq_rel);

    {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingEntries.push_back(std::move(entry));
    }

    if (m_batchInterval <= 0) {
        flushPending();
    } else if (!m_flushScheduled) {
        m_flushScheduled = true;
        // 使用单次定时器，避免频繁触发
        QMetaObject::invokeMethod(this, [this]() {
            m_flushTimer.start(m_batchInterval);
        }, Qt::QueuedConnection);
    }
}

void LogModel::addEntries(std::vector<LogEntry>& entries)
{
    if (entries.empty()) return;

    quint32 baseIndex = m_totalCount.fetch_add(static_cast<quint32>(entries.size()),
                                                std::memory_order_acq_rel);
    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].index = baseIndex + static_cast<quint32>(i);
    }

    {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingEntries.insert(m_pendingEntries.end(),
                                std::make_move_iterator(entries.begin()),
                                std::make_move_iterator(entries.end()));
    }

    if (m_batchInterval <= 0) {
        flushPending();
    } else if (!m_flushScheduled) {
        m_flushScheduled = true;
        QMetaObject::invokeMethod(this, [this]() {
            m_flushTimer.start(0);
        }, Qt::QueuedConnection);
    }
}

void LogModel::flushPending()
{
    m_flushTimer.stop();
    m_flushScheduled = false;

    std::vector<LogEntry> batch;
    {
        QMutexLocker locker(&m_pendingMutex);
        batch.swap(m_pendingEntries);
    }

    if (batch.empty()) return;

    int oldSize = static_cast<int>(m_buffer.size());
    int addCount = static_cast<int>(batch.size());

    if (oldSize + addCount > m_maxEntries) {
        // 需要淘汰旧条目
        int toRemove = oldSize + addCount - m_maxEntries;
        if (toRemove >= oldSize) {
            // 全部清空重新填充
            beginResetModel();
            m_buffer.clear();
            m_offset += oldSize;
            int keepFrom = toRemove - oldSize;
            if (keepFrom < addCount) {
                m_buffer.assign(
                    std::make_move_iterator(batch.begin() + keepFrom),
                    std::make_move_iterator(batch.end()));
            }
            endResetModel();
        } else {
            // 移除头部 toRemove 条
            beginRemoveRows(QModelIndex(), 0, toRemove - 1);
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + toRemove);
            m_offset += toRemove;
            endRemoveRows();

            // 追加新条目
            int newFirst = static_cast<int>(m_buffer.size());
            int newLast = newFirst + addCount - 1;
            beginInsertRows(QModelIndex(), newFirst, newLast);
            m_buffer.insert(m_buffer.end(),
                            std::make_move_iterator(batch.begin()),
                            std::make_move_iterator(batch.end()));
            endInsertRows();
        }
    } else {
        beginInsertRows(QModelIndex(), oldSize, oldSize + addCount - 1);
        m_buffer.insert(m_buffer.end(),
                        std::make_move_iterator(batch.begin()),
                        std::make_move_iterator(batch.end()));
        endInsertRows();
    }

    emit statsChanged(static_cast<int>(m_buffer.size()),
                      m_totalCount.load(std::memory_order_acquire));
}

void LogModel::setMaxEntries(int max)
{
    max = qMax(1000, max);
    if (m_maxEntries == max) return;
    m_maxEntries = max;

    // 如果当前条目超过新上限，裁剪
    int over = static_cast<int>(m_buffer.size()) - m_maxEntries;
    if (over > 0) {
        beginRemoveRows(QModelIndex(), 0, over - 1);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + over);
        m_offset += over;
        endRemoveRows();
    }
}

void LogModel::clear()
{
    beginResetModel();
    m_buffer.clear();
    m_offset = 0;
    m_totalCount.store(0, std::memory_order_release);
    endResetModel();
    emit statsChanged(0, 0);
}

const LogEntry* LogModel::entryAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_buffer.size())) return nullptr;
    return &m_buffer[row];
}

void LogModel::setBatchInterval(int ms)
{
    m_batchInterval = ms;
}

void LogModel::doInsertRows(int first, int last)
{
    Q_UNUSED(first)
    Q_UNUSED(last)
}