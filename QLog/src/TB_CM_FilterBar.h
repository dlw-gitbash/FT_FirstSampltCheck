#pragma once

#include <TB_CM_Model.h>
#include <TB_CM_Theme.h>

#include <QWidget>

class QLabel;
class QLineEdit;
class QTimer;
class QToolButton;

class TB_CM_FilterBar final : public QWidget
{
    Q_OBJECT

public:
    explicit TB_CM_FilterBar(QWidget* parent = nullptr);

    void setTheme(const TB_CM_Theme& theme);
    void setFollowTail(bool enabled);
    void setCounts(int visible, int total);

    TB_CM_Filter filter() const;

signals:
    void filterChanged();
    void wrapChanged(bool enabled);
    void followChanged(bool enabled);
    void scrollToBottomRequested();
    void clearRequested();

private:
    void onSearchTextChanged(const QString& text);
    void onFilterOptionToggled();
    void updateStyles();

private:
    TB_CM_Theme theme_ = TB_CM_Theme::dark();
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
