#include "FT_FunctionItem.h"
#include "FT_Function.h"
#include "FT_Widget.h"
#include "FT_Data.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include <QResizeEvent>

namespace {
constexpr int kItemMargin    = 2;
constexpr int kItemSpacing   = 6;
constexpr int kTitleMinH     = 36;
constexpr int kItemDefaultH  = 48;
constexpr int kListItemPadV  = 2;
constexpr int kDelayWidth    = 80;
}

FT_FunctionItem::FT_FunctionItem(QWidget* parent)
    : QWidget(parent)
{
    m_enableCheck = new QCheckBox(this);
    m_enableCheck->setFixedWidth(28);
    m_enableCheck->setChecked(true);
    m_enableCheck->setToolTip(tr("Enable/disable this Function"));
    m_enableCheck->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_titleEdit = new FHintTextEdit(tr("Title"), this);
    m_titleEdit->setFixedWidth(120);
    m_titleEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_titleEdit->setPlaceholderText(tr("Title"));

    m_delaySpin = new FHintSpinBox(tr("Delay"), this);
    m_delaySpin->setFixedWidth(kDelayWidth);
    m_delaySpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_delaySpin->setRange(0, 60000);
    m_delaySpin->setSingleStep(100);
    m_delaySpin->setValue(1000);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(kItemMargin, kItemMargin, kItemMargin, kItemMargin);
    root->setSpacing(kItemSpacing);
    root->addWidget(m_enableCheck, 0);
    root->addWidget(m_titleEdit,   0);
    root->addStretch(0);
    root->addWidget(m_delaySpin,   0);

    connect(m_titleEdit, &FHintTextEdit::textChanged, this, &FT_FunctionItem::contentChanged);
    connect(m_titleEdit, &FHintTextEdit::heightChanged, this, [this](int) {
        if (!m_loading)
            adjustHeight();
    });
    connect(m_enableCheck, &QCheckBox::toggled, this, &FT_FunctionItem::contentChanged);
    connect(m_delaySpin, QOverload<int>::of(&FHintSpinBox::valueChanged),
            this, [this](int) { emit contentChanged(); });

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* up = menu.addAction(
            style()->standardIcon(QStyle::SP_ArrowUp),
            tr("Move Up"));
        QAction* down = menu.addAction(
            style()->standardIcon(QStyle::SP_ArrowDown),
            tr("Move Down"));
        menu.addSeparator();
        QAction* del = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove this Function"));
        QAction* chosen = menu.exec(mapToGlobal(pos));
        if (chosen == up)
            emit moveUpRequested();
        else if (chosen == down)
            emit moveDownRequested();
        else if (chosen == del)
            emit removeRequested();
    });

    QTimer::singleShot(0, this, &FT_FunctionItem::adjustHeight);
}

void FT_FunctionItem::connectFunction(FT_Function* function)
{
    connect(function, &FT_Function::contentChanged,
            this, &FT_FunctionItem::contentChanged);
    connect(function, &FT_Function::requestRemove,
            this, &FT_FunctionItem::removeRequested);
    connect(function, &FT_Function::requestMoveUp,
            this, &FT_FunctionItem::moveUpRequested);
    connect(function, &FT_Function::requestMoveDown,
            this, &FT_FunctionItem::moveDownRequested);
    connect(function, &FT_Function::requestResize, this, [this]() {
        if (!m_loading)
            adjustHeight();
    });
}

void FT_FunctionItem::setFunction(FT_Function* function)
{
    if (m_function == function)
        return;

    if (m_function) {
        layout()->removeWidget(m_function);
        m_function->deleteLater();
        m_function = nullptr;
    }

    m_function = function;
    if (!m_function) {
        if (!m_loading)
            adjustHeight();
        return;
    }

    if (auto* hbox = qobject_cast<QHBoxLayout*>(layout())) {
        hbox->insertWidget(2, m_function, 1);
    }
    connectFunction(m_function);

    if (!m_loading) {
        adjustHeight();
        emit contentChanged();
    }
}

FT_FunctionItemConfig FT_FunctionItem::toConfig() const
{
    FT_FunctionItemConfig cfg;
    cfg.enabled = m_enableCheck->isChecked();
    cfg.title = m_titleEdit->toPlainText().trimmed();
    cfg.delayMs = m_delaySpin->value();
    if (m_function)
        cfg.functionData = m_function->toConfig();
    return cfg;
}

void FT_FunctionItem::applyConfig(const FT_FunctionItemConfig& cfg)
{
    m_loading = true;

    m_enableCheck->setChecked(cfg.enabled);
    m_titleEdit->setPlainText(cfg.title);
    m_delaySpin->setValue(cfg.delayMs);

    if (m_function) {
        layout()->removeWidget(m_function);
        m_function->deleteLater();
        m_function = nullptr;
    }

    QString typeName = ftFunctionDataTypeName(cfg.functionData);
    if (typeName != QLatin1String("Empty")) {
        FT_Function* func = FtFunctionFactory::create(typeName);
        if (func) {
            m_function = func;
            if (auto* hbox = qobject_cast<QHBoxLayout*>(layout()))
                hbox->insertWidget(2, m_function, 1);
            connectFunction(m_function);
            func->applyConfig(cfg.functionData);
        }
    }

    m_loading = false;
    emit contentChanged();

    QTimer::singleShot(0, this, &FT_FunctionItem::adjustHeight);
}

void FT_FunctionItem::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_loading && !m_adjusting)
        adjustHeight();
}

void FT_FunctionItem::adjustHeight()
{
    if (m_adjusting) {
        m_pendingAdjust = true;
        return;
    }
    m_adjusting = true;
    m_pendingAdjust = false;

    int funcMinH = 0;
    if (m_function) {
        funcMinH = m_function->minimumHeight();
        if (funcMinH <= 0) {
            const int sh = m_function->sizeHint().height();
            funcMinH = sh > 0 ? sh : m_function->height();
        }
    }
    int titleMinH = kTitleMinH;
    if (m_titleEdit) {
        const int th = m_titleEdit->minimumHeight();
        titleMinH = th > 0 ? th : qMax(kTitleMinH, m_titleEdit->sizeHint().height());
    }
    const int contentH = qMax(titleMinH, funcMinH);
    const int itemH = contentH + kItemMargin * 2;

    m_cachedHeight = itemH;
    setFixedHeight(itemH);
    updateGeometry();

    m_adjusting = false;

    if (m_pendingAdjust) {
        m_pendingAdjust = false;
        adjustHeight();
        return;
    }

    QWidget* vp = parentWidget();
    QListWidget* outer = vp ? qobject_cast<QListWidget*>(vp->parentWidget()) : nullptr;
    if (!outer)
        return;

    for (int i = 0; i < outer->count(); ++i) {
        QListWidgetItem* outerItem = outer->item(i);
        if (outer->itemWidget(outerItem) == this) {
            outerItem->setSizeHint(QSize(0, itemH + kListItemPadV));
            break;
        }
    }
}

QSize FT_FunctionItem::sizeHint() const
{
    return QSize(0, m_cachedHeight > 0 ? m_cachedHeight : kItemDefaultH);
}