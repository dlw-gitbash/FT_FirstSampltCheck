#pragma once

#include <TB_CM_Types.h>

#include <QMutex>
#include <QTimer>
#include <QWidget>

#include <vector>

class TB_CM_FilterBar;
class TB_CM_Model;
class TB_CM_View;

class TB_CM_LogWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit TB_CM_LogWidget(QWidget* parent = nullptr);
    ~TB_CM_LogWidget() override;

    Q_INVOKABLE void appendLog(const QString& message, quint64 timestampMs, int level = L_INFO);
    Q_INVOKABLE void appendLog(const TB_CM_LogEntry& entry);

    void clear();
    void scrollToBottom();

signals:
    void countsChanged(int visible, int total);

private:
    void scheduleFlush();
    void flushPending();
    void onCountsChanged(int visible, int total);
    void onFilterChanged();
    void onWrapChanged(bool enabled);
    void onFollowChanged(bool enabled);
    void onViewFollowTailChanged(bool enabled);

private:
    TB_CM_Model* model_ = nullptr;
    TB_CM_View* view_ = nullptr;
    TB_CM_FilterBar* filterBar_ = nullptr;
    QTimer flushTimer_;
    mutable QMutex mutex_;
    std::vector<TB_CM_LogEntry> pending_;
    int capacity_ = 20000;
};
