#ifndef FT_FUNCTIONGROUP_H
#define FT_FUNCTIONGROUP_H

#include <QListWidget>
#include <QByteArray>
#include "FT_Type.h"

class FT_Function;
class FtDropIndicator;

inline constexpr const char kFtNodeMime[]    = "application/x-ft-node";
inline constexpr const char kFtCommandMime[] = "application/x-ft-command";

struct FtGroupHit
{
    int  index = 0;
    bool valid = false;
};

struct FtCommandDragSourceInfo
{
    QWidget* group = nullptr;
    int      flat  = -1;
};
FtCommandDragSourceInfo ftCommandDragSource();
void ftSetCommandDragSource(QWidget* group, int flat);
void ftClearCommandDragSource();

class FT_FunctionGroup : public QListWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionGroup(QWidget* parent = nullptr);

    void applyLines(const FT_FunctionLines& lines);
    FT_FunctionLines toLines() const;

    int  count() const;
    FT_Function* functionAt(int index) const;
    int  indexOfFunction(FT_Function* function) const;

    FT_Function* insertFunction(const QString& typeName, int index, bool /*hardBreak*/);
    FT_Function* insertConfig(const FT_FunctionData& data, int index, bool /*hardBreak*/);
    void removeFunctionAt(int index);
    FT_Function* appendFunction(const QString& typeName);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;

    FtGroupHit hitTest(const QPoint& pos) const;

signals:
    void contentChanged();
    void structureChanged();
    void requestResize();
    void splitStepRequested(int index);
    void nodeDroppedAt(int nodeType, int index, bool hardBreak);
    void commandDroppedAt(const QByteArray& payload, int index, bool hardBreak);
    void requestInsertStepAfter();
    void requestMergeWithNext();

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    QStringList mimeTypes() const override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onCurrentRowChanged();

private:
    FT_Function* createWidget(const QString& typeName);
    void wireFunction(FT_Function* function);
    void syncItemHeight(FT_Function* f);
    void syncAllItemHeights();
    void makeSelectionExclusive();
    void scheduleNotifyResize();
    void refreshMenuStates();
    QListWidgetItem* newRowItem(FT_Function* f);
    void removeRowInternal(int row);
    void showGroupMenu(const QPoint& globalPos, const QPoint& localPos);

    // 自绘插入指示线(与 FT_FunctionList 同款控件),落点行由 m_dragOverRow 给出
    void updateDropIndicator(const QPoint& pos);
    void hideDropIndicator();

    bool m_updating = false;
    bool m_notifyPending = false;
    FtDropIndicator* m_dropIndicator = nullptr;
    int  m_dragOverRow = -1;
};

#endif