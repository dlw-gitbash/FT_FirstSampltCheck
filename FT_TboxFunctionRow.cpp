#include "FT_TboxFunctionRow.h"
#include "FT_AdvanceDialog.h"
#include "FHintWidgets.h"

#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QPalette>
#include <QContextMenuEvent>
#include <QEvent>
#include <QColor>
#include <QStyle>

namespace {
constexpr int kRowDefaultHeight = 36;
}

FT_TboxFunctionRow::FT_TboxFunctionRow(QWidget* parent)
    : FT_FunctionRow(parent)
{
    setObjectName(QStringLiteral("ftTboxRow"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(kRowDefaultHeight);
    setStyleSheet(
        "QWidget#ftTboxRow {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 3px;"
        "}");

    m_combo = new FHintComboBox(tr("TBox Command"), this);
    m_combo->setFixedWidth(160);
    m_combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_combo->addItems({
        QStringLiteral("AT+CGSN"),
        QStringLiteral("AT+CFUN?"),
        QStringLiteral("AT+COPS?"),
        QStringLiteral("AT+CSQ"),
        QStringLiteral("AT$QCRMCALL?"),
        QStringLiteral("TBox_Reset"),
        QStringLiteral("TBox_Version")
    });

    m_payload = new FHintTextEdit(tr("Payload"), this);
    m_payload->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_payload->setPlaceholderText(tr("Hex bytes separated by spaces, e.g. 22 66"));

    m_advance = new QLabel(this);
    m_advance->setFixedWidth(80);
    m_advance->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_advance->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QPalette advancePal = m_advance->palette();
    advancePal.setColor(QPalette::WindowText, QColor(0x1B, 0x7A, 0x3A));
    m_advance->setPalette(advancePal);

    m_delay = new FHintSpinBox(tr("Delay"), this);
    m_delay->setFixedWidth(80);
    m_delay->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_delay->setRange(0, 60000);
    m_delay->setSingleStep(100);
    m_delay->setValue(1000);

    auto* layout = rowLayout();
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(6);
    layout->addWidget(m_combo,   0);
    layout->addWidget(m_payload, 1);
    layout->addWidget(m_advance, 0);
    layout->addWidget(m_delay,   0);

    connect(m_combo, &FHintComboBox::currentTextChanged,
            this, &FT_FunctionRow::contentChanged);
    connect(m_payload, &FHintTextEdit::textChanged,
            this, &FT_FunctionRow::contentChanged);
    connect(m_delay, QOverload<int>::of(&FHintSpinBox::valueChanged),
            this, [this](int) { emit contentChanged(); });

    connect(m_payload, &FHintTextEdit::heightChanged, this, [this](int) {
        updateRowHeight();
    });

    installEventFilter(this);
    const QList<QWidget*> kids = findChildren<QWidget*>();
    for (QWidget* w : kids) {
        if (qobject_cast<FHintTextEdit*>(w))
            continue;
        w->installEventFilter(this);
    }

    updateAdvanceLabel();
    updateRowHeight();
}

QSize FT_TboxFunctionRow::sizeHint() const
{
    int inner = kFHintMinHeight;
    auto bumpMin = [&inner](const QWidget* w) {
        if (w)
            inner = qMax(inner, w->minimumHeight());
    };
    bumpMin(m_combo);
    bumpMin(m_payload);
    bumpMin(m_delay);

    const int margin = layout()
                           ? layout()->contentsMargins().top() + layout()->contentsMargins().bottom()
                           : 0;
    const int h = qMax(kRowDefaultHeight, inner + margin);
    return QSize(0, h);
}

void FT_TboxFunctionRow::updateRowHeight()
{
    const int h = sizeHint().height();
    if (h != minimumHeight()) {
        setMinimumHeight(h);
        updateGeometry();
        emit requestResize();
    }
}

QString FT_TboxFunctionRow::typeName() const
{
    return QStringLiteral("TBoxCommand");
}

FT_FunctionRowData FT_TboxFunctionRow::config() const
{
    FT_TboxConfig cfg;
    cfg.command = m_combo->currentText();
    cfg.payload = m_payload->toPlainText();
    cfg.delayMs = m_delay->value();
    cfg.advance = m_advanceCfg;
    return cfg;
}

void FT_TboxFunctionRow::applyConfig(const FT_FunctionRowData& data)
{
    if (!std::holds_alternative<FT_TboxConfig>(data))
        return;

    const FT_TboxConfig& t = std::get<FT_TboxConfig>(data);
    if (int idx = m_combo->findText(t.command); idx >= 0) {
        m_combo->setCurrentIndex(idx);
    } else if (!t.command.isEmpty()) {
        m_combo->insertItem(0, t.command);
        m_combo->setCurrentIndex(0);
    }
    m_payload->setPlainText(t.payload);

    const int lo = m_delay->minimum();
    const int hi = m_delay->maximum();
    const int delay = qBound(lo, t.delayMs, hi);
    if (delay != t.delayMs)
        qWarning().noquote() << QStringLiteral("delayMs=%1 out of [%2,%3], clamped to %4")
                                    .arg(t.delayMs).arg(lo).arg(hi).arg(delay);
    m_delay->setValue(delay);

    m_advanceCfg = t.advance;
    updateAdvanceLabel();
}

QString FT_TboxFunctionRow::resultModeText(FtResultMode mode)
{
    switch (mode) {
    case FtResultMode::Equal:     return tr("Equal");
    case FtResultMode::Range:     return tr("Range");
    case FtResultMode::StartWith: return tr("Start With");
    case FtResultMode::DontCare:  default: return tr("DontCare");
    }
}

void FT_TboxFunctionRow::updateAdvanceLabel()
{
    m_advance->setText(resultModeText(m_advanceCfg.toResult));
}

void FT_TboxFunctionRow::contextMenuEvent(QContextMenuEvent* event)
{
    showRowContextMenu(event->globalPos());
}

bool FT_TboxFunctionRow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::ContextMenu) {
        if (qobject_cast<FHintTextEdit*>(watched))
            return FT_FunctionRow::eventFilter(watched, event);
        auto* ce = static_cast<QContextMenuEvent*>(event);
        showRowContextMenu(ce->globalPos());
        return true;
    }
    return FT_FunctionRow::eventFilter(watched, event);
}

void FT_TboxFunctionRow::showRowContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    QAction* actAdvance = menu.addAction(tr("Response Advance"));
    QAction* actRemove  = menu.addAction(
        style()->standardIcon(QStyle::SP_TrashIcon),
        tr("Remove this row"));
    QAction* chosen = menu.exec(globalPos);
    if (chosen == actAdvance)
        openResponseAdvance();
    else if (chosen == actRemove)
        emit requestRemove();
}

void FT_TboxFunctionRow::openResponseAdvance()
{
    FT_AdvanceDialog dlg(this);
    dlg.setConfig(m_advanceCfg);
    if (dlg.exec() == QDialog::Accepted) {
        m_advanceCfg = dlg.config();
        updateAdvanceLabel();
        emit contentChanged();
    }
}