#include "FT_FunctionGroup.h"
#include "FT_Function.h"
#include "FT_Data.h"

#include <QWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QLabel>

namespace {
constexpr int kGroupMarginH   = 2;
constexpr int kGroupMarginV   = 4;
constexpr int kHSpace         = 8;
constexpr int kVSpace         = 8;
constexpr int kGroupMinH      = 44;
constexpr int kFuncMinH       = 36;
constexpr int kHardZoneWidth  = 10;
constexpr int kCaretThick     = 3;
}

// ----------------------------------------------------------------
// 全局拖拽源(三个访问器共享同一份状态)
// ----------------------------------------------------------------
namespace {
FtCommandDragSourceInfo& commandDragSourceRef()
{
    static FtCommandDragSourceInfo info;
    return info;
}
}

FtCommandDragSourceInfo ftCommandDragSource()
{
    return commandDragSourceRef();
}

void ftSetCommandDragSource(QWidget* group, int flat)
{
    commandDragSourceRef() = {group, flat};
}

void ftClearCommandDragSource()
{
    commandDragSourceRef() = {};
}

// ================================================================
// FtFlowBreakLayout
// ================================================================
FtFlowBreakLayout::FtFlowBreakLayout(QWidget* parent)
    : QLayout(parent)
{
    setContentsMargins(kGroupMarginH, kGroupMarginV, kGroupMarginH, kGroupMarginV);
    setSpacing(kHSpace);
}

FtFlowBreakLayout::~FtFlowBreakLayout()
{
    qDeleteAll(m_items);
}

void FtFlowBreakLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
    invalidate();
}

QLayoutItem* FtFlowBreakLayout::itemAt(int index) const
{
    return (index >= 0 && index < m_items.size()) ? m_items.at(index) : nullptr;
}

QLayoutItem* FtFlowBreakLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    QLayoutItem* item = m_items.takeAt(index);
    if (QWidget* w = item->widget())
        m_breaks.remove(w);
    invalidate();
    return item;
}

int FtFlowBreakLayout::count() const
{
    return m_items.size();
}

QSize FtFlowBreakLayout::minimumSize() const
{
    return QSize(0, m_cachedHeight);
}

QSize FtFlowBreakLayout::sizeHint() const
{
    return QSize(0, m_cachedHeight);
}

Qt::Orientations FtFlowBreakLayout::expandingDirections() const
{
    return Qt::Horizontal | Qt::Vertical;
}

void FtFlowBreakLayout::insertWidgetItem(int index, QWidget* widget)
{
    index = qBound(0, index, m_items.size());
    addChildWidget(widget);
    m_items.insert(index, new QWidgetItem(widget));
    invalidate();
}

QWidget* FtFlowBreakLayout::widgetAt(int index) const
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    return m_items.at(index)->widget();
}

int FtFlowBreakLayout::indexOfWidget(QWidget* widget) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i)->widget() == widget)
            return i;
    }
    return -1;
}

void FtFlowBreakLayout::moveItem(int from, int to)
{
    if (from < 0 || from >= m_items.size())
        return;
    to = qBound(0, to, m_items.size() - 1);
    if (from == to)
        return;
    m_items.move(from, to);
    invalidate();
}

void FtFlowBreakLayout::setBreakBefore(QWidget* widget, bool on)
{
    if (!widget)
        return;
    if (on)
        m_breaks.insert(widget);
    else
        m_breaks.remove(widget);
    invalidate();
}

bool FtFlowBreakLayout::breakBefore(QWidget* widget) const
{
    return m_breaks.contains(widget);
}

FtFlowBreakLayout::Arranged FtFlowBreakLayout::arrange(const QRect& rect, bool applyGeometry) const
{
    Arranged a;
    const int n = m_items.size();
    a.rects.fill(QRect(), n);
    const QMargins mg = contentsMargins();

    if (n == 0 || rect.width() <= 0) {
        a.height = qMax(kGroupMinH, mg.top() + mg.bottom());
        return a;
    }

    const int contentW  = qMax(0, rect.width() - mg.left() - mg.right());
    const int rightBound = mg.left() + contentW;

    QVector<int> minW(n), minH(n), originX(n);
    for (int i = 0; i < n; ++i) {
        const QSize s = m_items.at(i)->minimumSize();
        minW[i] = qMax(1, s.width());
        minH[i] = qMax(kFuncMinH, s.height());
    }

    // ---- 第一遍:贪心分行(软折行 + 硬断点) ----
    QList<QVector<int>> rows;
    QVector<int> cur;
    int x    = mg.left();
    int rowH = 0;
    for (int i = 0; i < n; ++i) {
        const bool forced = m_breaks.contains(m_items.at(i)->widget());
        if (!cur.isEmpty()) {
            const bool wrap = forced || (x + kHSpace + minW[i] > rightBound);
            if (wrap) {
                rows.append(cur);
                cur.clear();
                x = mg.left();
                rowH = 0;
            }
        }
        originX[i] = x;
        x += minW[i] + kHSpace;
        rowH = qMax(rowH, minH[i]);
        cur.append(i);
    }
    if (!cur.isEmpty())
        rows.append(cur);

    // ---- 第二遍:行内分配多余宽度(所有命令均分,内部编辑器自动伸展) ----
    a.rows = rows;
    int y = mg.top();
    for (const QVector<int>& row : rows) {
        int sumMin = 0;
        for (int idx : row)
            sumMin += minW[idx];
        sumMin += (row.size() - 1) * kHSpace;
        const int extra = qMax(0, contentW - sumMin);
        const int base  = row.size() > 0 ? extra / row.size() : 0;
        int remain      = row.size() > 0 ? extra % row.size() : 0;

        int xx = mg.left();
        const int h = rowH;
        for (int k = 0; k < row.size(); ++k) {
            const int idx = row[k];
            int ww = minW[idx] + base + (remain > 0 ? 1 : 0);
            if (remain > 0)
                --remain;
            a.rects[idx] = QRect(xx, y, ww, h);
            xx += ww + kHSpace;
        }
        y += h + kVSpace;
    }

    a.height = qMax(kGroupMinH, y - kVSpace + mg.bottom());

    if (applyGeometry) {
        for (int i = 0; i < n; ++i) {
            if (a.rects[i].isValid())
                m_items.at(i)->setGeometry(a.rects[i]);
        }
    }
    return a;
}

void FtFlowBreakLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    if (rect.width() <= 0)
        return;

    // 第一遍设置宽度后,文本可能重排改变最小高度,故再排第二遍收敛。
    arrange(rect, true);
    const Arranged a = arrange(rect, true);
    m_last = a;

    if (m_cachedHeight != a.height) {
        m_cachedHeight = a.height;
        emit heightChanged(a.height);
    }
}

FtGroupHit FtFlowBreakLayout::hitTest(const QPoint& pos) const
{
    FtGroupHit hit;
    hit.valid = true;
    const int n = m_items.size();
    if (n == 0 || m_last.rows.isEmpty()) {
        hit.flat = 0;
        hit.hardBreak = pos.y() > m_cachedHeight / 2;
        return hit;
    }

    const QMargins mg = contentsMargins();

    auto rowRect = [this](const QVector<int>& row) {
        QRect r = m_last.rects.value(row.first());
        for (int idx : row)
            r = r.united(m_last.rects.value(idx));
        return r;
    };

    const QRect firstRow = rowRect(m_last.rows.first());
    const QRect lastRow  = rowRect(m_last.rows.last());

    if (pos.y() < firstRow.top() - kVSpace / 2) {
        hit.flat = 0;
        hit.hardBreak = true;
        return hit;
    }
    if (pos.y() > lastRow.bottom() + kVSpace / 2) {
        hit.flat = n;
        hit.hardBreak = true;
        return hit;
    }

    int prevBottom = firstRow.top();
    for (int r = 0; r < m_last.rows.size(); ++r) {
        const QVector<int>& row = m_last.rows[r];
        const QRect rr = rowRect(row);

        // 与上一行之间的间隙 → 在本行首条命令前硬换行
        if (r > 0 && pos.y() <= (prevBottom + rr.top()) / 2) {
            hit.flat = row.first();
            hit.hardBreak = true;
            return hit;
        }

        if (pos.y() <= rr.bottom()) {
            // 行左缘硬换行热区
            if (pos.x() <= mg.left() + kHardZoneWidth) {
                hit.flat = row.first();
                hit.hardBreak = hit.flat > 0;
                return hit;
            }
            for (int idx : row) {
                const QRect cr = m_last.rects.value(idx);
                if (pos.x() < cr.center().x()) {
                    hit.flat = idx;
                    hit.hardBreak = false;
                    return hit;
                }
            }
            hit.flat = row.last() + 1;
            hit.hardBreak = false;
            return hit;
        }
        prevBottom = rr.bottom();
    }

    hit.flat = n;
    hit.hardBreak = false;
    return hit;
}

QRect FtFlowBreakLayout::caretRect(const FtGroupHit& hit) const
{
    if (!hit.valid || m_items.isEmpty() || m_last.rows.isEmpty())
        return QRect();

    const QMargins mg = contentsMargins();
    const int contentW = qMax(0, geometry().width() - mg.left() - mg.right());

    auto rowRectOf = [this](const QVector<int>& row) {
        QRect r = m_last.rects.value(row.first());
        for (int idx : row)
            r = r.united(m_last.rects.value(idx));
        return r;
    };

    if (hit.hardBreak) {
        int y = 0;
        if (hit.flat <= 0) {
            y = rowRectOf(m_last.rows.first()).top() - kVSpace / 2;
        } else if (hit.flat >= m_items.size()) {
            y = rowRectOf(m_last.rows.last()).bottom() + kVSpace / 2;
        } else {
            for (const QVector<int>& row : m_last.rows) {
                if (row.contains(hit.flat)) {
                    y = rowRectOf(row).top() - kVSpace / 2;
                    break;
                }
            }
        }
        return QRect(mg.left(), y - kCaretThick / 2, contentW, kCaretThick);
    }

    // 行内竖线
    const int n = m_items.size();
    for (const QVector<int>& row : m_last.rows) {
        if (hit.flat <= row.last()) {
            int x = 0;
            int h = 0;
            int top = 0;
            if (hit.flat < n && row.contains(hit.flat)) {
                const QRect cr = m_last.rects.value(hit.flat);
                x = cr.left() - kHSpace / 2;
                h = cr.height();
                top = cr.top();
            } else if (hit.flat == n && row.last() == n - 1) {
                const QRect cr = m_last.rects.value(n - 1);
                x = cr.right() + kHSpace / 2;
                h = cr.height();
                top = cr.top();
            } else {
                continue;
            }
            return QRect(x - kCaretThick / 2, top, kCaretThick, h);
        }
    }
    return QRect();
}

// ================================================================
// FT_FunctionGroup
// ================================================================
FT_FunctionGroup::FT_FunctionGroup(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(kGroupMinH);
    setAcceptDrops(true);

    m_layout = new FtFlowBreakLayout(this);
    connect(m_layout, &FtFlowBreakLayout::heightChanged,
            this, &FT_FunctionGroup::handleLayoutHeight);
}

QSize FT_FunctionGroup::sizeHint() const
{
    return QSize(0, qMax(kGroupMinH, m_layout->contentHeight()));
}

QSize FT_FunctionGroup::minimumSizeHint() const
{
    return sizeHint();
}

int FT_FunctionGroup::count() const
{
    return m_layout->count();
}

FT_Function* FT_FunctionGroup::functionAt(int flat) const
{
    return qobject_cast<FT_Function*>(m_layout->widgetAt(flat));
}

int FT_FunctionGroup::indexOfFunction(FT_Function* function) const
{
    return function ? m_layout->indexOfWidget(function) : -1;
}

FT_Function* FT_FunctionGroup::createWidget(const QString& typeName)
{
    FT_Function* f = FtFunctionFactory::create(typeName);
    if (f) {
        f->setParent(this);
        f->show();
    }
    return f;
}

void FT_FunctionGroup::wireFunction(FT_Function* function)
{
    connect(function, &FT_Function::contentChanged, this, &FT_FunctionGroup::contentChanged);
    connect(function, &FT_Function::requestResize, this, [this]() {
        if (m_arranging)
            m_relayoutRequested = true;
        else
            relayout();
    });
    connect(function, &FT_Function::requestRemove, this, [this, function]() {
        const int flat = indexOfFunction(function);
        if (flat >= 0)
            removeFunctionAt(flat);
    });
    connect(function, &FT_Function::requestMoveUp, this, [this, function]() {
        const int flat = indexOfFunction(function);
        if (flat > 0)
            moveFlat(flat, flat - 1, m_layout->breakBefore(function));
    });
    connect(function, &FT_Function::requestMoveDown, this, [this, function]() {
        const int flat = indexOfFunction(function);
        if (flat >= 0 && flat < count() - 1)
            moveFlat(flat, flat + 1, m_layout->breakBefore(function));
    });
    connect(function, &FT_Function::requestWrap, this, [this, function]() {
        const int flat = indexOfFunction(function);
        if (flat > 0)
            setBreakBefore(flat, true);
    });
    connect(function, &FT_Function::requestUnwrap, this, [this, function]() {
        const int flat = indexOfFunction(function);
        if (flat >= 0)
            setBreakBefore(flat, false);
    });
    connect(function, &FT_Function::requestSplitNewStep, this, [this, function]() {
        const int flat = indexOfFunction(function);
        if (flat > 0)
            emit splitStepRequested(flat);
    });
    connect(function, &FT_Function::requestInsertAfter, this,
            [this, function](const QString& typeName) {
        const int flat = indexOfFunction(function);
        if (flat >= 0)
            insertFunction(typeName, flat + 1, false);
    });
}

void FT_FunctionGroup::installDragFilter(FT_Function* function)
{
    function->installEventFilter(this);
    const QList<QLabel*> labels = function->findChildren<QLabel*>();
    for (QLabel* label : labels)
        label->installEventFilter(this);
}

void FT_FunctionGroup::refreshMenuStates()
{
    const int n = count();
    for (int i = 0; i < n; ++i) {
        FT_Function* f = functionAt(i);
        if (!f)
            continue;
        FT_Function::MenuState st;
        st.canMoveUp    = (i > 0);
        st.canMoveDown  = (i < n - 1);
        const bool br   = m_layout->breakBefore(f);
        st.canWrap      = (i > 0 && !br);
        st.canUnwrap    = br;
        st.canSplitStep = (i > 0);
        f->setMenuState(st);
    }
}

void FT_FunctionGroup::relayout()
{
    m_arranging = true;
    m_layout->setGeometry(QRect(0, 0, qMax(width(), 0), qMax(height(), kGroupMinH)));
    m_arranging = false;

    const int h = m_layout->contentHeight();
    if (h != height()) {
        setFixedHeight(h);
        updateGeometry();
        emit requestResize();
    }

    if (m_relayoutRequested) {
        m_relayoutRequested = false;
        QTimer::singleShot(0, this, [this]() { relayout(); });
    }
}

void FT_FunctionGroup::handleLayoutHeight(int height)
{
    if (m_arranging) {
        m_relayoutRequested = true;
        return;
    }
    if (height != this->height()) {
        setFixedHeight(height);
        updateGeometry();
        emit requestResize();
    }
}

FT_Function* FT_FunctionGroup::insertFunction(const QString& typeName, int flat, bool hardBreak)
{
    FT_Function* f = createWidget(typeName);
    if (!f)
        return nullptr;

    wireFunction(f);
    const int index = qBound(0, flat, count());
    m_layout->insertWidgetItem(index, f);
    installDragFilter(f);
    if (hardBreak && index > 0)
        m_layout->setBreakBefore(f, true);

    refreshMenuStates();
    relayout();
    emit structureChanged();
    emit contentChanged();
    return f;
}

FT_Function* FT_FunctionGroup::insertConfig(const FT_FunctionData& data, int flat, bool hardBreak)
{
    const QString typeName = ftFunctionDataTypeName(data);
    if (typeName == QLatin1String("Empty"))
        return nullptr;

    FT_Function* f = insertFunction(typeName, flat, hardBreak);
    if (f)
        f->applyConfig(data);
    return f;
}

void FT_FunctionGroup::removeFunctionAt(int flat)
{
    FT_Function* f = functionAt(flat);
    if (!f)
        return;

    QLayoutItem* item = m_layout->takeAt(flat);
    delete item;
    f->removeEventFilter(this);
    // deleteLater 在下次事件循环才真正析构,期间控件仍是可见子控件,
    // 先隐藏避免它以旧几何残影绘制到新布局上。
    f->hide();
    f->deleteLater();

    refreshMenuStates();
    relayout();
    emit structureChanged();
    emit contentChanged();
}

void FT_FunctionGroup::setBreakBefore(int flat, bool on)
{
    FT_Function* f = functionAt(flat);
    if (!f)
        return;
    m_layout->setBreakBefore(f, on);
    refreshMenuStates();
    relayout();
    emit structureChanged();
    emit contentChanged();
}

void FT_FunctionGroup::moveFlat(int fromFlat, int toFlat, bool hardBreak)
{
    FT_Function* f = functionAt(fromFlat);
    if (!f)
        return;

    int to = qBound(0, toFlat, count() - 1);
    if (to == fromFlat)
        return;

    m_layout->moveItem(fromFlat, to);
    m_layout->setBreakBefore(f, hardBreak && to > 0);

    refreshMenuStates();
    relayout();
    emit structureChanged();
    emit contentChanged();
}

FT_Function* FT_FunctionGroup::appendFunction(const QString& typeName)
{
    return insertFunction(typeName, count(), false);
}

void FT_FunctionGroup::applyLines(const FT_FunctionLines& lines)
{
    while (m_layout->count() > 0) {
        QLayoutItem* item = m_layout->takeAt(0);
        QWidget* w = item ? item->widget() : nullptr;
        delete item;
        if (w) {
            w->removeEventFilter(this);
            w->hide();
            w->deleteLater();
        }
    }

    for (int li = 0; li < lines.size(); ++li) {
        const QVector<FT_FunctionData>& line = lines[li];
        for (int ci = 0; ci < line.size(); ++ci) {
            const FT_FunctionData& data = line[ci];
            if (ftFunctionDataTypeName(data) == QLatin1String("Empty"))
                continue;
            FT_Function* f = createWidget(ftFunctionDataTypeName(data));
            if (!f)
                continue;
            wireFunction(f);
            m_layout->insertWidgetItem(m_layout->count(), f);
            installDragFilter(f);
            if (li > 0 && ci == 0)
                m_layout->setBreakBefore(f, true);
            f->applyConfig(data);
        }
    }

    refreshMenuStates();
    relayout();
    m_caret.valid = false;
    update();
}

FT_FunctionLines FT_FunctionGroup::toLines() const
{
    FT_FunctionLines lines;
    const int n = count();
    for (int i = 0; i < n; ++i) {
        FT_Function* f = functionAt(i);
        if (!f)
            continue;
        if (i == 0 || m_layout->breakBefore(f))
            lines.append(QVector<FT_FunctionData>{});
        if (lines.isEmpty())
            lines.append(QVector<FT_FunctionData>{});
        const FT_FunctionData data = f->toConfig();
        if (ftFunctionDataTypeName(data) != QLatin1String("Empty"))
            lines.last().append(data);
    }
    return lines;
}

void FT_FunctionGroup::showGroupMenu(const QPoint& globalPos, const QPoint& localPos)
{
    // 落点决定插入位置:行间间隙/行左缘 → 新硬行;命令之间 → 行内;
    // 最后一行之后 → 组尾另起一行。
    const FtGroupHit hit = hitTest(localPos);

    QMenu menu(this);
    QAction* addTbox = menu.addAction(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add TBox Command"));
    QAction* addI2cW = menu.addAction(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add I2C Write"));
    QAction* addI2cWR = menu.addAction(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Add I2C Write+Read"));

    menu.addSeparator();
    QAction* newEmptyStep = menu.addAction(
        style()->standardIcon(QStyle::SP_FileDialogNewFolder),
        tr("New Empty Step After"));
    QAction* mergeNext = menu.addAction(
        style()->standardIcon(QStyle::SP_ArrowBack),
        tr("Merge with Next Step"));

    QAction* chosen = menu.exec(globalPos);
    if (!chosen)
        return;

    if (chosen == addTbox)
        insertFunction(QString::fromLatin1(kFtTypeTbox), hit.flat, hit.hardBreak);
    else if (chosen == addI2cW)
        insertFunction(QString::fromLatin1(kFtTypeIicWrite), hit.flat, hit.hardBreak);
    else if (chosen == addI2cWR)
        insertFunction(QString::fromLatin1(kFtTypeIicWriteRead), hit.flat, hit.hardBreak);
    else if (chosen == newEmptyStep)
        emit requestInsertStepAfter();
    else if (chosen == mergeNext)
        emit requestMergeWithNext();
}

void FT_FunctionGroup::contextMenuEvent(QContextMenuEvent* event)
{
    showGroupMenu(event->globalPos(), event->pos());
    event->accept();
}

void FT_FunctionGroup::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && count() == 0) {
        appendFunction(FtFunctionFactory::defaultTypeName());
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void FT_FunctionGroup::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (count() == 0) {
        const QRect r = rect().adjusted(2, 2, -2, -2);
        QPen pen(QColor(150, 150, 150));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(1);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 4, 4);

        p.setPen(QColor(150, 150, 150));
        p.drawText(r, Qt::AlignCenter,
                   tr("Double-click or right-click to add a command"));
    }

    if (m_caret.valid) {
        const QRect cr = m_layout->caretRect(m_caret);
        if (cr.isValid()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(74, 144, 217));
            p.drawRoundedRect(cr, 1.5, 1.5);
        }
    }
}

FtGroupHit FT_FunctionGroup::hitTest(const QPoint& pos) const
{
    return m_layout->hitTest(pos);
}

void FT_FunctionGroup::showDropCaret(const FtGroupHit& hit)
{
    m_caret = hit;
    update();
}

void FT_FunctionGroup::hideDropCaret()
{
    m_caret.valid = false;
    update();
}

void FT_FunctionGroup::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* md = event->mimeData();
    if (md->hasFormat(kFtNodeMime) || md->hasFormat(kFtCommandMime)) {
        event->acceptProposedAction();
        showDropCaret(hitTest(event->position().toPoint()));
        return;
    }
    event->ignore();
}

void FT_FunctionGroup::dragMoveEvent(QDragMoveEvent* event)
{
    const QMimeData* md = event->mimeData();
    if (md->hasFormat(kFtNodeMime) || md->hasFormat(kFtCommandMime)) {
        event->acceptProposedAction();
        showDropCaret(hitTest(event->position().toPoint()));
        return;
    }
    event->ignore();
}

void FT_FunctionGroup::dragLeaveEvent(QDragLeaveEvent* event)
{
    hideDropCaret();
    event->accept();
}

void FT_FunctionGroup::dropEvent(QDropEvent* event)
{
    const QMimeData* md = event->mimeData();
    const FtGroupHit hit = hitTest(event->position().toPoint());
    hideDropCaret();

    if (md->hasFormat(kFtNodeMime)) {
        const int nodeType = md->data(QLatin1String(kFtNodeMime)).toInt();
        emit nodeDroppedAt(nodeType, hit.flat, hit.hardBreak);
        event->acceptProposedAction();
        return;
    }
    if (md->hasFormat(kFtCommandMime)) {
        const QByteArray payload = md->data(QLatin1String(kFtCommandMime));
        emit commandDroppedAt(payload, hit.flat, hit.hardBreak);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

bool FT_FunctionGroup::eventFilter(QObject* watched, QEvent* event)
{
    QWidget* w = qobject_cast<QWidget*>(watched);
    FT_Function* f = w ? qobject_cast<FT_Function*>(w) : nullptr;
    if (!f && w)
        f = qobject_cast<FT_Function*>(w->parentWidget());
    if (f && indexOfFunction(f) < 0)
        f = nullptr;

    if (f) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_pressFunction = f;
                m_pressPos = me->globalPosition().toPoint();
            }
            break;
        }
        case QEvent::MouseMove: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (m_pressFunction == f && (me->buttons() & Qt::MouseButton::LeftButton)) {
                const QPoint global = me->globalPosition().toPoint();
                if ((global - m_pressPos).manhattanLength() >=
                    QApplication::startDragDistance()) {
                    beginCommandDrag(f, f->mapFromGlobal(m_pressPos));
                    return true;
                }
            }
            break;
        }
        case QEvent::MouseButtonRelease:
            if (m_pressFunction == f)
                m_pressFunction = nullptr;
            break;
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void FT_FunctionGroup::beginCommandDrag(FT_Function* function, const QPoint& pressPos)
{
    const int flat = indexOfFunction(function);
    if (flat < 0)
        return;

    const FT_FunctionData data = function->toConfig();
    const QByteArray payload =
        QByteArray::fromStdString(ftFunctionDataToJson(data).dump());

    auto* md = new QMimeData;
    md->setData(QLatin1String(kFtCommandMime), payload);

    auto* drag = new QDrag(this);
    drag->setMimeData(md);
    drag->setPixmap(function->grab());
    drag->setHotSpot(pressPos);

    m_pressFunction = nullptr;
    ftSetCommandDragSource(this, flat);
    drag->exec(Qt::MoveAction);
    ftClearCommandDragSource();
    hideDropCaret();
    drag->deleteLater();
}
