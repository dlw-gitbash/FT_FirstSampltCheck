#pragma once

#include <TB_CM_LogModel.h>

#include <QWidget>

class QLabel;
class QLineEdit;
class QTimer;
class QToolButton;
class TB_CM_LogListView;

class TB_CM_LogView final : public QWidget
{
    Q_OBJECT

public:
    explicit TB_CM_LogView(QWidget* parent = nullptr);
    ~TB_CM_LogView() override;

    Q_INVOKABLE void appendLog(const QString& message, quint64 timestampMs, int level = L_INFO);
    Q_INVOKABLE void appendLog(const TB_CM_LogEntry& entry);

    void clear();
    void scrollToBottom();

signals:
    void countsChanged(int visible, int total);

private:
    void initFilterBar();
    void initConnections();
    void updateStyles();
    void onFilterChanged();
    void onFilterOptionToggled();
    void onSearchTextChanged(const QString& text);
    void onWrapChanged(bool enabled);
    void onFollowChanged(bool enabled);
    void onViewFollowTailChanged(bool enabled);
    void onCountsChanged(int visible, int total);

private:
    TB_CM_LogModel* model_ = nullptr;
    TB_CM_LogListView* listView_ = nullptr;
    QLineEdit* search_ = nullptr;
    QToolButton* regexButton_ = nullptr;
    QToolButton* caseButton_ = nullptr;
    QToolButton* wrapButton_ = nullptr;
    QToolButton* followButton_ = nullptr;
    QToolButton* bottomButton_ = nullptr;
    QToolButton* clearButton_ = nullptr;
    QLabel* counter_ = nullptr;
    QTimer* debounce_ = nullptr;
};
