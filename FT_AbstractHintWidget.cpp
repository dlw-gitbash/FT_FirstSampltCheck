#include "FT_AbstractHintWidget.h"
#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QFontMetrics>
#include <QSizePolicy>
#include <QColor>
#include <QEvent>
#include <QRect>

FT_AbstractHintWidget::FT_AbstractHintWidget(QWidget *owner)
    : m_owner(owner)
{
    m_owner->setAttribute(Qt::WA_StyledBackground, true);
    m_owner->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_owner->setMinimumHeight(56);

    QTimer::singleShot(0, m_owner, [this] {
        m_initialized = true;
        applyMargins();
        m_owner->update();
    });
}

FT_AbstractHintWidget::FT_HintMetrics FT_AbstractHintWidget::makeHintMetrics(const QFont &baseFont)
{
    FT_HintMetrics m;
    m.font = baseFont;
    const int sz = m.font.pointSize();
    m.font.setPointSize(qMax(7, sz <= 0 ? 8 : sz - 2));
    m.leading = 3;
    const QFontMetrics fm(m.font);
    m.height  = fm.height();
    m.topArea = m.leading + m.height + 3;
    return m;
}

int FT_AbstractHintWidget::hintTopPad() const
{
    return m_hint.isEmpty() ? 6 : makeHintMetrics(m_owner->font()).topArea;
}

void FT_AbstractHintWidget::setHint(const QString &hint)
{
    if (m_hint == hint)
        return;
    m_hint = hint;
    if (m_initialized) {
        applyMargins();
        m_owner->update();
    }
}

bool FT_AbstractHintWidget::ftChangeEvent(QEvent *e)
{
    const QEvent::Type t = e->type();
    if ((t == QEvent::FontChange || t == QEvent::StyleChange) && m_initialized) {
        applyMargins();
        m_owner->update();
        return true;
    }
    return false;
}

void FT_AbstractHintWidget::ftResizeEvent()
{
    if (m_initialized)
        applyMargins();
}

void FT_AbstractHintWidget::ftPaint(QPainter &p, const QRect &area, int rightReserve)
{
    if (!m_initialized || area.width() < 40 || area.height() < 30)
        return;
    if (!m_hint.isEmpty()) {
        const FT_HintMetrics hm = makeHintMetrics(m_owner->font());
        p.setFont(hm.font);
        p.setPen(QColor(0x99, 0x99, 0x99));
        p.drawText(area.left() + kSidePad, area.top() + hm.leading,
                   area.width() - kSidePad * 2 - rightReserve, hm.height,
                   Qt::AlignLeft | Qt::AlignVCenter, m_hint);
    }
    ftPaintExtra(p, area);
}