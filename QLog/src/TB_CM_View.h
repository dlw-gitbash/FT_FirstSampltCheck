#pragma once

#include <TB_CM_Theme.h>

#include <QListView>

struct TB_CM_Record;
class TB_CM_Model;
class TB_CM_Delegate;

class TB_CM_View final : public QListView
{
    Q_OBJECT

public:
    explicit TB_CM_View(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* model) override;
    void setTheme(const TB_CM_Theme& theme);
    void setWrapEnabled(bool enabled);
    void setFollowTail(bool enabled);

    bool isWrapEnabled() const { return wrapEnabled_; }
    bool isFollowTail() const { return followTail_; }

signals:
    void followTailChanged(bool enabled);
    void clearRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void onRowsAppended(int firstRow, int lastRow);
    void onModelReset();
    void onFilterChanged();
    void onScrollBarRangeChanged(int minimum, int maximum);
    void onScrollBarValueChanged(int value);
    void copySelectionToClipboard();
    void copyAllToClipboard();
    QString lineFor(const TB_CM_Record& record) const;

private:
    TB_CM_Model* model_ = nullptr;
    TB_CM_Delegate* delegate_ = nullptr;
    TB_CM_Theme theme_ = TB_CM_Theme::dark();
    bool wrapEnabled_ = true;
    bool followTail_ = true;
    bool applyingFollow_ = false;
};
