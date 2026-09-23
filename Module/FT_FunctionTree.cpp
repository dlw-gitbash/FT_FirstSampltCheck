#include "FT_FunctionTree.h"
#include <QMimeData>

static const QString kFtNodeMime = QStringLiteral("application/x-ft-node");

FT_FunctionTree::FT_FunctionTree(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setFrameShape(QFrame::NoFrame);
    setMinimumWidth(180);
    // 保留根节点的展开/折叠箭头（双击已用于“添加命令”，不承担展开职责）
    setRootIsDecorated(true);
    setIndentation(16);
    setExpandsOnDoubleClick(false);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);
    setStyleSheet(QStringLiteral("QTreeWidget { outline: 0; }"));
}

QMimeData* FT_FunctionTree::mimeData(const QList<QTreeWidgetItem*>& items) const
{
    auto* mime = new QMimeData();
    if (items.isEmpty())
        return mime;
    const int nodeType = items.first()->data(0, Qt::UserRole).toInt();
    mime->setData(kFtNodeMime, QByteArray::number(nodeType));
    return mime;
}

QStringList FT_FunctionTree::mimeTypes() const
{
    return {kFtNodeMime};
}

Qt::DropActions FT_FunctionTree::supportedDropActions() const
{
    return Qt::CopyAction;
}