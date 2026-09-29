#include "FT_FunctionGroup.h"
#include "FT_Function.h"
#include "FT_Data.h"
#include "FT_Log.h"
// 复用 FT_FunctionList 里的 FtDropIndicator(自绘插入指示线),保证组内/组外观感一致。
#include "FT_FunctionList.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QPointer>
#include <QPainter>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>

static QPointer<QWidget> s_commandDragGroup;
static int s_commandDragFlat = -1;

// 组内 item 的边框与内边距,必须与构造函数里 QListWidget#ftFunctionGroup::item 的 QSS 保持一致。
// 视图会把行控件放进 item 的“内容矩形”(visualRect 再内缩 border+padding),
// 所以行控件的宽/高要各减去 2*inset,item 自身的尺寸要各加上 2*inset。
static constexpr int kGroupItemBorder  = 1;
static constexpr int kGroupItemPadding = 2;
static constexpr int kGroupItemInset   = kGroupItemBorder + kGroupItemPadding;

FtCommandDragSourceInfo ftCommandDragSource()
{
    return {s_commandDragGroup.data(), s_commandDragFlat};
}

void ftSetCommandDragSource(QWidget* group, int flat)
{
    s_commandDragGroup = group;
    s_commandDragFlat = flat;
}

void ftClearCommandDragSource()
{
    s_commandDragGroup.clear();
    s_commandDragFlat = -1;
}

FT_FunctionGroup::FT_FunctionGroup(QWidget* parent)
    : QListWidget(parent)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    // 关闭 Qt 自带的插入指示,改用自绘指示线(见 updateDropIndicator),
    // 以便落点与 m_dragOverRow 完全一致(与 FT_FunctionList 同款做法)。
    setDropIndicatorShown(false);

    m_dropIndicator = new FtDropIndicator(viewport());
    m_dropIndicator->raise();

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);
    setContentsMargins(0, 0, 0, 0);
    // 纵向 Expanding:步骤比本组内容高时(例如左侧标题换行变高),
    // 由布局把多余高度给本组,灰底内容区跟着长高,而不是居中留白。
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setSpacing(kGroupRowSpacing);

    // 外层步骤列表(FT_FunctionList)设了不带限定符的样式表:
    //   QListWidget       { background-color:#f5f5f5; border:1px solid #d9d9d9; padding:6px; }
    //   QListWidget::item { background-color:#ffffff; border:1px solid #d9d9d9; border-radius:4px; }
    // QSS 的类选择器会命中子类,于是这些规则也落到本命令组上:组被额外加上 border+padding,
    // 视口比组本身小 14px,而 sizeHint() 是按内容算的 44,于是行控件被塞进 44 高的组里后
    // 下移(顶部多出间距)、下半截被视口裁掉,组底部再空出一条。这里用 ID 选择器把规则收回到本组,
    // 组自身只保留灰底(不能有 border/padding,否则视口会内缩);单元格的边框/底色/选中高亮
    // 仍按外层列表的观感给出,但只作用于 ::item,不影响组自身的几何。
    setObjectName(QStringLiteral("ftFunctionGroup"));
    setStyleSheet(QStringLiteral(
        "QListWidget#ftFunctionGroup {"
        "  background-color: #f5f5f5;"
        "  border: none;"
        "  padding: 0px;"
        "}"
        "QListWidget#ftFunctionGroup::item {"
        "  background-color: #ffffff;"
        "  border: %1px solid #d9d9d9;"
        "  border-radius: 4px;"
        "  padding: %2px;"
        "}"
        "QListWidget#ftFunctionGroup::item:selected {"
        "  background-color: #d6e8ff;"
        "  border: %1px solid #4a90d9;"
        "}"
        "QListWidget#ftFunctionGroup::item:hover:!selected {"
        "  border: %1px solid #a8c8f0;"
        "}")
        .arg(kGroupItemBorder).arg(kGroupItemPadding));

    connect(this, &QListWidget::itemClicked,
            this, &FT_FunctionGroup::onItemClicked);
    connect(this, &QListWidget::currentRowChanged,
            this, &FT_FunctionGroup::onCurrentRowChanged);
}

void FT_FunctionGroup::applyLines(const FT_FunctionLines& lines)
{
    m_updating = true;

    while (QListWidget::count() > 0)
        removeRowInternal(QListWidget::count() - 1);

    for (const auto& row : lines) {
        if (!row.isEmpty())
            insertConfig(row.first(), QListWidget::count(), false);
    }

    m_updating = false;
    scheduleDelayedItemsLayout();
    scheduleNotifyResize();
    emit structureChanged();
    emit contentChanged();
}

FT_FunctionLines FT_FunctionGroup::toLines() const
{
    FT_FunctionLines lines;
    for (int i = 0; i < QListWidget::count(); ++i) {
        QListWidgetItem* item = QListWidget::item(i);
        FT_Function* f = qobject_cast<FT_Function*>(itemWidget(item));
        if (f) {
            lines.append(QVector<FT_FunctionData>{f->toConfig()});
        }
    }
    return lines;
}

int FT_FunctionGroup::count() const
{
    return QListWidget::count();
}

FT_Function* FT_FunctionGroup::functionAt(int index) const
{
    if (index < 0 || index >= QListWidget::count())
        return nullptr;
    return qobject_cast<FT_Function*>(itemWidget(QListWidget::item(index)));
}

int FT_FunctionGroup::indexOfFunction(FT_Function* function) const
{
    for (int i = 0; i < QListWidget::count(); ++i) {
        if (itemWidget(QListWidget::item(i)) == function)
            return i;
    }
    return -1;
}

FT_Function* FT_FunctionGroup::insertFunction(const QString& typeName, int index, bool /*hardBreak*/)
{
    FT_Function* f = createWidget(typeName);
    if (!f) return nullptr;

    const int actual = qBound(0, index, QListWidget::count());
    auto* item = newRowItem(f);
    QListWidget::insertItem(actual, item);
    setItemWidget(item, f);
    syncItemHeight(f);

    if (!m_updating) {
        scheduleNotifyResize();
        emit structureChanged();
        emit contentChanged();
    }
    return f;
}

FT_Function* FT_FunctionGroup::insertConfig(const FT_FunctionData& data, int index, bool /*hardBreak*/)
{
    const FtJson j = ftFunctionDataToJson(data);
    const QString typeName = ftStrField(j, "type");
    if (typeName.isEmpty()) return nullptr;

    FT_Function* f = FtFunctionFactory::create(typeName);
    if (!f) return nullptr;

    f->applyConfig(data);
    wireFunction(f);

    const int actual = qBound(0, index, QListWidget::count());
    auto* item = newRowItem(f);
    QListWidget::insertItem(actual, item);
    setItemWidget(item, f);
    syncItemHeight(f);

    if (!m_updating) {
        scheduleNotifyResize();
        emit structureChanged();
        emit contentChanged();
    }
    return f;
}

void FT_FunctionGroup::removeFunctionAt(int index)
{
    if (index < 0 || index >= QListWidget::count())
        return;

    removeRowInternal(index);
    scheduleDelayedItemsLayout();

    if (!m_updating) {
        scheduleNotifyResize();
        refreshMenuStates();
        emit structureChanged();
        emit contentChanged();
    }
}

FT_Function* FT_FunctionGroup::appendFunction(const QString& typeName)
{
    return insertFunction(typeName, QListWidget::count(), false);
}

QSize FT_FunctionGroup::sizeHint() const
{
    const int n = QListWidget::count();

    int totalH = 0;
    for (int i = 0; i < n; ++i) {
        QListWidgetItem* item = QListWidget::item(i);
        const int ih = (item && item->sizeHint().height() > 0)
            ? item->sizeHint().height()
            : kFunctionDefaultH + 2 * kGroupItemInset;
        totalH += ih;
    }

    // QListView::spacing 会在每个 item 四周都留出 spacing,
    // 每个 item 实际占高 = h + 2*spacing,故总高须加 2*spacing*n。
    totalH += 2 * kGroupRowSpacing * n;

    if (n == 0)
        totalH = kGroupMinH;
    else
        totalH = qMax(totalH, kGroupMinH);

    // 宽度只报最小宽度,绝不能返回 QListWidget::sizeHint().width()。
    // 那是"内容宽度",而内容宽度本身是由 viewport()->width() 算出来再写进
    // item->setSizeHint() 的;两者互相引用会形成正反馈:
    //   minimumSizeHint().width() ≈ 当前宽度 + 边框/滚动条余量 > 当前宽度
    //   → 父布局下一次必须再给它更宽 → 一路涨到超出步骤行,右侧就被裁掉
    // (而且这个尺寸会"粘住",只有重新布局才缓解,所以看起来像"改一下窗口就好了")。
    // 本控件的实际宽度由父布局(FT_FunctionItem 中 stretch=1)拉伸决定,这里只给下限。
    return QSize(kGroupMinWidth, totalH);
}

QSize FT_FunctionGroup::minimumSizeHint() const
{
    return sizeHint();
}

int FT_FunctionGroup::heightForWidth(int width) const
{
    Q_UNUSED(width);
    return sizeHint().height();
}

FtGroupHit FT_FunctionGroup::hitTest(const QPoint& pos) const
{
    FtGroupHit hit;
    QListWidgetItem* item = itemAt(pos);
    if (item) {
        hit.index = QListWidget::row(item);
        hit.valid = true;
    } else if (QListWidget::count() == 0) {
        hit.index = 0;
        hit.valid = true;
    }
    return hit;
}

// 与 FT_FunctionList::updateDropIndicator 同款:按光标落在行的上半/下半,
// 把指示线画在该行上沿或下沿,并用 m_dragOverRow 记下"松手后命令要插到第几行"。
void FT_FunctionGroup::updateDropIndicator(const QPoint& pos)
{
    if (!m_dropIndicator)
        return;

    const int vw = viewport()->width();
    const int ih = m_dropIndicator->height();

    if (QListWidget::count() == 0) {
        m_dropIndicator->setGeometry(0, 0, vw, ih);
        m_dropIndicator->raise();
        m_dropIndicator->show();
        m_dragOverRow = 0;
        return;
    }

    QListWidgetItem* at = itemAt(pos);
    if (!at) {
        // 落在最后一行下方/空白处 → 追加到末尾
        const int lastRow = QListWidget::count() - 1;
        const QRect r = visualItemRect(QListWidget::item(lastRow));
        m_dropIndicator->setGeometry(0, r.bottom() + 1, vw, ih);
        m_dropIndicator->raise();
        m_dropIndicator->show();
        m_dragOverRow = QListWidget::count();
        return;
    }

    const QRect r = visualItemRect(at);
    const int rowMid = r.center().y();
    const bool insertAbove = (pos.y() < rowMid);
    const int y = insertAbove ? r.top() - ih / 2 : r.bottom() + 1 - ih / 2;

    m_dropIndicator->setGeometry(0, y, vw, ih);
    m_dropIndicator->raise();
    m_dropIndicator->show();
    m_dragOverRow = QListWidget::row(at) + (insertAbove ? 0 : 1);
}

void FT_FunctionGroup::hideDropIndicator()
{
    if (m_dropIndicator)
        m_dropIndicator->hide();
    m_dragOverRow = -1;
}

void FT_FunctionGroup::startDrag(Qt::DropActions /*supportedActions*/)
{
    QListWidgetItem* item = currentItem();
    if (!item) return;

    const int row = QListWidget::row(item);
    FT_Function* f = functionAt(row);
    if (!f) return;

    ftSetCommandDragSource(this, row);

    // MIME 里放命令内容(JSON)而不是行号:
    // 拖到外层 FT_FunctionList 的步骤间隙松手时,FT_Edit::onCommandDroppedToGap 会直接解析它
    // 并新建一个 Step(源命令由其按拖拽源信息删除);组内/跨组落点仍由静态拖拽源信息处理。
    const FtJson j = ftFunctionDataToJson(f->toConfig());
    QMimeData* mimeData = new QMimeData();
    mimeData->setData(QLatin1String(kFtCommandMime),
                      QByteArray::fromStdString(j.dump()));

    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);

    ftClearCommandDragSource();
}

void FT_FunctionGroup::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* md = event->mimeData();
    if (md->hasFormat(QLatin1String(kFtNodeMime)) ||
        md->hasFormat(QLatin1String(kFtCommandMime))) {
        updateDropIndicator(event->position().toPoint());
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void FT_FunctionGroup::dragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* md = event->mimeData();
    if (md->hasFormat(QLatin1String(kFtNodeMime)) ||
        md->hasFormat(QLatin1String(kFtCommandMime))) {
        updateDropIndicator(event->position().toPoint());
        event->acceptProposedAction();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void FT_FunctionGroup::dragLeaveEvent(QDragLeaveEvent* event)
{
    hideDropIndicator();
    QListWidget::dragLeaveEvent(event);
}

void FT_FunctionGroup::dropEvent(QDropEvent* event)
{
    // 落点行以自绘指示线给出的 m_dragOverRow 为准(与用户看到的蓝线一致)。
    const int toRow = m_dragOverRow;
    hideDropIndicator();

    const QMimeData* md = event->mimeData();

    // 若未收到过 dragMove 事件(极少见),退回按坐标 hitTest。
    const auto resolvedIndex = [this, toRow, event]() -> int {
        if (toRow >= 0)
            return qBound(0, toRow, QListWidget::count());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const FtGroupHit hit = hitTest(event->position().toPoint());
#else
        const FtGroupHit hit = hitTest(event->pos());
#endif
        return hit.valid ? qBound(0, hit.index, QListWidget::count()) : -1;
    };

    if (md->hasFormat(QLatin1String(kFtNodeMime))) {
        const int nodeType = md->data(QLatin1String(kFtNodeMime)).toInt();
        const int idx = resolvedIndex();
        if (idx >= 0)
            emit nodeDroppedAt(nodeType, idx, false);
        event->acceptProposedAction();
        return;
    }

    if (md->hasFormat(QLatin1String(kFtCommandMime))) {
        // 组内重排 / 跨组搬运统一交给 FT_Edit::onCommandIntoGroup:
        // 那一层会先按拖拽源删掉原命令,同源时再修正目标行,所以这里只发落点行。
        const FtCommandDragSourceInfo src = ftCommandDragSource();
        FT_FunctionGroup* srcGroup = qobject_cast<FT_FunctionGroup*>(src.group);
        if (srcGroup && src.flat >= 0 && src.flat < srcGroup->count()) {
            FT_Function* sf = srcGroup->functionAt(src.flat);
            const int idx = resolvedIndex();
            if (sf && idx >= 0) {
                const FtJson j = ftFunctionDataToJson(sf->toConfig());
                const QByteArray payload = QByteArray::fromStdString(j.dump());
                emit commandDroppedAt(payload, idx, false);
                event->acceptProposedAction();
                return;
            }
        }
        event->ignore();
        return;
    }

    QListWidget::dropEvent(event);

    if (!m_updating) {
        syncAllItemHeights();
        scheduleNotifyResize();
        refreshMenuStates();
        emit structureChanged();
        emit contentChanged();
    }
}

QStringList FT_FunctionGroup::mimeTypes() const
{
    return {QLatin1String(kFtNodeMime), QLatin1String(kFtCommandMime)};
}

void FT_FunctionGroup::contextMenuEvent(QContextMenuEvent* event)
{
    // 右键先命中所点的那一行:组菜单里的“删除/拆分/向前插入”等都以当前行为准,
    // 而当前行现在会被别的组的点击清掉,所以必须按右键位置重新指定,否则菜单会指到空。
    const FtGroupHit hit = hitTest(event->pos());
    if (hit.valid && hit.index < QListWidget::count())
        QListWidget::setCurrentRow(hit.index);

    showGroupMenu(event->globalPos(), event->pos());
}

void FT_FunctionGroup::paintEvent(QPaintEvent* event)
{
    if (QListWidget::count() == 0) {
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing, true);

        p.setPen(QColor(160, 160, 160));
        QFont f = p.font();
        f.setPointSize(11);
        p.setFont(f);

        p.drawText(viewport()->rect(), Qt::AlignHCenter | Qt::AlignVCenter,
                   QStringLiteral("\u251C\u2500  添加命令步骤"));
    }

    QListWidget::paintEvent(event);
}

void FT_FunctionGroup::onItemClicked(QListWidgetItem* item)
{
    if (item) {
        QListWidget::setCurrentItem(item);
    }
}

void FT_FunctionGroup::onCurrentRowChanged()
{
    refreshMenuStates();
    if (QListWidget::currentRow() >= 0)
        makeSelectionExclusive();
}

// 每个命令组都是独立的 QListWidget,各自维护自己的 current/selection:
// 在 A 组点一行、再到 B 组点一行,两组会同时保持高亮(外层步骤的高亮也还在),
// 看上去像同时选中了好几处命令。这里在某组产生当前行时,清掉其它组的高亮,
// 并把当前行所属的步骤设为外层列表的当前项,保证全局同时只有一处高亮。
void FT_FunctionGroup::makeSelectionExclusive()
{
    // 注意:setItemWidget() 会把行控件挂到外层列表的 viewport 下,
    // 所以从行控件往上要跨过 viewport 才能拿到外层 QListWidget。
    QWidget* w = parentWidget();      // FT_FunctionItem
    QListWidget* outer = nullptr;
    for (int hop = 0; w && hop < 4; ++hop, w = w->parentWidget()) {
        if ((outer = qobject_cast<QListWidget*>(w)) != nullptr)
            break;
    }
    if (!outer) {
        FT_LOG("FG.exclusive", "outer QListWidget not found, skip");
        return;   // 独立使用(探针等)时不做协调
    }
    FT_LOG("FG.exclusive", QString("outer=%1 count=%2").arg(outer->metaObject()->className()).arg(outer->count()));

    QWidget* stepItem = parentWidget();
    for (int i = 0; i < outer->count(); ++i) {
        QListWidgetItem* outerItem = outer->item(i);
        QWidget* itemWidget = outer->itemWidget(outerItem);
        if (!itemWidget)
            continue;

        if (itemWidget == stepItem) {
            outer->setCurrentItem(outerItem);
            continue;
        }

        for (FT_FunctionGroup* g : itemWidget->findChildren<FT_FunctionGroup*>()) {
            if (g && g != this && g->currentRow() >= 0) {
                FT_LOG("FG.exclusive", QString("clear sibling group row=%1").arg(g->currentRow()));
                g->clearSelection();
                g->setCurrentRow(-1);
            }
        }
    }
}

QListWidgetItem* FT_FunctionGroup::newRowItem(FT_Function* f)
{
    auto* item = new QListWidgetItem();
    item->setSizeHint(QSize(0, kFunctionDefaultH + 2 * kGroupItemInset));
    return item;
}

void FT_FunctionGroup::syncItemHeight(FT_Function* f)
{
    const int idx = indexOfFunction(f);
    if (idx < 0) return;
    QListWidgetItem* item = QListWidget::item(idx);

    int w = viewport()->width();
    if (w <= 0) w = width();
    // 整格宽 = 视口宽 - 两侧 spacing(否则右侧被裁);
    // 行控件放在整格的内容矩形里,再各内缩 kGroupItemInset(见文件头的常量说明)。
    const int cellW = qMax(1, w - 2 * kGroupRowSpacing);
    const int rowW  = qMax(1, cellW - 2 * kGroupItemInset);
    FT_LOG("FG.syncItemHeight", QString("idx=%1 viewportW=%2 groupW=%3 cellW=%4 rowW=%5")
        .arg(idx).arg(viewport()->width()).arg(width()).arg(cellW).arg(rowW));
    if (w > 0) {
        // 同 syncAllItemHeights:只设上限,让视图随时能按新的 item 矩形把宽度缩回去
        f->setMaximumWidth(rowW);
        f->layout()->invalidate();
        f->layout()->activate();
        f->refreshChildHeights();
        const int h = qMax(f->sizeHint().height(), kFunctionDefaultH);
        FT_LOG("FG.syncItemHeight", QString("idx=%1 sizeHintH=%2 finalH=%3")
            .arg(idx).arg(f->sizeHint().height()).arg(h));
        f->resize(rowW, h);
        item->setSizeHint(QSize(cellW, h + 2 * kGroupItemInset));
    }
}

void FT_FunctionGroup::syncAllItemHeights()
{
    const int n = QListWidget::count();
    int w = viewport()->width();
    if (w <= 0) w = width();
    FT_LOG("FG.syncAll", QString("count=%1 viewportW=%2 groupW=%3 usingW=%4")
        .arg(n).arg(viewport()->width()).arg(width()).arg(w));
    if (w <= 0) return;

    const int cellW = qMax(1, w - 2 * kGroupRowSpacing);
    const int rowW  = qMax(1, cellW - 2 * kGroupItemInset);

    int totalItemH = 0;
    for (int i = 0; i < n; ++i) {
        FT_Function* f = qobject_cast<FT_Function*>(itemWidget(QListWidget::item(i)));
        if (!f) continue;
        // 用 setMaximumWidth 而不是 setFixedWidth:行控件是挂在视口下的,
        // 宽度由视图按 item 矩形设置。setFixedWidth 会把 min=max 都钉死,
        // 一旦某次同步读到了偏大的宽度,视图就再也无法把它改窄 → 右侧被裁。
        // 只设上限则任何时候视图都能按新矩形把它缩回去,不会"卡住"。
        f->setMaximumWidth(rowW);
        f->layout()->invalidate();
        f->layout()->activate();
        f->refreshChildHeights();
        const int h = qMax(f->sizeHint().height(), kFunctionDefaultH);
        f->resize(rowW, h);
        QListWidget::item(i)->setSizeHint(QSize(cellW, h + 2 * kGroupItemInset));
        totalItemH += h + 2 * kGroupItemInset;
        FT_LOG("FG.syncAll", QString("  [%1] f->w=%2 maxW=%3 sizeHint.h=%4 item.sizeHint.h=%5")
            .arg(i).arg(f->width()).arg(rowW).arg(f->sizeHint().height()).arg(h + 2 * kGroupItemInset));
    }
    const int spacingTotal = 2 * kGroupRowSpacing * n;
    const int qlistSH = QListWidget::sizeHint().height();
    FT_LOG("FG.syncAll", QString("totalItemH=%1 spacing=%2 QListWidget.sizeHint.h=%3")
        .arg(totalItemH).arg(spacingTotal).arg(qlistSH));
}

void FT_FunctionGroup::scheduleNotifyResize()
{
    if (m_notifyPending) { FT_LOG("FG.notify", "skip (pending)"); return; }
    m_notifyPending = true;
    FT_LOG("FG.notify", "posted singleShot");
    QTimer::singleShot(0, this, [this]() {
        m_notifyPending = false;
        FT_LOG("FG.notify", "singleShot fired");
        syncAllItemHeights();
        const QSize sh = sizeHint();
        FT_LOG("FG.notify", QString("after sync, sizeHint=(%1,%2)")
            .arg(sh.width()).arg(sh.height()));
        if (sh.height() > 0)
            setMinimumHeight(sh.height());
        scheduleDelayedItemsLayout();
        updateGeometries();
        updateGeometry();
        emit requestResize();
    });
}

bool FT_FunctionGroup::viewportEvent(QEvent* event)
{
    const bool handled = QListWidget::viewportEvent(event);

    // 行控件的宽度是按 viewport()->width() 算出来的,但视口自身的缩放
    // 不会走到 FT_FunctionGroup::resizeEvent(那只是本控件自己的尺寸变化)。
    // 最大化→还原时,视口会在本控件 resize 之后才落到最终宽度;
    // 若只靠 resizeEvent 触发,这个最终宽度就永远没有同步机会,
    // 行控件会停在旧(更宽)的 setFixedWidth 上,右侧被裁掉。
    // 视口的 Resize 事件到达时其宽度已是新值,直接在这里同步。
    if (event->type() == QEvent::Resize)
        syncAllItemHeights();

    return handled;
}

void FT_FunctionGroup::resizeEvent(QResizeEvent* event)
{
    QListWidget::resizeEvent(event);
    FT_LOG("FG.resizeEvent", QString("oldW=%1 newW=%2 oldH=%3 newH=%4")
        .arg(event->oldSize().width()).arg(event->size().width())
        .arg(event->oldSize().height()).arg(event->size().height()));
    // 这里不能直接 syncAllItemHeights():本控件的 resizeEvent 早于视口重排,
    // 此刻 viewport()->width() 还是旧值,按它同步会把行控件设成旧的宽度
    // (最大化后还原时右侧就被裁掉)。统一走延迟同步,那时宽度已正确,
    // 且连续 resize 会被 scheduleNotifyResize 的 pending 标记合并成一次。
    scheduleNotifyResize();
}

void FT_FunctionGroup::showEvent(QShowEvent* event)
{
    QListWidget::showEvent(event);
    FT_LOG("FG.showEvent", QString("groupW=%1 groupH=%2 viewportW=%3")
        .arg(width()).arg(height()).arg(viewport()->width()));
    syncAllItemHeights();
    scheduleNotifyResize();
}

FT_Function* FT_FunctionGroup::createWidget(const QString& typeName)
{
    FT_Function* f = FtFunctionFactory::create(typeName);
    if (f) wireFunction(f);
    return f;
}

void FT_FunctionGroup::wireFunction(FT_Function* function)
{
    connect(function, &FT_Function::requestResize, this, [this, function]() {
        if (m_updating) return;
        syncItemHeight(function);
        scheduleNotifyResize();
    });

    connect(function, &FT_Function::requestRemove, this, [this, function]() {
        const int idx = indexOfFunction(function);
        if (idx >= 0) removeFunctionAt(idx);
    });

    connect(function, &FT_Function::requestMoveUp, this, [this, function]() {
        const int idx = indexOfFunction(function);
        if (idx > 0) {
            m_updating = true;
            QListWidgetItem* item = QListWidget::takeItem(idx);
            QListWidget::insertItem(idx - 1, item);
            m_updating = false;
            scheduleNotifyResize();
            emit structureChanged();
            emit contentChanged();
        }
    });

    connect(function, &FT_Function::requestMoveDown, this, [this, function]() {
        const int idx = indexOfFunction(function);
        if (idx >= 0 && idx < QListWidget::count() - 1) {
            m_updating = true;
            QListWidgetItem* item = QListWidget::takeItem(idx);
            QListWidget::insertItem(idx + 1, item);
            m_updating = false;
            scheduleNotifyResize();
            emit structureChanged();
            emit contentChanged();
        }
    });

    connect(function, &FT_Function::requestSplitNewStep, this, [this, function]() {
        const int idx = indexOfFunction(function);
        if (idx >= 0) emit splitStepRequested(idx);
    });

    connect(function, &FT_Function::requestInsertAfter,
            this, [this, function](const QString& typeName) {
        const int idx = indexOfFunction(function);
        if (idx >= 0) insertFunction(typeName, idx + 1, false);
    });

    connect(function, &FT_Function::contentChanged, this, [this]() {
        if (!m_updating) emit contentChanged();
    });
}

void FT_FunctionGroup::removeRowInternal(int row)
{
    QListWidgetItem* item = QListWidget::takeItem(row);
    if (item) {
        QWidget* w = itemWidget(item);
        if (w) w->deleteLater();
        delete item;
    }
}

void FT_FunctionGroup::refreshMenuStates()
{
    for (int i = 0; i < QListWidget::count(); ++i) {
        FT_Function* f = functionAt(i);
        if (!f) continue;

        FT_Function::MenuState state;
        state.canMoveUp    = (i > 0);
        state.canMoveDown  = (i < QListWidget::count() - 1);
        state.canSplitStep = (i > 0);
        f->setMenuState(state);
    }
}

void FT_FunctionGroup::showGroupMenu(const QPoint& globalPos, const QPoint& localPos)
{
    Q_UNUSED(localPos);

    // 组空白区只负责“往这个组里加命令”和“再开一个步骤”。
    // 单条命令的上移/下移/拆分/删除等条目级操作统一放在命令菜单里(FT_Function::showCommandMenu),
    // 避免两个入口互相重复、行为又不一致。
    QMenu menu(this);

    QAction* actAppendTbox = menu.addAction(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add TBox Command"));
    connect(actAppendTbox, &QAction::triggered, this, [this]() {
        appendFunction(QString::fromLatin1(kFtTypeTbox));
    });

    QAction* actAppendIicW = menu.addAction(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add I2C Write"));
    connect(actAppendIicW, &QAction::triggered, this, [this]() {
        appendFunction(QString::fromLatin1(kFtTypeIicWrite));
    });

    QAction* actAppendIicWR = menu.addAction(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add I2C Write+Read"));
    connect(actAppendIicWR, &QAction::triggered, this, [this]() {
        appendFunction(QString::fromLatin1(kFtTypeIicWriteRead));
    });

    menu.addSeparator();

    QAction* actNewStep = menu.addAction(
        style()->standardIcon(QStyle::SP_FileDialogNewFolder),
        tr("New Empty Step After"));
    connect(actNewStep, &QAction::triggered, this, [this]() {
        emit requestInsertStepAfter();
    });

    menu.exec(globalPos);
}