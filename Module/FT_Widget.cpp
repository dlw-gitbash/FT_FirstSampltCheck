#include "FT_Widget.h"
#include "FT_Log.h"

#include <QLabel>
#include <QLineEdit>
#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QTextDocument>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFont>
#include <QPalette>
#include <QColor>

namespace {

QLabel* makeTitleLabel(QWidget* host, const QString& title)
{
    auto* label = new QLabel(host);
    label->setObjectName(QStringLiteral("FHintTitle"));
    label->setText(title);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QFont f = host->font();
    if (f.pointSizeF() > 0)
        f.setPointSizeF(qMax(7.0, f.pointSizeF() - 1.5));
    else
        f.setPixelSize(qMax(9, f.pixelSize() - 2));
    label->setFont(f);

    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, QColor(0x8A, 0x8A, 0x8A));
    label->setPalette(pal);

    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setVisible(!title.isEmpty());
    label->raise();
    return label;
}

void placeTitleLabel(QLabel* label, int hostWidth, int rightReserve = 0)
{
    if (!label)
        return;
    label->setGeometry(kTitleLeft, kFHintTitleTop,
                       qMax(0, hostWidth - 2 * kTitleLeft - rightReserve),
                       kFHintTitleHeight);
}

} // anonymous namespace

FHintComboBox::FHintComboBox(const QString& title, QWidget* parent)
    : QComboBox(parent)
    , m_titleLabel(makeTitleLabel(this, title))
{
    setMinimumHeight(kFHintMinHeight);

    setEditable(true);
    QLineEdit* le = lineEdit();
    le->setReadOnly(true);
    le->setTextMargins(6, kFHintSingleTopPadding, kFHintComboArrowReserve, 0);
    le->installEventFilter(this);

    placeTitleLabel(m_titleLabel, width(), kFHintComboArrowReserve);
}

void FHintComboBox::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
    m_titleLabel->setVisible(!title.isEmpty());
}

QString FHintComboBox::title() const
{
    return m_titleLabel->text();
}

bool FHintComboBox::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == lineEdit() && event->type() == QEvent::MouseButtonPress) {
        showPopup();
        return true;
    }
    return QComboBox::eventFilter(watched, event);
}

void FHintComboBox::wheelEvent(QWheelEvent* event)
{
    if (!hasFocus() && !(lineEdit() && lineEdit()->hasFocus())) {
        event->ignore();
        return;
    }
    QComboBox::wheelEvent(event);
}

void FHintComboBox::resizeEvent(QResizeEvent* event)
{
    QComboBox::resizeEvent(event);
    placeTitleLabel(m_titleLabel, event->size().width(), kFHintComboArrowReserve);
}

QSize FHintComboBox::minimumSizeHint() const
{
    QSize s = QComboBox::minimumSizeHint();
    s.setHeight(kFHintMinHeight);
    return s;
}

FHintTextEdit::FHintTextEdit(const QString& title, QWidget* parent)
    : QTextEdit(parent)
    , m_titleLabel(makeTitleLabel(this, title))
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QTextEdit::WidgetWidth);
    document()->setDocumentMargin(0);
    setViewportMargins(4, kFHintSingleTopPadding, 4, 1);
    placeTitleLabel(m_titleLabel, width());

    setMinimumHeight(m_singleHeight);
    setMaximumHeight(QWIDGETSIZE_MAX);

    connect(this, &QTextEdit::textChanged, this, [this]() {
        adjustHeightToContent(true);
    });
}

void FHintTextEdit::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
    m_titleLabel->setVisible(!title.isEmpty());
}

QString FHintTextEdit::title() const
{
    return m_titleLabel->text();
}

void FHintTextEdit::setAutoGrow(bool on)
{
    m_autoGrow = on;
    if (on) {
        setMinimumHeight(m_singleHeight);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        adjustHeightToContent(true);
    } else {
        setMinimumHeight(kFHintMinHeight);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    }
}

void FHintTextEdit::refreshHeight()
{
    if (!m_autoGrow)
        return;

    int viewW = viewport()->width();
    if (viewW <= 0)
        viewW = width() - 2 * frameWidth();
    FT_LOG("Widget.FHintTE.refresh", QString("viewportW=%1 width=%2 frameW=%3 viewW_used=%4")
        .arg(viewport()->width()).arg(width()).arg(frameWidth()).arg(viewW));
    if (viewW <= 0) {
        FT_LOG("Widget.FHintTE.refresh", "viewW still <= 0, bail");
        return;
    }

    document()->setTextWidth(viewW);
    document()->documentLayout()->documentSize();
    const int docH = qRound(document()->size().height());
    const QMargins vm = viewportMargins();
    const int needed = qMax(m_singleHeight,
                            vm.top() + docH + vm.bottom() + 2 * frameWidth());
    FT_LOG("Widget.FHintTE.refresh", QString("docH=%1 vmTop+Bot=%2 frameW*2=%3 needed=%4 currentMinH=%5")
        .arg(docH).arg(vm.top() + vm.bottom()).arg(2 * frameWidth()).arg(needed).arg(minimumHeight()));
    if (needed != minimumHeight()) {
        FT_LOG("Widget.FHintTE.refresh", QString("setMinimumHeight: %1 -> %2").arg(minimumHeight()).arg(needed));
        setMinimumHeight(needed);
        updateGeometry();
        emit heightChanged(needed);
    }
}

void FHintTextEdit::adjustHeightToContent(bool force)
{
    if (!m_autoGrow)
        return;

    const int viewW = viewport()->width();
    FT_LOG("Widget.FHintTE.adjust", QString("viewportW=%1 width=%2 force=%3")
        .arg(viewW).arg(width()).arg(force ? 1 : 0));
    if (viewW <= 0) {
        FT_LOG("Widget.FHintTE.adjust", "viewportW <= 0, QTimer delayed");
        QTimer::singleShot(0, this, [this]() { adjustHeightToContent(true); });
        return;
    }

    // 文档高度只取决于“换行宽度”和“文本内容”。宽度没变且内容没变时,
    // 重新计算的结果必然与上次相同;但本函数会被 resizeEvent/layout 反复触发,
    // 而且它算出的 minimumHeight 又会反过来驱动布局(布局→resizeEvent→本函数),
    // 于是外部尺寸噪声被逐轮放大,表现为标题最小高度每轮 +1 行的棘轮式增长
    // (整行步骤被越顶越高,组内留下一大片空灰)。
    // 因此宽度未变就直接短路,只在宽度变化或文本变化(force=true)时才重算。
    if (!force && viewW == m_lastLayoutWidth)
        return;
    m_lastLayoutWidth = viewW;

    document()->setTextWidth(viewW);
    document()->documentLayout()->documentSize();
    const int docH = qRound(document()->size().height());
    const QMargins vm = viewportMargins();
    const int needed = qMax(m_singleHeight,
                            vm.top() + docH + vm.bottom() + 2 * frameWidth());
    FT_LOG("Widget.FHintTE.adjust", QString("docH=%1 vmTop+Bot=%2 frameW*2=%3 needed=%4 minH=%5 h=%6 blocks=%7 chars=%8")
        .arg(docH).arg(vm.top() + vm.bottom()).arg(2 * frameWidth()).arg(needed)
        .arg(minimumHeight()).arg(height())
        .arg(document()->blockCount()).arg(document()->characterCount()));
    if (needed != minimumHeight()) {
        setMinimumHeight(needed);
        updateGeometry();
        emit heightChanged(needed);
    }
}

void FHintTextEdit::resizeEvent(QResizeEvent* event)
{
    QTextEdit::resizeEvent(event);
    placeTitleLabel(m_titleLabel, event->size().width());
    adjustHeightToContent();
}

void FHintTextEdit::showEvent(QShowEvent* event)
{
    QTextEdit::showEvent(event);
    adjustHeightToContent();
}

QSize FHintTextEdit::minimumSizeHint() const
{
    QSize s = QTextEdit::minimumSizeHint();
    s.setHeight(minimumHeight());
    return s;
}

FHintSpinBox::FHintSpinBox(const QString& title, QWidget* parent)
    : QSpinBox(parent)
    , m_titleLabel(makeTitleLabel(this, title))
{
    setMinimumHeight(kFHintMinHeight);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    if (QLineEdit* le = lineEdit())
        le->setTextMargins(4, kFHintSingleTopPadding, 4, 0);
    placeTitleLabel(m_titleLabel, width());
}

void FHintSpinBox::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
    m_titleLabel->setVisible(!title.isEmpty());
}

QString FHintSpinBox::title() const
{
    return m_titleLabel->text();
}

void FHintSpinBox::resizeEvent(QResizeEvent* event)
{
    QSpinBox::resizeEvent(event);
    placeTitleLabel(m_titleLabel, event->size().width());
}

void FHintSpinBox::keyPressEvent(QKeyEvent* event)
{
    QSpinBox::keyPressEvent(event);
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        event->accept();
}

void FHintSpinBox::wheelEvent(QWheelEvent* event)
{
    if (!hasFocus() && !(lineEdit() && lineEdit()->hasFocus())) {
        event->ignore();
        return;
    }
    QSpinBox::wheelEvent(event);
}

QSize FHintSpinBox::minimumSizeHint() const
{
    QSize s = QSpinBox::minimumSizeHint();
    s.setHeight(kFHintMinHeight);
    return s;
}