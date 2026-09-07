#ifndef FT_EDIT_H
#define FT_EDIT_H

#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include "FT_FunctionConfig.h"

class QTreeWidgetItem;
class QLabel;
class QStatusBar;
class QCloseEvent;
class FT_FunctionCard;
class FtDropIndicator;

class FT_FunctionTree : public QTreeWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionTree(QWidget* parent = nullptr);

protected:
    QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
};

class FT_FunctionList : public QListWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionList(QWidget* parent = nullptr);

signals:
    void treeNodeDropped(int nodeType, int atRow);
    void cardMoved(int fromRow, int toRow);
    void emptyAreaDoubleClicked();
    void cardEnterPressed(int row);

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

class FT_Edit : public QWidget
{
    Q_OBJECT
public:
    explicit FT_Edit(QWidget* parent = nullptr);

    FT_FunctionCard* addCard(int atRow = -1);
    void addTboxRow();

private slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onTreeNodeDropped(int nodeType, int atRow);
    void onCardMoved(int fromRow, int toRow);
    void onEmptyAreaDoubleClicked();
    void onCardEnterPressed(int row);

private:
    enum NodeType
    {
        NodeFunctions   = 1,
        NodeTboxCommand = 2
    };

    void buildButtonBar();
    void buildTree();
    void buildCardList();

    FT_FunctionCard* currentCard() const;
    FT_FunctionCard* cardAtRow(int row) const;

    void removeCard(FT_FunctionCard* card);
    void deleteCurrentCard();
    void moveCurrentCard(int delta);

    bool exportConfig();
    void importConfig();
    void clearAllCards();

    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void updateEmptyHint();

    void markDirty();
    void markClean();
    bool confirmDiscardIfDirty(const QString& actionTitle);
    bool payloadHexOk(const FT_FunctionDocument& doc, QString* errorDetail) const;

    QWidget*         m_buttonBar     = nullptr;
    FT_FunctionTree* m_funcTree      = nullptr;
    FT_FunctionList* m_funcList      = nullptr;
    QLabel*          m_emptyHint     = nullptr;
    QStatusBar*      m_statusBar     = nullptr;
    bool             m_dirty         = false;
    bool             m_suppressDirty = false;
};

#endif // FT_EDIT_H