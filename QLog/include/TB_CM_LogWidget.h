#pragma once

#include <TB_CM_Router.h>
#include <TB_CM_Types.h>

#include <QTimer>
#include <QWidget>

#include <vector>

class TB_CM_FilterBar;
class TB_CM_Model;
class TB_CM_View;

class TB_CM_LogWidget final : public QWidget, public TB_CM_Router::Sink
{
    Q_OBJECT

public:
    explicit TB_CM_LogWidget(QWidget* parent = nullptr);
    ~TB_CM_LogWidget() override;

    void append(Level level, QStringView category, QString message);
    void append(const Record& record);
    void appendBatch(std::vector<Record> records);
    void clear();
    void flushPending();

    void setCapacity(int capacity);
    void setTheme(const Theme& theme);
    void setWrapEnabled(bool enabled);
    void setFollowTail(bool enabled);
    void setFlushInterval(int milliseconds);
    void scrollToBottom();
    void attachToRouter();
    void detachFromRouter();

    int capacity() const;
    const Theme& theme() const { return theme_; }
    bool isWrapEnabled() const;
    bool isFollowTail() const;
    int flushInterval() const { return flushTimer_.interval(); }
    bool isAttachedToRouter() const { return attached_; }
    TB_CM_Model* model() const { return model_; }
    TB_CM_View* view() const { return view_; }
    TB_CM_FilterBar* filterBar() const { return filterBar_; }

    void onLogBatch(std::vector<Record>& batch) override;

signals:
    void countsChanged(int visible, int total);
    void followTailChanged(bool enabled);

private:
    void enqueue(std::vector<Record> records);
    void trimPending();
    void onCountsChanged(int visible, int total);
    void onFilterBarChanged();
    void onFilterBarWrapChanged(bool enabled);
    void onFilterBarFollowChanged(bool enabled);
    void onViewFollowTailChanged(bool enabled);
    void onScrollToBottomRequested();

private:
    TB_CM_Model* model_ = nullptr;
    TB_CM_View* view_ = nullptr;
    TB_CM_FilterBar* filterBar_ = nullptr;
    Theme theme_ = Theme::dark();
    QTimer flushTimer_;
    std::vector<Record> pending_;
    bool attached_ = false;
};
