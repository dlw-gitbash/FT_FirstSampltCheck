#include "FHintWidgets.h"

#include <QLabel>
#include <QLineEdit>
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

static constexpr int kTitleLeft = 6;

static QLabel* makeTitleLabel(QWidget* host, const QString& title)
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

static void placeTitleLabel(QLabel* label, int hostWidth, int rightReserve = 0)
{
    if (!label)
        return;
    label->setGeometry(kTitleLeft, kFHintTitleTop,
                       qMax(0, hostWidth - 2 * kTitleLeft - rightReserve),
                       kFHintTitleHeight);
}

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

    m_singleHeight = kFHintMinHeight;
    setMinimumHeight(m_singleHeight);
    setMaximumHeight(QWIDGETSIZE_MAX);

    connect(this, &QTextEdit::textChanged, this, &FHintTextEdit::adjustHeightToContent);
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
        adjustHeightToContent();
    } else {
        setMinimumHeight(kFHintMinHeight);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    }
}

void FHintTextEdit::adjustHeightToContent()
{
    if (!m_autoGrow)
        return;

    const int viewW = viewport()->width();
    if (viewW <= 0) {
        QTimer::singleShot(0, this, &FHintTextEdit::adjustHeightToContent);
        return;
    }

    document()->setTextWidth(viewW);
    const int docH = qRound(document()->size().height());
    const QMargins vm = viewportMargins();
    const int needed = qMax(m_singleHeight,
                            vm.top() + docH + vm.bottom() + 2 * frameWidth());
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