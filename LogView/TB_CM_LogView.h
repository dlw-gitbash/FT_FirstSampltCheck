#pragma once

#include <QWidget>
#include <QString>
#include <QMutex>
#include <QVector>
#include <QTextCharFormat>
#include <QMetaType>
#include <atomic>

#include "TB_CM_Log.h"

class QPlainTextEdit;
class QPushButton;
class QLineEdit;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class QMenu;
class QAction;
class TB_CM_LogSearch;
class TB_CM_LogContextMenu;

enum E_OverflowPolicy
{
    OP_RING_BUFFER,
    OP_CLEAR_ALL
};

struct TB_CM_LogEntry
{
    QString message;
    quint64 timestampMs = 0;
    int level = L_INFO;
};

Q_DECLARE_METATYPE(TB_CM_LogEntry)
Q_DECLARE_METATYPE(E_OverflowPolicy)

class TB_CM_LogView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int maxLogs READ maxLogs WRITE setMaxLogs)
    Q_PROPERTY(E_OverflowPolicy overflowPolicy READ overflowPolicy WRITE setOverflowPolicy)
    Q_PROPERTY(QString filterKeyword READ filterKeyword WRITE setFilterKeyword)
public:
    static constexpr int DEFAULT_MAX_LOGS = 10000;
    static constexpr int DEFAULT_FLUSH_INTERVAL_MS = 100;
    static constexpr int DEFAULT_PENDING_MAX = 20000;
    explicit TB_CM_LogView(QWidget *parent = nullptr);
    ~TB_CM_LogView() override;
    Q_INVOKABLE void appendLog(const QString &message, quint64 timestampMs, int level = L_INFO);
    Q_INVOKABLE void appendLog(const TB_CM_LogEntry &entry);
    Q_INVOKABLE void setMaxLogs(int maxLogs);
    Q_INVOKABLE int maxLogs() const;
    Q_INVOKABLE void setOverflowPolicy(E_OverflowPolicy policy);
    Q_INVOKABLE E_OverflowPolicy overflowPolicy() const;
    Q_INVOKABLE void setFilterKeyword(const QString &keyword);
    Q_INVOKABLE QString filterKeyword() const;
    Q_INVOKABLE void search(const QString &keyword);
    Q_INVOKABLE void findNext();
    Q_INVOKABLE void findPrev();
    Q_INVOKABLE QString searchKeyword() const;
    Q_INVOKABLE int searchHitCount() const;
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE qint64 droppedCount() const;
    Q_INVOKABLE void resetDroppedCount();
    Q_INVOKABLE void setFlushInterval(int ms);
    Q_INVOKABLE int flushInterval() const;
    Q_INVOKABLE QPlainTextEdit *textEdit() const;
    static QString levelToString(int level);
    static QString nowTimestamp();
signals:
    void logAppended(int count);
    void logsCleared();
    void searchHitsChanged(int count);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    struct LogRecord
    {
        QString line;
        int level = L_INFO;
    };
    void initView();
    void initFilterBar();
    void initContextMenu();
    void initConnections();
    void showContextMenu(const QPoint &globalPos);
    void onCopyAction();
    void onSelectAllAction();
    void startFlushTimer();
    void flushPending();
    void applyPending(const QVector<LogRecord> &batch);
    void insertRecords(const QVector<LogRecord> &records, int from, int to);
    void rebuildDocument();
    void applyFilterAndSearch(const QString &keyword);
    void onSearchReturnPressed();
    void scrollToBottom();
    void onScrollValueChanged(int value);
    void setMaxLogsImpl(int maxLogs);
    QTextCharFormat levelFormat(int level) const;

    QPlainTextEdit *m_edit;
    QWidget *m_filterBar;
    QHBoxLayout *m_filterBarLayout;
    QLineEdit *m_filterEdit;
    QPushButton *m_floatNewLogBtn;
    QPushButton *m_clearLogsBtn;
    QVBoxLayout *m_mainLayout;
    QTimer *m_flushTimer;
    QTimer *m_searchDebounceTimer;
    TB_CM_LogContextMenu *m_contextMenu;
    TB_CM_LogSearch *m_search;

    QTextCharFormat m_levelFormats[6];

    mutable QMutex m_mutex;
    QVector<LogRecord> m_pending;
    QVector<LogRecord> m_records;

    QString m_filterKeyword;

    int m_maxLogs;
    int m_flushIntervalMs;
    E_OverflowPolicy m_overflowPolicy;
    std::atomic<qint64> m_droppedCount;
    std::atomic<bool> m_flushScheduled;

    bool m_autoScroll;
    bool m_flushNow;
};
