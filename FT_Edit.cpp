#include "FT_Edit.h"
#include "FT_FunctionWidgetItem.h"

#include <QListWidgetItem>
#include <QMenu>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QContextMenuEvent>
#include <utility>

FT_Edit::FT_Edit(QWidget *parent)
    : QListWidget(parent)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setUniformItemSizes(false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    setAutoScroll(true);
#endif

    connect(this, &QListWidget::customContextMenuRequested,
            this, &FT_Edit::showContextMenu);
}

static void rewriteAllItemSizeHints(FT_Edit *list)
{
    const int viewW = list->viewport() ? list->viewport()->width() : 900;
    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem *it = list->item(i);
        if (!it) continue;
        auto *w = qobject_cast<FT_FunctionWidgetItem *>(list->itemWidget(it));
        if (!w) continue;

        w->ensurePolished();
        const QSize sz = w->sizeHint();

        const int wantW = qMax(viewW, w->minimumWidth());
        const int wantH = qMax(sz.isValid() ? sz.height() : 0, w->minimumHeight());

        if (wantW <= 0 || wantH <= 0) continue;

        const QSize wantSz(wantW, wantH);
        it->setSizeHint(wantSz);

        if (w->size() != wantSz) {
            w->resize(wantSz);
            w->updateGeometry();
        }
    }
}

FT_FunctionWidgetItem *FT_Edit::addSchedule(const QString &title)
{
    auto *itemWidget = new FT_FunctionWidgetItem;
    itemWidget->setTitle(title.isEmpty()
                             ? tr("Schedule %1").arg(count() + 1)
                             : title);

    auto *listItem = new QListWidgetItem;
    addItem(listItem);
    setItemWidget(listItem, itemWidget);
    listItem->setSizeHint(itemWidget->sizeHint());
    attachItem(itemWidget, listItem);

    rewriteAllItemSizeHints(this);
    scheduleDelayedItemsLayout();
    doItemsLayout();
    viewport()->update();

    emit scheduleAdded(itemWidget);
    return itemWidget;
}

void FT_Edit::attachItem(FT_FunctionWidgetItem *fwItem, QListWidgetItem *listItem)
{
    connect(fwItem, &FT_FunctionWidgetItem::sizeHintChanged, this,
            [this, listItem, fwItem] {
                fwItem->ensurePolished();
                const QSize sz = fwItem->sizeHint();
                const int viewW = viewport() ? viewport()->width() : fwItem->width();
                const int wantW = qMax(viewW, fwItem->minimumWidth());
                const int wantH = qMax(sz.isValid() ? sz.height() : 0, fwItem->minimumHeight());
                if (wantW > 0 && wantH > 0) {
                    listItem->setSizeHint(QSize(wantW, wantH));
                    fwItem->resize(wantW, wantH);
                }
                rewriteAllItemSizeHints(this);
                scheduleDelayedItemsLayout();
                doItemsLayout();
                viewport()->update();
            });
    connect(fwItem, &FT_FunctionWidgetItem::contextMenuRequested,
            this, &FT_Edit::showContextMenuAt);
}

void FT_Edit::resizeEvent(QResizeEvent *e)
{
    QListWidget::resizeEvent(e);
    if (count() > 0) {
        rewriteAllItemSizeHints(this);
        scheduleDelayedItemsLayout();
        doItemsLayout();
        viewport()->update();
    }
}

QList<FT_FunctionWidgetItem *> FT_Edit::schedules() const
{
    QList<FT_FunctionWidgetItem *> result;
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *it = QListWidget::item(i);
        auto *w = qobject_cast<FT_FunctionWidgetItem *>(QListWidget::itemWidget(it));
        if (w)
            result.append(w);
    }
    return result;
}

void FT_Edit::showContextMenu(const QPoint &pos)
{
    showContextMenuAt(viewport()->mapToGlobal(pos));
}

void FT_Edit::showContextMenuAt(const QPoint &globalPos)
{
    QMenu menu(this);
    QAction *addAct = menu.addAction(tr("Add Schedule"));
    QAction *delAct = menu.addAction(tr("Remove Schedule"));

    QAction *chosen = menu.exec(globalPos);
    if (chosen == addAct) {
        addSchedule();
    } else if (chosen == delAct) {
        const auto sel = selectedItems();
        for (QListWidgetItem *it : sel) {
            if (auto *w = qobject_cast<FT_FunctionWidgetItem *>(QListWidget::itemWidget(it)))
                emit scheduleRemoved(w);
            delete it;
        }
    }
}

QString FT_Edit::toConfig() const
{
    QJsonArray arr;
    for (FT_FunctionWidgetItem *s : schedules())
        arr.append(QJsonDocument::fromJson(s->toConfig().toUtf8()).object());
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

void FT_Edit::fromConfig(const QString &cfg)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(cfg.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return;
    const QJsonArray arr = doc.array();

    QList<QPair<QString, QString>> pending;
    pending.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject obj = v.toObject();
        const QString itemCfg = QString::fromUtf8(QJsonDocument(obj).toJson());
        const QString title = obj["title"].toString();
        pending.append(qMakePair(title, itemCfg));
    }

    clear();
    for (const auto &p : std::as_const(pending)) {
        FT_FunctionWidgetItem *s = addSchedule(p.first);
        s->fromConfig(p.second);
    }
}