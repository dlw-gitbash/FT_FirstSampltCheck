#include "FT_LineEdit.h"
#include <QPainter>
#include <QResizeEvent>
#include <QEvent>

FT_LineEdit::FT_LineEdit(QWidget *parent)
    : QLineEdit(parent)
    , FT_AbstractHintWidget(this)
{
    setStyleSheet(QStringLiteral(
        "FT_LineEdit {"
        "   border: 1px solid #cfcfcf;"
        "   border-radius: 4px;"
        "   background-color: #ffffff;"
        "   padding: 0px;"
        "}"));
}

void FT_LineEdit::paintEvent(QPaintEvent *e)
{
    QLineEdit::paintEvent(e);
    QPainter p(this);
    if (p.isActive())
        ftPaint(p, rect());
}

void FT_LineEdit::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);
    ftResizeEvent();
}

void FT_LineEdit::changeEvent(QEvent *e)
{
    QLineEdit::changeEvent(e);
    ftChangeEvent(e);
}

void FT_LineEdit::applyMargins()
{
    setTextMargins(kSidePad, hintTopPad(), kSidePad, kBottomPad);
    if (hasHint() && !placeholderText().isEmpty())
        setPlaceholderText(QString());
}