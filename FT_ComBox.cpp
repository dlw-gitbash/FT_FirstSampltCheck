#include "FT_ComBox.h"
#include "FT_AbstractHintWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QLineEdit>
#include <QResizeEvent>
#include <QEvent>
#include <QListView>
#include <QAbstractItemView>

namespace {
constexpr int kArrowWidth = 24;
}

FT_ComBox::FT_ComBox(QWidget *parent)
    : QComboBox(parent)
    , FT_AbstractHintWidget(this)
{
    setStyleSheet(QStringLiteral(
        "FT_ComBox {"
        "   border: 1px solid #cfcfcf;"
        "   border-radius: 4px;"
        "   background-color: #ffffff;"
        "}"
        "FT_ComBox QLineEdit {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "FT_ComBox::drop-down {"
        "   subcontrol-origin: padding;"
        "   subcontrol-position: top right;"
        "   width: %1px;"
        "   background: transparent;"
        "   border: none;"
        "}"
        "FT_ComBox::down-arrow {"
        "   width: 0px; height: 0px;"
        "   image: none;"
        "}"
        "FT_ComBox QAbstractItemView {"
        "   border: 1px solid #cfcfcf;"
        "   background: #ffffff;"
        "   outline: 0px;"
        "   selection-background-color: #e6f0ff;"
        "   selection-color: #222222;"
        "}"
    ).arg(kArrowWidth));

    QMetaObject::invokeMethod(this, [this] {
        if (QAbstractItemView *vw = view()) {
            if (auto *lv = qobject_cast<QListView *>(vw)) {
                lv->setUniformItemSizes(true);
                lv->setLayoutMode(QListView::SinglePass);
                lv->setBatchSize(100);
            }
        }
    }, Qt::QueuedConnection);
}

void FT_ComBox::showPopup()
{
    if (QAbstractItemView *vw = view()) {
        const int w = qMax(width(), 120);
        vw->setMinimumWidth(w);
        if (QWidget *container = vw->parentWidget()) {
            container->setMinimumWidth(w);
            container->updateGeometry();
        }
    }
    QComboBox::showPopup();
}

void FT_ComBox::paintEvent(QPaintEvent *e)
{
    QComboBox::paintEvent(e);
    QPainter p(this);
    if (p.isActive())
        ftPaint(p, rect(), kArrowWidth + 2);
}

void FT_ComBox::resizeEvent(QResizeEvent *e)
{
    QComboBox::resizeEvent(e);
    ftResizeEvent();
}

void FT_ComBox::changeEvent(QEvent *e)
{
    QComboBox::changeEvent(e);
    ftChangeEvent(e);
}

void FT_ComBox::applyMargins()
{
    const int top   = hintTopPad();
    const int right = kSidePad + kArrowWidth;
    setContentsMargins(kSidePad, top, right, kBottomPad);
    if (QLineEdit *le = lineEdit()) {
        le->blockSignals(true);
        le->setTextMargins(kSidePad, top, kSidePad, kBottomPad);
        le->blockSignals(false);
        if (hasHint() && !le->placeholderText().isEmpty())
            le->setPlaceholderText(QString());
    }
}

void FT_ComBox::setPlaceholderText(const QString &text)
{
    if (QLineEdit *le = lineEdit())
        le->setPlaceholderText(hasHint() ? QString() : text);
}

void FT_ComBox::addItem(const QString &text)
{
    QComboBox::addItem(text);
}

void FT_ComBox::addItems(const QStringList &texts)
{
    QComboBox::addItems(texts);
}

void FT_ComBox::ftPaintExtra(QPainter &p, const QRect &area)
{
    (void)area;
    const qreal cx = width()  - kArrowWidth / 2.0;
    const qreal cy = height() / 2.0;
    constexpr qreal halfW = 4.0;
    constexpr qreal triH  = 5.0;
    const QColor triColor = (underMouse() || (view() && view()->isVisible()))
                                ? QColor(0x2f, 0x80, 0xed)
                                : QColor(0x7a, 0x7a, 0x7a);
    p.setBrush(triColor);
    p.setPen(Qt::NoPen);
    QPainterPath tri;
    tri.moveTo(cx - halfW, cy - triH / 2.0);
    tri.lineTo(cx + halfW, cy - triH / 2.0);
    tri.lineTo(cx,         cy + triH / 2.0);
    tri.closeSubpath();
    p.drawPath(tri);
}