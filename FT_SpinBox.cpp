#include "FT_SpinBox.h"
#include <QPainter>
#include <QLineEdit>
#include <QResizeEvent>
#include <QEvent>

FT_SpinBox::FT_SpinBox(QWidget *parent)
    : QSpinBox(parent)
    , FT_AbstractHintWidget(this)
{
    setStyleSheet(QStringLiteral(
        "FT_SpinBox {"
        "   border: 1px solid #cfcfcf;"
        "   border-radius: 4px;"
        "   background-color: #ffffff;"
        "}"
        "FT_SpinBox::up-button   { width: %1px; border-left: 1px solid #eaeaea; }"
        "FT_SpinBox::down-button { width: %1px; border-left: 1px solid #eaeaea; }"
    ).arg(kBtnWidth));
}

void FT_SpinBox::paintEvent(QPaintEvent *e)
{
    QSpinBox::paintEvent(e);
    QPainter p(this);
    if (p.isActive())
        ftPaint(p, rect(), kBtnWidth + 2);
}

void FT_SpinBox::resizeEvent(QResizeEvent *e)
{
    QSpinBox::resizeEvent(e);
    ftResizeEvent();
}

void FT_SpinBox::changeEvent(QEvent *e)
{
    QSpinBox::changeEvent(e);
    ftChangeEvent(e);
}

void FT_SpinBox::applyMargins()
{
    const int top   = hintTopPad();
    const int right = kSidePad + kBtnWidth;
    setContentsMargins(kSidePad, top, right, kBottomPad);
    if (QLineEdit *le = lineEdit()) {
        le->blockSignals(true);
        le->setTextMargins(kSidePad, top, kSidePad, kBottomPad);
        le->blockSignals(false);
        if (hasHint() && !le->placeholderText().isEmpty())
            le->setPlaceholderText(QString());
    }
}