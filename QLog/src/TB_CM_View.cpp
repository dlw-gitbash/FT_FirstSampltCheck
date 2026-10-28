#include <TB_CM_View.h>

#include <TB_CM_Delegate.h>
#include <TB_CM_Model.h>

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>

#include <algorithm>

namespace {

bool rowLessThan(const QModelIndex& lhs, const QModelIndex& rhs)
{
    return lhs.row() < rhs.row();
}

} // namespace

TB_CM_View::TB_CM_View(QWidget* parent)
    : QListView(parent)
    , delegate_(new TB_CM_Delegate(this))
{
    setObjectName(QStringLiteral("TB_CM_View"));
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
    viewport()->setAutoFillBackground(true);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &TB_CM_View::onScrollBarRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TB_CM_View::onScrollBarValueChanged);
    setTheme(theme_);
}
void TB_CM_View::setModel(QAbstractItemModel* model)
{
    if (model_ != nullptr) {
        disconnect(model_, nullptr, this, nullptr);
    }
    QListView::setModel(model);
    model_ = qobject_cast<TB_CM_Model*>(model);
    delegate_->setLogModel(model_);
    delegate_->setFallbackWidth(viewport()->width());
    if (model_ != nullptr) {
        connect(model_, &TB_CM_Model::rowsAppended, this, &TB_CM_View::onRowsAppended);
        connect(model_, &TB_CM_Model::modelReset, this, &TB_CM_View::onModelReset);
        connect(model_, &TB_CM_Model::filterChanged, this, &TB_CM_View::onFilterChanged);
        delegate_->setFilter(model_->filter());
    }
}
void TB_CM_View::setTheme(const Theme& theme)
{
    theme_ = theme;
    setFont(theme.font);
    delegate_->setTheme(theme);
    // 必须先设样式表再设调色板：QStyleSheetStyle 在 polish 时会重写调色板
    setStyleSheet(QStringLiteral(
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background: %1; min-height: 22px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: %2; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
        .arg(theme.scrollBar.name(), theme.scrollBarHover.name()));
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, theme.background);
    palette.setColor(QPalette::Window, theme.background);
    palette.setColor(QPalette::Text, theme.text);
    palette.setColor(QPalette::Highlight, theme.selection);
    palette.setColor(QPalette::HighlightedText, theme.text);
    setPalette(palette);
    QPalette viewportPalette = viewport()->palette();
    viewportPalette.setColor(QPalette::Base, theme.background);
    viewportPalette.setColor(QPalette::Window, theme.background);
    viewportPalette.setColor(QPalette::Text, theme.text);
    viewportPalette.setColor(QPalette::Highlight, theme.selection);
    viewportPalette.setColor(QPalette::HighlightedText, theme.text);
    viewport()->setPalette(viewportPalette);
    if (model_ != nullptr) {
        model_->invalidateLayoutCache();
    }
    scheduleDelayedItemsLayout();
    viewport()->update();
}
void TB_CM_View::setWrapEnabled(bool enabled)
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
void TB_CM_View::setFollowTail(bool enabled)
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
void TB_CM_View::onRowsAppended(int, int)
{
    if (followTail_) {
        scrollToBottom();
    }
}
void TB_CM_View::onModelReset()
{
    if (model_ != nullptr) {
        delegate_->setFilter(model_->filter());
    }
    if (followTail_) {
        scrollToBottom();
    }
    viewport()->update();
}
void TB_CM_View::onFilterChanged()
{
    if (model_ != nullptr) {
        delegate_->setFilter(model_->filter());
    }
}
void TB_CM_View::onScrollBarRangeChanged(int, int maximum)
{
    // 布局是分批执行的，最大值在插入后才更新，跟随模式下要把视图钉在底部
    if (followTail_ && verticalScrollBar()->value() != maximum) {
        applyingFollow_ = true;
        verticalScrollBar()->setValue(maximum);
        applyingFollow_ = false;
    }
}
void TB_CM_View::onScrollBarValueChanged(int value)
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
void TB_CM_View::paintEvent(QPaintEvent* event)
{
    // 行区域由委托自绘，空区域必须显式填底色，视口调色板会被样式表重新 polish
    QPainter painter(viewport());
    painter.fillRect(event->rect(), theme_.background);
    painter.end();
    QListView::paintEvent(event);
}
void TB_CM_View::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);
    const int width = viewport()->width();
    if (width != delegate_->fallbackWidth()) {
        delegate_->setFallbackWidth(width);
        scheduleDelayedItemsLayout();
    }
}
void TB_CM_View::keyPressEvent(QKeyEvent* event)
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
void TB_CM_View::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    QAction* copy = menu.addAction(tr("复制选中"));
    copy->setEnabled(selectionModel() != nullptr && !selectionModel()->selectedIndexes().isEmpty());
    copy->setShortcut(QKeySequence::Copy);
    connect(copy, &QAction::triggered, this, &TB_CM_View::copySelectionToClipboard);
    QAction* copyAll = menu.addAction(tr("复制全部（当前过滤）"));
    connect(copyAll, &QAction::triggered, this, &TB_CM_View::copyAllToClipboard);
    copyAll->setEnabled(model_ != nullptr && model_->visibleCount() > 0);
    menu.addSeparator();
    QAction* follow = menu.addAction(tr("自动滚动"));
    follow->setCheckable(true);
    follow->setChecked(followTail_);
    connect(follow, &QAction::toggled, this, &TB_CM_View::setFollowTail);
    menu.addSeparator();
    QAction* clear = menu.addAction(tr("清空"));
    connect(clear, &QAction::triggered, this, &TB_CM_View::clearRequested);
    menu.exec(event->globalPos());
}
QString TB_CM_View::lineFor(const Record& record) const
{
    QString line = QDateTime::fromMSecsSinceEpoch(record.timestampMs)
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
void TB_CM_View::copySelectionToClipboard()
{
    if (model_ == nullptr || selectionModel() == nullptr) {
        return;
    }
    QModelIndexList rows = selectionModel()->selectedIndexes();
    std::sort(rows.begin(), rows.end(), rowLessThan);
    QStringList lines;
    lines.reserve(rows.size());
    for (const QModelIndex& index : rows) {
        const Record* record = model_->recordAt(index.row());
        if (record != nullptr) {
            lines.append(lineFor(*record));
        }
    }
    if (!lines.isEmpty()) {
        QGuiApplication::clipboard()->setText(lines.join(u'\n'));
    }
}
void TB_CM_View::copyAllToClipboard()
{
    if (model_ == nullptr) {
        return;
    }
    QStringList lines;
    lines.reserve(model_->visibleCount());
    for (int row = 0; row < model_->visibleCount(); ++row) {
        const Record* record = model_->recordAt(row);
        if (record != nullptr) {
            lines.append(lineFor(*record));
        }
    }
    if (!lines.isEmpty()) {
        QGuiApplication::clipboard()->setText(lines.join(u'\n'));
    }
}
