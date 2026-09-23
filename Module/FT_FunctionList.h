#ifndef FT_FUNCTIONLIST_H
#define FT_FUNCTIONLIST_H

#include <QListWidget>
#include <QByteArray>

class QLabel;

class FtDropIndicator : public QWidget
{
public:
    explicit FtDropIndicator(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
};

class FT_FunctionList : public QListWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionList(QWidget* parent = nullptr);

signals:
    void treeNodeDropped(int nodeType, int atIndex);
    void commandDroppedToGap(const QByteArray& payload, int atIndex);
    void itemMoved(int fromIndex, int toIndex);
    void internalReorderRequested(int from, int to);
    void emptyAreaDoubleClicked();
    void itemEnterPressed(int index);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateDropIndicator(const QPoint& pos);
    void hideDropIndicator();
    void updateDragPreview(const QPoint& pos);
    void hideDragPreview();

    FtDropIndicator* m_dropIndicator  = nullptr;
    QLabel*          m_dragPreview    = nullptr;
    int              m_dragOverRow    = -1;
    int              m_dragSourceRow  = -1;
};

#endif // FT_FUNCTIONLIST_H