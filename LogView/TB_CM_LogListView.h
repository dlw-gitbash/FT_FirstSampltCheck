#pragma once

#include <QListView>

struct TB_CM_Record;
class TB_CM_LogModel;
class TB_CM_LogDelegate;

class TB_CM_LogListView final : public QListView
{
    Q_OBJECT

public:
    explicit TB_CM_LogListView(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* model) override;
    void setWrapEnabled(bool enabled);
    void setFollowTail(bool enabled);

    bool isWrapEnabled() const { return wrapEnabled_; }
    bool isFollowTail() const { return followTail_; }

signals:
    void followTailChanged(bool enabled);
    void clearRequested();

protected:
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
    TB_CM_LogModel* model_ = nullptr;
    TB_CM_LogDelegate* delegate_ = nullptr;
    bool wrapEnabled_ = true;
    bool followTail_ = true;
    bool applyingFollow_ = false;
};
