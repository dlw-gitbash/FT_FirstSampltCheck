#pragma once

#include <TB_CM_Types.h>

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

    void setTheme(const Theme& theme);
    void setFilterText(const QString& text);
    void setWrapEnabled(bool enabled);
    void setFollowTail(bool enabled);
    void setCounts(int visible, int total);

    Filter filter() const;
    const Theme& theme() const { return theme_; }
    QString filterText() const;
    bool isWrapEnabled() const;
    bool isFollowTail() const;

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
    Theme theme_ = Theme::dark();
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
