#pragma once

#include "LogEntry.h"
#include <QAbstractTableModel>
#include <QTimer>
#include <QMutex>
#include <vector>
#include <deque>
#include <atomic>
#include <memory>

// =====================================================
// LogModel：高性能环形缓冲区日志模型
// 特点：
//   1. 固定内存上限（环形缓冲区），不随日志量无限增长
//   2. 批量添加接口，配合 beginInsertRows/endInsertRows 减少通知次数
//   3. 定时器批量刷新，避免高频日志频繁重绘
//   4. 线程安全的添加接口（addEntry / addEntries）
// =====================================================
class LogModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        ColTimestamp  = 0,
        ColLevel      = 1,
        ColMessage    = 2,
        ColumnCount   = 3
    };

    explicit LogModel(int maxEntries = 1000000, QObject* parent = nullptr);
    ~LogModel() override;

    // --- QAbstractTableModel 接口 ---
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // --- 日志添加（线程安全） ---
    // 单个添加（内部会触发批量刷新逻辑）
    void addEntry(LogEntry entry);

    // 批量添加（推荐，性能最优）
    void addEntries(std::vector<LogEntry>& entries);

    // --- 缓冲区操作 ---
    // 设置最大条目数
    void setMaxEntries(int max);
    int  maxEntries() const { return m_maxEntries; }

    // 清空所有日志
    void clear();

    // 获取当前总条目数（含已淘汰的）
    quint32 totalCount() const { return m_totalCount.load(std::memory_order_acquire); }

    // 通过缓冲区索引获取原始 LogEntry
    const LogEntry* entryAt(int row) const;

    // 设置批量刷新间隔（毫秒），0 表示立即刷新
    void setBatchInterval(int ms);

signals:
    void statsChanged(int visibleCount, quint32 totalCount);

private slots:
    void flushPending();

private:
    void doInsertRows(int first, int last);

    // 使用 deque 作为环形缓冲区底层容器（支持 O(1) 头尾操作）
    // 当前显示的行 = m_buffer[m_offset .. m_offset + m_buffer.size() - 1]
    std::deque<LogEntry> m_buffer;
    int                  m_offset;        // 已淘汰的条目数偏移
    int                  m_maxEntries;    // 缓冲区最大容量
    std::atomic<quint32> m_totalCount;    // 全局递增计数器

    // 批量刷新
    QTimer               m_flushTimer;
    std::vector<LogEntry> m_pendingEntries;
    mutable QMutex        m_pendingMutex;
    int                   m_batchInterval;
    bool                  m_flushScheduled;
};