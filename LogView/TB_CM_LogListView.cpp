#include "TB_CM_LogListView.h"

#include "TB_CM_LogDelegate.h"
#include "TB_CM_LogModel.h"

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFont>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QResizeEvent>
#include <QScrollBar>

#include <algorithm>

namespace {

bool rowLessThan(const QModelIndex& lhs, const QModelIndex& rhs)
{
    return lhs.row() < rhs.row();
}
QFont monospaceFont()
{
    QFont font(QStringLiteral("Consolas"));
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(9);
    return font;
}

} // namespace

TB_CM_LogListView::TB_CM_LogListView(QWidget* parent)
    : QListView(parent)
    , delegate_(new TB_CM_LogDelegate(this))
{
    setObjectName(QStringLiteral("TB_CM_LogListView"));
    setItemDelegate(delegate_);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setMovement(QListView::Static);
    setDragDropMode(QAbstractItemView::NoDragDrop);
    setFlow(QListView::TopToBottom);
    setViewMode(QListView::ListMode);
    setWrapping(false);
    setResizeMode(QListView::Adjust);
    setLayoutMode(QListView::Batched);
    setBatchSize(1000);
    setUniformItemSizes(false);
    setSpacing(0);
    setWordWrap(true);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTextElideMode(Qt::ElideNone);
    setFocusPolicy(Qt::StrongFocus);
    setAutoScroll(true);
    setAlternatingRowColors(true);
    setFont(monospaceFont());
    viewport()->setAutoFillBackground(true);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this,
            &TB_CM_LogListView::onScrollBarRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
            &TB_CM_LogListView::onScrollBarValueChanged);
    scheduleDelayedItemsLayout();
}
void TB_CM_LogListView::setModel(QAbstractItemModel* model)
{
    if (model_ != nullptr) {
        disconnect(model_, nullptr, this, nullptr);
    }
    QListView::setModel(model);
    model_ = qobject_cast<TB_CM_LogModel*>(model);
    delegate_->setLogModel(model_);
    delegate_->setFallbackWidth(viewport()->width());
    if (model_ != nullptr) {
        connect(model_, &TB_CM_LogModel::rowsAppended, this,
                &TB_CM_LogListView::onRowsAppended);
        connect(model_, &TB_CM_LogModel::modelReset, this, &TB_CM_LogListView::onModelReset);
        connect(model_, &TB_CM_LogModel::filterChanged, this,
                &TB_CM_LogListView::onFilterChanged);
        delegate_->setFilter(model_->filter());
    }
}
void TB_CM_LogListView::setWrapEnabled(bool enabled)
{
    if (wrapEnabled_ == enabled) {
        return;
    }
    wrapEnabled_ = enabled;
    setWordWrap(enabled);
    delegate_->setWrapEnabled(enabled);
    if (model_ != nullptr) {
        model_->invalidateLayoutCache();
    }
    scheduleDelayedItemsLayout();
    viewport()->update();
}
void TB_CM_LogListView::setFollowTail(bool enabled)
{
    if (followTail_ == enabled) {
        return;
    }
    followTail_ = enabled;
    if (enabled) {
        scrollToBottom();
    }
    emit followTailChanged(enabled);
}
void TB_CM_LogListView::onRowsAppended(int, int)
{
    if (followTail_) {
        scrollToBottom();
    }
}
void TB_CM_LogListView::onModelReset()
{
    if (model_ != nullptr) {
        delegate_->setFilter(model_->filter());
    }
    if (followTail_) {
        scrollToBottom();
    }
    viewport()->update();
}
void TB_CM_LogListView::onFilterChanged()
{
    if (model_ != nullptr) {
        delegate_->setFilter(model_->filter());
    }
}
void TB_CM_LogListView::onScrollBarRangeChanged(int, int maximum)
{
    // 布局是分批执行的，最大值在插入后才更新，跟随模式下要把视图钉在底部
    if (followTail_ && verticalScrollBar()->value() != maximum) {
        applyingFollow_ = true;
        verticalScrollBar()->setValue(maximum);
        applyingFollow_ = false;
    }
}
void TB_CM_LogListView::onScrollBarValueChanged(int value)
{
    if (applyingFollow_) {
        return;
    }
    const bool atBottom = value >= verticalScrollBar()->maximum();
    if (atBottom != followTail_) {
        followTail_ = atBottom;
        emit followTailChanged(followTail_);
    }
}
void TB_CM_LogListView::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);
    const int width = viewport()->width();
    if (width != delegate_->fallbackWidth()) {
        delegate_->setFallbackWidth(width);
        scheduleDelayedItemsLayout();
    }
}
void TB_CM_LogListView::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy)) {
        copySelectionToClipboard();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::SelectAll)) {
        selectAll();
        event->accept();
        return;
    }
    QListView::keyPressEvent(event);
}
void TB_CM_LogListView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    QAction* copy = menu.addAction(tr("复制选中"));
    copy->setEnabled(selectionModel() != nullptr && !selectionModel()->selectedIndexes().isEmpty());
    copy->setShortcut(QKeySequence::Copy);
    connect(copy, &QAction::triggered, this, &TB_CM_LogListView::copySelectionToClipboard);
    QAction* copyAll = menu.addAction(tr("复制全部（当前过滤）"));
    connect(copyAll, &QAction::triggered, this, &TB_CM_LogListView::copyAllToClipboard);
    copyAll->setEnabled(model_ != nullptr && model_->visibleCount() > 0);
    menu.addSeparator();
    QAction* follow = menu.addAction(tr("自动滚动"));
    follow->setCheckable(true);
    follow->setChecked(followTail_);
    connect(follow, &QAction::toggled, this, &TB_CM_LogListView::setFollowTail);
    menu.addSeparator();
    QAction* clear = menu.addAction(tr("清空"));
    connect(clear, &QAction::triggered, this, &TB_CM_LogListView::clearRequested);
    menu.exec(event->globalPos());
}
QString TB_CM_LogListView::lineFor(const TB_CM_Record& record) const
{
    QString line = QDateTime::fromMSecsSinceEpoch(qint64(record.timestampMs))
                       .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    line += QStringLiteral(" [%1]").arg(levelName(record.level).toString());
    if (!record.category.isEmpty()) {
        line += QStringLiteral(" [%1]").arg(record.category);
    }
    if (!record.message.isEmpty()) {
        line += QLatin1String(": ") + record.message;
    }
    return line;
}
void TB_CM_LogListView::copySelectionToClipboard()
{
    if (model_ == nullptr || selectionModel() == nullptr) {
        return;
    }
    QModelIndexList rows = selectionModel()->selectedIndexes();
    std::sort(rows.begin(), rows.end(), rowLessThan);
    QStringList lines;
    lines.reserve(rows.size());
    for (const QModelIndex& index : rows) {
        const TB_CM_Record* record = model_->recordAt(index.row());
        if (record != nullptr) {
            lines.append(lineFor(*record));
        }
    }
    if (!lines.isEmpty()) {
        QGuiApplication::clipboard()->setText(lines.join(u'\n'));
    }
}
void TB_CM_LogListView::copyAllToClipboard()
{
    if (model_ == nullptr) {
        return;
    }
    QStringList lines;
    lines.reserve(model_->visibleCount());
    for (int row = 0; row < model_->visibleCount(); ++row) {
        const TB_CM_Record* record = model_->recordAt(row);
        if (record != nullptr) {
            lines.append(lineFor(*record));
        }
    }
    if (!lines.isEmpty()) {
        QGuiApplication::clipboard()->setText(lines.join(u'\n'));
    }
}
