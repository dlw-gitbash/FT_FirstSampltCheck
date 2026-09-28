#include "FT_FunctionItem.h"
#include "FT_FunctionGroup.h"
#include "FT_Widget.h"
#include "FT_Log.h"

#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include <QResizeEvent>

FT_FunctionItem::FT_FunctionItem(QWidget* parent)
    : QWidget(parent)
{
    m_enableCheck = new QCheckBox(this);
    m_enableCheck->setFixedWidth(28);
    m_enableCheck->setChecked(true);
    m_enableCheck->setToolTip(tr("Enable/disable this Step"));
    m_enableCheck->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_titleEdit = new FHintTextEdit(tr("Title"), this);
    m_titleEdit->setFixedWidth(120);
    m_titleEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_titleEdit->setPlaceholderText(tr("Title"));

    m_delaySpin = new FHintSpinBox(tr("Delay"), this);
    m_delaySpin->setFixedWidth(kItemDelayWidth);
    m_delaySpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_delaySpin->setRange(0, 60000);
    m_delaySpin->setSingleStep(100);
    m_delaySpin->setValue(1000);

    m_group = new FT_FunctionGroup(this);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(kItemMargin, kItemMargin, kItemMargin, kItemMargin);
    root->setSpacing(kItemSpacing);
    root->addWidget(m_enableCheck, 0);
    root->addWidget(m_titleEdit,   0);
    root->addWidget(m_group,       1);
    root->addWidget(m_delaySpin,   0);

    connect(m_titleEdit, &FHintTextEdit::textChanged, this, [this]() {
        if (!m_loading)
            emit contentChanged();
    });
    connect(m_titleEdit, &FHintTextEdit::heightChanged, this, [this](int) {
        if (!m_loading)
            adjustHeight();
    });
    connect(m_enableCheck, &QCheckBox::toggled, this, [this](bool) {
        if (!m_loading)
            emit contentChanged();
    });
    connect(m_delaySpin, QOverload<int>::of(&FHintSpinBox::valueChanged),
            this, [this](int) { if (!m_loading) emit contentChanged(); });

    connect(m_group, &FT_FunctionGroup::contentChanged, this, [this]() {
        if (!m_loading)
            emit contentChanged();
    });
    connect(m_group, &FT_FunctionGroup::structureChanged, this, [this]() {
        if (!m_loading) {
            adjustHeight();
            emit contentChanged();
        }
    });
    connect(m_group, &FT_FunctionGroup::requestResize, this, [this]() {
        if (!m_loading)
            adjustHeight();
    });
    connect(m_group, &FT_FunctionGroup::splitStepRequested,
            this, &FT_FunctionItem::splitStepRequested);
    connect(m_group, &FT_FunctionGroup::requestInsertStepAfter,
            this, &FT_FunctionItem::insertStepAfterRequested);
    connect(m_group, &FT_FunctionGroup::requestMergeWithNext,
            this, &FT_FunctionItem::mergeWithNextRequested);

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
        QAction* addLine = menu.addAction(
            style()->standardIcon(QStyle::SP_FileDialogNewFolder),
            tr("Wrap to New Empty Step After"));
        QAction* merge = menu.addAction(
            style()->standardIcon(QStyle::SP_ArrowBack),
            tr("Merge with Next Step"));
        menu.addSeparator();
        QAction* del = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove this Step"));
        QAction* chosen = menu.exec(mapToGlobal(pos));
        if (chosen == up)
            emit moveUpRequested();
        else if (chosen == down)
            emit moveDownRequested();
        else if (chosen == addLine)
            emit insertStepAfterRequested();
        else if (chosen == merge)
            emit mergeWithNextRequested();
        else if (chosen == del)
            emit removeRequested();
    });

    QTimer::singleShot(0, this, &FT_FunctionItem::adjustHeight);
}

FT_FunctionItemConfig FT_FunctionItem::toConfig() const
{
    FT_FunctionItemConfig cfg;
    cfg.enabled = m_enableCheck->isChecked();
    cfg.title = m_titleEdit->toPlainText().trimmed();
    cfg.delayMs = m_delaySpin->value();
    cfg.lines = m_group->toLines();
    return cfg;
}

void FT_FunctionItem::applyConfig(const FT_FunctionItemConfig& cfg)
{
    m_loading = true;

    m_enableCheck->setChecked(cfg.enabled);
    m_titleEdit->setPlainText(cfg.title);
    m_delaySpin->setValue(cfg.delayMs);
    m_group->applyLines(cfg.lines);

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
        FT_LOG("FI.adjustHeight", "recursive guard hit, set pending");
        return;
    }
    m_adjusting = true;
    m_pendingAdjust = false;

    int groupH = 0;
    if (m_group) {
        groupH = m_group->sizeHint().height();
        if (groupH <= 0)
            groupH = m_group->height();
    }
    int titleMinH = kItemTitleMinH;
    if (m_titleEdit) {
        const int th = m_titleEdit->minimumHeight();
        titleMinH = th > 0 ? th : qMax(kItemTitleMinH, m_titleEdit->sizeHint().height());
    }
    const int contentH = qMax(titleMinH, groupH);
    const int itemH = contentH + kItemMargin * 2;

    FT_LOG("FI.adjustHeight", QString("group->sizeHint.h=%1 group->h=%2 titleMinH=%3 contentH=%4 itemH=%5")
        .arg(m_group ? m_group->sizeHint().height() : -1)
        .arg(m_group ? m_group->height() : -1)
        .arg(titleMinH).arg(contentH).arg(itemH));

    m_cachedHeight = itemH;
    setFixedHeight(itemH);
    updateGeometry();

    m_adjusting = false;

    if (m_pendingAdjust) {
        m_pendingAdjust = false;
        FT_LOG("FI.adjustHeight", "re-trigger due to pending");
        adjustHeight();
        return;
    }

    QWidget* vp = parentWidget();
    QListWidget* outer = vp ? qobject_cast<QListWidget*>(vp->parentWidget()) : nullptr;
    if (!outer) {
        FT_LOG("FI.adjustHeight", "outer QListWidget not found, skip item sizeHint update");
        return;
    }

    for (int i = 0; i < outer->count(); ++i) {
        QListWidgetItem* outerItem = outer->item(i);
        if (outer->itemWidget(outerItem) == this) {
            outerItem->setSizeHint(QSize(0, itemH + kItemListPadV));
            FT_LOG("FI.adjustHeight", QString("outer[%1].setSizeHint h=%2").arg(i).arg(itemH + kItemListPadV));
            break;
        }
    }
}

QSize FT_FunctionItem::sizeHint() const
{
    return QSize(0, m_cachedHeight > 0 ? m_cachedHeight : kItemDefaultH);
}