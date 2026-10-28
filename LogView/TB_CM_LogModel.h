#pragma once

#include <TB_CM_Log.h>

#include <QAbstractListModel>
#include <QMutex>
#include <QRegularExpression>
#include <QString>
#include <QStringView>
#include <QTimer>

#include <vector>

struct TB_CM_LogEntry {
    QString message;
    QString category;
    quint64 timestampMs = 0;
    int level = L_INFO;
};

inline constexpr int TB_CM_LevelCount = L_CRITICAL + 1;

inline QStringView levelName(int level) noexcept
{
    switch (level) {
    case L_TRACE:
        return u"TRACE";
    case L_DEBUG:
        return u"DEBUG";
    case L_WARN:
        return u"WARN";
    case L_ERROR:
        return u"ERROR";
    case L_CRITICAL:
        return u"CRITICAL";
    default:
        return u"INFO";
    }
}

struct TB_CM_Record {
    QString message;
    QString category;
    mutable QString timeText;
    quint64 timestampMs = 0;
    qint64 sequence = 0;
    int level = L_INFO;
    mutable int cachedWidth = -1;
    mutable int cachedHeight = -1;
};

enum class TB_CM_MatchMode : quint8 {
    Substring,
    Regex,
};

struct TB_CM_Filter {
    QString text;
    TB_CM_MatchMode mode = TB_CM_MatchMode::Substring;
    bool caseSensitive = false;
    bool searchCategory = true;
};

// 定长环形缓冲 + 16ms 批量提交：appendLog 可在任意线程调用，仅 GUI 线程访问视图数据
class TB_CM_LogModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        LevelRole = Qt::UserRole + 1,
        MessageRole,
        CategoryRole,
        TimestampRole,
    };

    explicit TB_CM_LogModel(QObject* parent = nullptr);

    void appendLog(const QString& message, quint64 timestampMs, int level);
    void appendLog(const TB_CM_LogEntry& entry);
    void clear();
    void setFilter(const TB_CM_Filter& filter);
    void invalidateLayoutCache();

    int capacity() const;
    int totalCount() const;
    int visibleCount() const;
    const TB_CM_Filter& filter() const { return filter_; }

    const TB_CM_Record* recordAt(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

signals:
    void countsChanged(int visible, int total);
    void rowsAppended(int firstRow, int lastRow);
    void filterChanged();

private:
    void scheduleFlush();
    void flushPending();
    void trimPending();
    void commit(std::vector<TB_CM_LogEntry>&& entries);
    bool matches(const TB_CM_Record& record) const;
    void compileFilter();
    void rebuildVisible();

private:
    std::vector<TB_CM_Record> slots_;
    qint64 firstSeq_ = 0;
    qint64 nextSeq_ = 0;
    std::vector<qint64> visible_;
    TB_CM_Filter filter_;
    QRegularExpression regex_;
    bool regexValid_ = true;
    mutable QMutex mutex_;
    std::vector<TB_CM_LogEntry> pending_;
    QTimer flushTimer_;
};
