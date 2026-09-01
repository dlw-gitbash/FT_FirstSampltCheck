#include "FT_TextEdit.h"
#include <QPainter>
#include <QFrame>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QEvent>

FT_TextEdit::FT_TextEdit(QWidget *parent)
    : QTextEdit(parent)
    , FT_AbstractHintWidget(this)
    , m_verticalCenter(false)
    , m_inUpdateVC(0)
{
    setStyleSheet(QStringLiteral(
        "FT_TextEdit {"
        "   border: 1px solid #cfcfcf;"
        "   border-radius: 4px;"
        "   background-color: #ffffff;"
        "   padding: 0px;"
        "}"));
    setFrameStyle(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(this, &QTextEdit::textChanged, this, [this] {
        if (hintReady())
            updateVerticalCenter();
    });
}

void FT_TextEdit::setText(const QString &text)
{
    QTextEdit::setPlainText(text);
}

void FT_TextEdit::paintEvent(QPaintEvent *e)
{
    QTextEdit::paintEvent(e);
    QPainter p(viewport());
    if (p.isActive())
        ftPaint(p, viewport()->rect());
}

void FT_TextEdit::resizeEvent(QResizeEvent *e)
{
    QTextEdit::resizeEvent(e);
    ftResizeEvent();
}

void FT_TextEdit::changeEvent(QEvent *e)
{
    QTextEdit::changeEvent(e);
    ftChangeEvent(e);
}

void FT_TextEdit::applyMargins()
{
    QTextDocument *doc  = document();
    QTextFrame    *root = doc ? doc->rootFrame() : nullptr;
    if (!root)
        return;
    QTextFrameFormat fmt = root->frameFormat();
    fmt.setLeftMargin(kSidePad);
    fmt.setRightMargin(kSidePad);
    fmt.setBottomMargin(kBottomPad);
    const int baseTop = hintTopPad();
    if (!m_verticalCenter) {
        if (!qFuzzyCompare(fmt.topMargin(), static_cast<qreal>(baseTop))) {
            fmt.setTopMargin(baseTop);
            root->setFrameFormat(fmt);
        }
    }
    if (hasHint() && !QTextEdit::placeholderText().isEmpty())
        QTextEdit::setPlaceholderText(QString());
    if (m_verticalCenter)
        updateVerticalCenter();
}

void FT_TextEdit::setAlignment(Qt::Alignment align)
{
    QTextEdit::setAlignment(align);
    m_verticalCenter = (align & Qt::AlignVCenter) != 0;
    if (!m_verticalCenter) {
        if (QTextDocument *doc = document()) {
            if (QTextFrame *root = doc->rootFrame()) {
                QTextFrameFormat fmt = root->frameFormat();
                if (!qFuzzyCompare(fmt.topMargin(), static_cast<qreal>(0))) {
                    fmt.setTopMargin(0);
                    root->setFrameFormat(fmt);
                }
            }
        }
    }
    if (hintReady()) {
        applyMargins();
        update();
    }
}

void FT_TextEdit::updateVerticalCenter()
{
    if (!m_verticalCenter || !hintReady() || m_inUpdateVC > 0)
        return;
    QTextDocument *doc  = document();
    QTextFrame    *root = doc ? doc->rootFrame() : nullptr;
    if (!root || !viewport())
        return;
    ++m_inUpdateVC;
    QTextFrameFormat fmt = root->frameFormat();
    const qreal currentBottom = fmt.bottomMargin();
    const int   baseTop       = hintTopPad();
    const int contentHeight = [&] {
        QAbstractTextDocumentLayout *layout = doc->documentLayout();
        if (!layout) return 0;
        const qreal total = layout->documentSize().height() - currentBottom;
        return qMax(0, static_cast<int>(total - static_cast<qreal>(baseTop)));
    }();
    const int availH   = qMax(1, viewport()->height() - baseTop - static_cast<int>(currentBottom));
    const int extraTop = qMax(0, (availH - contentHeight) / 2);
    const int newTop   = baseTop + extraTop;
    if (!qFuzzyCompare(fmt.topMargin(), static_cast<qreal>(newTop))) {
        fmt.setTopMargin(newTop);
        root->setFrameFormat(fmt);
    }
    --m_inUpdateVC;
}