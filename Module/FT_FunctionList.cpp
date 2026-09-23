#include "FT_FunctionList.h"

#include <QLabel>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QDrag>
#include <QPixmap>

static const QString kFtNodeMime = QStringLiteral("application/x-ft-node");

FtDropIndicator::FtDropIndicator(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedHeight(10);
    hide();
}

void FtDropIndicator::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor color(74, 144, 217);
    QPen pen(color, 2);
    p.setPen(pen);
    p.setBrush(color);

    const int midY = height() / 2;
    const int left = 6;
    const int right = width() - 6;

    p.drawLine(left, midY, right, midY);

    QPainterPath leftArrow;
    leftArrow.moveTo(left, midY);
    leftArrow.lineTo(left + 8, midY - 5);
    leftArrow.lineTo(left + 8, midY + 5);
    leftArrow.closeSubpath();
    p.drawPath(leftArrow);

    QPainterPath rightArrow;
    rightArrow.moveTo(right, midY);
    rightArrow.lineTo(right - 8, midY - 5);
    rightArrow.lineTo(right - 8, midY + 5);
    rightArrow.closeSubpath();
    p.drawPath(rightArrow);
}

FT_FunctionList::FT_FunctionList(QWidget* parent)
    : QListWidget(parent)
{
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSpacing(2);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(false);

    m_dropIndicator = new FtDropIndicator(viewport());
    m_dropIndicator->raise();

    m_dragPreview = new QLabel(viewport());
    m_dragPreview->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dragPreview->hide();
}

void FT_FunctionList::updateDragPreview(const QPoint& pos)
{
    if (!m_dragPreview || m_dragPreview->pixmap().isNull())
        return;

    const QSize ps = m_dragPreview->pixmap().size();
    const int vw = viewport()->width();
    const int vh = viewport()->height();

    int x = pos.x() - ps.width() / 2;
    int y = pos.y() - ps.height() / 2;

    x = qBound(0, x, qMax(0, vw - ps.width()));
    y = qBound(0, y, qMax(0, vh - ps.height()));

    m_dragPreview->move(x, y);
    m_dragPreview->raise();
    m_dragPreview->show();
}

void FT_FunctionList::hideDragPreview()
{
    if (m_dragPreview)
        m_dragPreview->hide();
}

void FT_FunctionList::updateDropIndicator(const QPoint& pos)
{
    if (!m_dropIndicator)
        return;

    const int vw = viewport()->width();

    if (count() == 0) {
        m_dropIndicator->setGeometry(0, 0, vw, m_dropIndicator->height());
        m_dropIndicator->raise();
        m_dropIndicator->show();
        m_dragOverRow = 0;
        return;
    }

    QListWidgetItem* at = itemAt(pos);
    if (!at) {
        int lastRow = count() - 1;
        QRect r = visualItemRect(item(lastRow));
        int y = r.bottom() + 1;
        m_dropIndicator->setGeometry(0, y, vw, m_dropIndicator->height());
        m_dropIndicator->raise();
        m_dropIndicator->show();
        m_dragOverRow = count();
        return;
    }

    QRect r = visualItemRect(at);
    const int rowMid = r.center().y();
    const int insertAbove = (pos.y() < rowMid) ? 1 : 0;
    int y = insertAbove ? r.top() - m_dropIndicator->height() / 2
                        : r.bottom() + 1 - m_dropIndicator->height() / 2;

    m_dropIndicator->setGeometry(0, y, vw, m_dropIndicator->height());
    m_dropIndicator->raise();
    m_dropIndicator->show();
    m_dragOverRow = row(at) + (insertAbove ? 0 : 1);
}

void FT_FunctionList::hideDropIndicator()
{
    if (m_dropIndicator)
        m_dropIndicator->hide();
    m_dragOverRow = -1;
}

void FT_FunctionList::startDrag(Qt::DropActions supportedActions)
{
    m_dragSourceRow = currentRow();

    QList<QListWidgetItem*> items = selectedItems();
    if (items.isEmpty()) {
        m_dragSourceRow = -1;
        return;
    }

    QWidget* w = itemWidget(items.first());
    if (w && m_dragPreview) {
        QPixmap pix = w->grab();
        m_dragPreview->setPixmap(pix);
        m_dragPreview->resize(pix.size());
    }

    QMimeData* mime = mimeData(items);
    if (!mime) {
        m_dragSourceRow = -1;
        return;
    }

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mime);
    QPixmap tpix(1, 1);
    tpix.fill(Qt::transparent);
    drag->setPixmap(tpix);
    drag->exec(supportedActions, Qt::MoveAction);
    delete drag;

    m_dragSourceRow = -1;
    hideDragPreview();
}

void FT_FunctionList::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* md = event->mimeData();
    const QPoint pos = event->position().toPoint();
    if (md->hasFormat(kFtNodeMime)) {
        updateDropIndicator(pos);
        event->acceptProposedAction();
        return;
    }
    updateDropIndicator(pos);
    updateDragPreview(pos);
    QListWidget::dragEnterEvent(event);
}

void FT_FunctionList::dragMoveEvent(QDragMoveEvent* event)
{
    const QPoint pos = event->position().toPoint();
    updateDropIndicator(pos);
    updateDragPreview(pos);
    QListWidget::dragMoveEvent(event);
}

void FT_FunctionList::dragLeaveEvent(QDragLeaveEvent* event)
{
    hideDropIndicator();
    hideDragPreview();
    QListWidget::dragLeaveEvent(event);
}

void FT_FunctionList::dropEvent(QDropEvent* event)
{
    const int toRow = m_dragOverRow;
    hideDropIndicator();
    hideDragPreview();

    const QMimeData* md = event->mimeData();

    if (md->hasFormat(kFtNodeMime)) {
        const int nodeType = md->data(kFtNodeMime).toInt();
        emit treeNodeDropped(nodeType, toRow);
        event->acceptProposedAction();
        m_dragSourceRow = -1;
        return;
    }

    const int fromRow = m_dragSourceRow;
    m_dragSourceRow = -1;

    if (fromRow >= 0 && fromRow < count()) {
        event->acceptProposedAction();
        emit internalReorderRequested(fromRow, toRow);
        return;
    }

    event->ignore();
}

void FT_FunctionList::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QListWidgetItem* at = itemAt(event->position().toPoint());
        if (!at) {
            emit emptyAreaDoubleClicked();
            event->accept();
            return;
        }
    }
    QListWidget::mouseDoubleClickEvent(event);
}

void FT_FunctionList::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        const int r = currentRow();
        if (r >= 0) {
            emit itemEnterPressed(r);
            event->accept();
            return;
        }
    }
    QListWidget::keyPressEvent(event);
}