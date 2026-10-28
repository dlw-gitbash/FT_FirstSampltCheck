#pragma once

#include <TB_CM_Types.h>

#include <QListView>

class TB_CM_Model;
class TB_CM_Delegate;

class TB_CM_View final : public QListView
{
    Q_OBJECT

public:
    explicit TB_CM_View(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* model) override;
    void setTheme(const Theme& theme);
    void setWrapEnabled(bool enabled);
    void setFollowTail(bool enabled);
    void copySelectionToClipboard();
    void copyAllToClipboard();

    TB_CM_Model* logModel() const { return model_; }
    TB_CM_Delegate* logDelegate() const { return delegate_; }
    const Theme& theme() const { return theme_; }
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
    QString lineFor(const Record& record) const;

private:
    TB_CM_Model* model_ = nullptr;
    TB_CM_Delegate* delegate_ = nullptr;
    Theme theme_ = Theme::dark();
    bool wrapEnabled_ = true;
    bool followTail_ = true;
    bool applyingFollow_ = false;
};
