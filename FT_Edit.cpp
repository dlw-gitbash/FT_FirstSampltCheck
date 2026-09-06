#include "FT_Edit.h"
#include "FT_FunctionCard.h"
#include "FT_FunctionRow.h"
#include "FT_FunctionConfig.h"
#include "FT_FunctionFactory.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QSizePolicy>
#include <QIcon>
#include <QAction>
#include <QStyle>
#include <QShortcut>
#include <QKeySequence>
#include <QMenu>
#include <QStatusBar>
#include <QApplication>
#include <QLabel>
#include <QEvent>
#include <QCheckBox>
#include <QTextEdit>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QCloseEvent>
#include <QLineEdit>
#include <QComboBox>
#include <QAbstractSpinBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QDrag>
#include <QPixmap>
#include <string>
#include <variant>

static const QString kFtNodeMime = QStringLiteral("application/x-ft-node");

class FtDropIndicator : public QWidget
{
public:
    explicit FtDropIndicator(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFixedHeight(10);
        hide();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QColor color(74, 144, 217);
        QPen pen(color, 2);
        p.setPen(pen);
        p.setBrush(color);

        const int midY = height() / 2;
        const int left = 6;
        const int right = width() - 6;

        p.drawLine(left, midY, right, midY);

        QPainterPath leftArrow;
        leftArrow.moveTo(left, midY);
        leftArrow.lineTo(left + 8, midY - 5);
        leftArrow.lineTo(left + 8, midY + 5);
        leftArrow.closeSubpath();
        p.drawPath(leftArrow);

        QPainterPath rightArrow;
        rightArrow.moveTo(right, midY);
        rightArrow.lineTo(right - 8, midY - 5);
        rightArrow.lineTo(right - 8, midY + 5);
        rightArrow.closeSubpath();
        p.drawPath(rightArrow);
    }
};

FT_FunctionTree::FT_FunctionTree(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setFrameShape(QFrame::NoFrame);
    setMinimumWidth(180);
    setIndentation(12);
    setRootIsDecorated(true);
    setExpandsOnDoubleClick(false);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);
}

QMimeData* FT_FunctionTree::mimeData(const QList<QTreeWidgetItem*>& items) const
{
    auto* mime = new QMimeData();
    if (items.isEmpty())
        return mime;
    const int nodeType = items.first()->data(0, Qt::UserRole).toInt();
    mime->setData(kFtNodeMime, QByteArray::number(nodeType));
    return mime;
}

QStringList FT_FunctionTree::mimeTypes() const
{
    return {kFtNodeMime};
}

Qt::DropActions FT_FunctionTree::supportedDropActions() const
{
    return Qt::CopyAction;
}

FT_FunctionList::FT_FunctionList(QWidget* parent)
    : QListWidget(parent)
{
    setFrameShape(QFrame::NoFrame);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSpacing(2);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(false);

    m_dropIndicator = new FtDropIndicator(this);
    m_dropIndicator->raise();

    m_dragPreview = new QLabel(viewport());
    m_dragPreview->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dragPreview->hide();
}

void FT_FunctionList::updateDragPreview(const QPoint& pos)
{
    if (!m_dragPreview || m_dragPreview->pixmap().isNull())
        return;

    const QSize ps = m_dragPreview->pixmap().size();
    const int vw = viewport()->width();
    const int vh = viewport()->height();

    int x = pos.x() - ps.width() / 2;
    int y = pos.y() - ps.height() / 2;

    x = qBound(0, x, qMax(0, vw - ps.width()));
    y = qBound(0, y, qMax(0, vh - ps.height()));

    m_dragPreview->move(x, y);
    m_dragPreview->raise();
    m_dragPreview->show();
}

void FT_FunctionList::hideDragPreview()
{
    if (m_dragPreview)
        m_dragPreview->hide();
}

void FT_FunctionList::updateDropIndicator(const QPoint& pos)
{
    if (!m_dropIndicator)
        return;

    const int vw = width();

    if (count() == 0) {
        m_dropIndicator->setGeometry(0, 0, vw, m_dropIndicator->height());
        m_dropIndicator->raise();
        m_dropIndicator->show();
        m_dragOverRow = 0;
        return;
    }

    QListWidgetItem* at = itemAt(pos);
    if (!at) {
        int lastRow = count() - 1;
        QRect r = visualItemRect(item(lastRow));
        int y = r.bottom() + 1;
        m_dropIndicator->setGeometry(0, y, vw, m_dropIndicator->height());
        m_dropIndicator->raise();
        m_dropIndicator->show();
        m_dragOverRow = count();
        return;
    }

    QRect r = visualItemRect(at);
    const int rowTop = r.top();
    const int rowMid = r.center().y();
    const int insertAbove = (pos.y() < rowMid) ? 1 : 0;
    int y = insertAbove ? rowTop - m_dropIndicator->height() / 2
                        : r.bottom() + 1 - m_dropIndicator->height() / 2;

    m_dropIndicator->setGeometry(0, y, vw, m_dropIndicator->height());
    m_dropIndicator->raise();
    m_dropIndicator->show();
    m_dragOverRow = row(at) + (insertAbove ? 0 : 1);
}

void FT_FunctionList::hideDropIndicator()
{
    if (m_dropIndicator)
        m_dropIndicator->hide();
    m_dragOverRow = -1;
}

void FT_FunctionList::startDrag(Qt::DropActions supportedActions)
{
    m_dragSourceRow = currentRow();

    QList<QListWidgetItem*> items = selectedItems();
    if (items.isEmpty())
        return;

    QWidget* w = itemWidget(items.first());
    if (w && m_dragPreview) {
        QPixmap pix = w->grab();
        m_dragPreview->setPixmap(pix);
        m_dragPreview->resize(pix.size());
    }

    QMimeData* mime = mimeData(items);
    if (!mime)
        return;

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mime);
    QPixmap tpix(1, 1);
    tpix.fill(Qt::transparent);
    drag->setPixmap(tpix);
    drag->exec(supportedActions, Qt::MoveAction);

    hideDragPreview();
}

void FT_FunctionList::dragEnterEvent(QDragEnterEvent* event)
{
    const QMimeData* md = event->mimeData();
    const QPoint pos = event->position().toPoint();
    if (md->hasFormat(kFtNodeMime)) {
        updateDropIndicator(pos);
        event->acceptProposedAction();
        return;
    }
    updateDropIndicator(pos);
    updateDragPreview(pos);
    QListWidget::dragEnterEvent(event);
}

void FT_FunctionList::dragMoveEvent(QDragMoveEvent* event)
{
    const QPoint pos = event->position().toPoint();
    updateDropIndicator(pos);
    updateDragPreview(pos);
    QListWidget::dragMoveEvent(event);
}

void FT_FunctionList::dragLeaveEvent(QDragLeaveEvent* event)
{
    hideDropIndicator();
    hideDragPreview();
    QListWidget::dragLeaveEvent(event);
}

void FT_FunctionList::dropEvent(QDropEvent* event)
{
    const int toRow = m_dragOverRow;
    hideDropIndicator();
    hideDragPreview();

    const QMimeData* md = event->mimeData();

    if (md->hasFormat(kFtNodeMime)) {
        const int nodeType = md->data(kFtNodeMime).toInt();
        emit treeNodeDropped(nodeType, toRow);
        event->acceptProposedAction();
        m_dragSourceRow = -1;
        return;
    }

    const int fromRow = m_dragSourceRow;
    m_dragSourceRow = -1;

    QListWidget::dropEvent(event);

    if (fromRow >= 0 && fromRow < count())
        emit cardMoved(fromRow, currentRow());
}

void FT_FunctionList::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QListWidgetItem* at = itemAt(event->position().toPoint());
        if (!at) {
            emit emptyAreaDoubleClicked();
            event->accept();
            return;
        }
    }
    QListWidget::mouseDoubleClickEvent(event);
}

void FT_FunctionList::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        const int r = currentRow();
        if (r >= 0) {
            emit cardEnterPressed(r);
            event->accept();
            return;
        }
    }
    QListWidget::keyPressEvent(event);
}

FT_Edit::FT_Edit(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(tr("FT_Edit - Function Editor"));
    resize(1080, 640);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    buildButtonBar();
    root->addWidget(m_buttonBar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    m_funcTree = new FT_FunctionTree();
    m_funcList = new FT_FunctionList();

    splitter->addWidget(m_funcTree);
    splitter->addWidget(m_funcList);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 860});
    root->addWidget(splitter, 1);

    m_statusBar = new QStatusBar(this);
    root->addWidget(m_statusBar);

    buildTree();
    buildCardList();

    connect(m_funcTree, &QTreeWidget::itemDoubleClicked,
            this, &FT_Edit::onTreeItemDoubleClicked);
    connect(m_funcList, &FT_FunctionList::treeNodeDropped,
            this, &FT_Edit::onTreeNodeDropped);
    connect(m_funcList, &FT_FunctionList::cardMoved,
            this, &FT_Edit::onCardMoved);
    connect(m_funcList, &FT_FunctionList::emptyAreaDoubleClicked,
            this, &FT_Edit::onEmptyAreaDoubleClicked);
    connect(m_funcList, &FT_FunctionList::cardEnterPressed,
            this, &FT_Edit::onCardEnterPressed);

    auto* delSc = new QShortcut(QKeySequence(QKeySequence::Delete), this);
    connect(delSc, &QShortcut::activated, this, &FT_Edit::deleteCurrentCard);

    m_statusBar->showMessage(
        tr("Double-click or drag Functions / TBox Command to add a row"));
}

void FT_Edit::buildButtonBar()
{
    m_buttonBar = new QWidget(this);
    m_buttonBar->setFixedHeight(30);

    auto* lay = new QHBoxLayout(m_buttonBar);
    lay->setContentsMargins(6, 2, 6, 2);
    lay->setSpacing(6);

    auto makeBtn = [this, lay](const QString& iconPath, const QString& text, auto slot) {
        auto* btn = new QToolButton(m_buttonBar);
        btn->setIcon(QIcon(iconPath));
        btn->setToolTip(text);
        btn->setIconSize(QSize(18, 18));
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setFixedSize(28, 26);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(btn, &QToolButton::clicked, this, slot);
        lay->addWidget(btn);
        return btn;
    };

    const QString iconDir = QStringLiteral(":/resources/icons/");
    makeBtn(iconDir + QStringLiteral("load.png"),   tr("Load"),   &FT_Edit::importConfig);
    makeBtn(iconDir + QStringLiteral("save.png"),   tr("Save"),   &FT_Edit::exportConfig);
    makeBtn(iconDir + QStringLiteral("insert.png"), tr("Insert"), [this]() { addCard(); });
    makeBtn(iconDir + QStringLiteral("clear.png"),  tr("Clear"),  [this]() {
        if (m_funcList->count() == 0)
            return;
        const auto ret = QMessageBox::question(
            this, tr("Clear All"),
            tr("Remove all %1 function(s)?").arg(m_funcList->count()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            clearAllCards();
    });
    makeBtn(iconDir + QStringLiteral("up.png"),   tr("Up"),   [this]() { moveCurrentCard(-1); });
    makeBtn(iconDir + QStringLiteral("down.png"), tr("Down"), [this]() { moveCurrentCard(1); });
    lay->addStretch(1);
}

void FT_Edit::buildTree()
{
    auto* functions = new QTreeWidgetItem(m_funcTree, QStringList{tr("Functions")});
    functions->setData(0, Qt::UserRole, NodeFunctions);

    auto* tbox = new QTreeWidgetItem(functions, QStringList{tr("TBox Command")});
    tbox->setData(0, Qt::UserRole, NodeTboxCommand);

    functions->setExpanded(true);
    m_funcTree->setCurrentItem(functions);

    m_funcTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_funcTree, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QTreeWidgetItem* at = m_funcTree->itemAt(pos);
        const int nodeType = at ? at->data(0, Qt::UserRole).toInt() : 0;

        QMenu menu(m_funcTree);
        QAction* add = nullptr;

        if (nodeType == NodeTboxCommand) {
            add = menu.addAction(
                style()->standardIcon(QStyle::SP_FileIcon),
                tr("Add TBox command"));
        } else {
            add = menu.addAction(
                style()->standardIcon(QStyle::SP_DirIcon),
                tr("New Function"));
        }

        QAction* chosen = menu.exec(m_funcTree->viewport()->mapToGlobal(pos));
        if (chosen == add)
            addCard();
    });
}

void FT_Edit::buildCardList()
{
    m_funcList->setStyleSheet(
        "QListWidget {"
        "  background-color: #f5f5f5;"
        "  border: 1px solid #d9d9d9;"
        "  outline: 0;"
        "  padding: 6px;"
        "}"
        "QListWidget::item {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d9d9d9;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #d6e8ff;"
        "  border: 1px solid #4a90d9;"
        "}"
        "QListWidget::item:hover:!selected {"
        "  border: 1px solid #a8c8f0;"
        "}"
    );

    m_emptyHint = new QLabel(m_funcList->viewport());
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setText(
        tr("Double-click or drag \"Functions\" / \"TBox Command\" here to add a row"));
    m_emptyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_funcList->viewport()->installEventFilter(this);

    m_funcList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_funcList, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(m_funcList);
        QAction* add = menu.addAction(
            style()->standardIcon(QStyle::SP_FileIcon),
            tr("Add Function"));
        QAction* clear = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Clear All"));
        clear->setEnabled(m_funcList->count() > 0);

        QAction* chosen = menu.exec(m_funcList->viewport()->mapToGlobal(pos));
        if (chosen == add) {
            addCard();
        } else if (chosen == clear) {
            const auto ret = QMessageBox::question(
                this, tr("Clear All"),
                tr("Remove all %1 function(s)?").arg(m_funcList->count()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes)
                clearAllCards();
        }
    });

    updateEmptyHint();
}

bool FT_Edit::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_funcList->viewport() && event->type() == QEvent::Resize) {
        updateEmptyHint();
    }
    return QWidget::eventFilter(watched, event);
}

void FT_Edit::updateEmptyHint()
{
    if (!m_emptyHint)
        return;

    const bool empty = (m_funcList->count() == 0);
    m_emptyHint->setVisible(empty);
    if (empty)
        m_emptyHint->setGeometry(m_funcList->viewport()->rect());
}

void FT_Edit::onTreeItemDoubleClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (!item)
        return;

    switch (item->data(0, Qt::UserRole).toInt()) {
    case NodeTboxCommand:
        addTboxRow();
        break;
    default:
        break;
    }
}

void FT_Edit::onTreeNodeDropped(int nodeType, int atRow)
{
    switch (nodeType) {
    case NodeFunctions:
        addCard(atRow);
        break;
    case NodeTboxCommand:
        addCard(atRow);
        break;
    default:
        break;
    }
}

void FT_Edit::onCardMoved(int /*fromRow*/, int /*toRow*/)
{
    if (auto* card = currentCard())
        card->adjustHeight();
    m_statusBar->showMessage(tr("Function moved"), 1500);
    markDirty();
}

void FT_Edit::onEmptyAreaDoubleClicked()
{
    addCard();
}

void FT_Edit::onCardEnterPressed(int row)
{
    if (cardAtRow(row))
        addTboxRow();
}

FT_FunctionCard* FT_Edit::cardAtRow(int row) const
{
    if (row < 0 || row >= m_funcList->count())
        return nullptr;
    QListWidgetItem* item = m_funcList->item(row);
    return qobject_cast<FT_FunctionCard*>(m_funcList->itemWidget(item));
}

FT_FunctionCard* FT_Edit::addCard(int atRow)
{
    auto* card = new FT_FunctionCard();
    FT_FunctionRow* row = FtFunctionFactory::create(FtFunctionFactory::defaultTypeName());
    if (row)
        card->setRow(row);

    auto* item = new QListWidgetItem();
    constexpr int kOuterBorderV = 2;
    item->setSizeHint(QSize(0, card->sizeHint().height() + kOuterBorderV));

    if (atRow < 0 || atRow >= m_funcList->count()) {
        m_funcList->addItem(item);
    } else {
        m_funcList->insertItem(atRow, item);
    }
    m_funcList->setItemWidget(item, card);
    m_funcList->setCurrentItem(item);

    card->adjustHeight();

    connect(card, &FT_FunctionCard::contentChanged, this, &FT_Edit::markDirty);
    connect(card, &FT_FunctionCard::removeRequested, this, [this, card]() {
        removeCard(card);
    });

    m_statusBar->showMessage(tr("New function added"), 2000);
    updateEmptyHint();
    markDirty();
    return card;
}

void FT_Edit::addTboxRow()
{
    addCard();
    m_statusBar->showMessage(tr("New TBox command added"), 2000);
}

FT_FunctionCard* FT_Edit::currentCard() const
{
    QListWidgetItem* item = m_funcList->currentItem();
    if (!item)
        return nullptr;
    return qobject_cast<FT_FunctionCard*>(m_funcList->itemWidget(item));
}

void FT_Edit::removeCard(FT_FunctionCard* card)
{
    if (!card)
        return;

    for (int i = 0; i < m_funcList->count(); ++i) {
        QListWidgetItem* item = m_funcList->item(i);
        if (m_funcList->itemWidget(item) == card) {
            m_funcList->removeItemWidget(item);
            delete m_funcList->takeItem(i);
            card->deleteLater();
            break;
        }
    }
    m_statusBar->showMessage(tr("Function removed"), 2000);
    updateEmptyHint();
    markDirty();
}

void FT_Edit::deleteCurrentCard()
{
    QWidget* fw = QApplication::focusWidget();
    if (qobject_cast<QTextEdit*>(fw) ||
        qobject_cast<QLineEdit*>(fw) ||
        qobject_cast<QAbstractSpinBox*>(fw) ||
        qobject_cast<QComboBox*>(fw))
        return;

    FT_FunctionCard* card = currentCard();
    if (!card)
        return;

    const auto ret = QMessageBox::question(
        this, tr("Remove Function"),
        tr("Remove the selected function?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes)
        removeCard(card);
}

void FT_Edit::moveCurrentCard(int delta)
{
    const int from = m_funcList->currentRow();
    if (from < 0) {
        m_statusBar->showMessage(tr("Nothing to move"), 1500);
        return;
    }
    const int to = from + delta;
    if (to < 0 || to >= m_funcList->count()) {
        m_statusBar->showMessage(tr("Nothing to move"), 1500);
        return;
    }

    QListWidgetItem* item = m_funcList->item(from);
    QWidget* widget = m_funcList->itemWidget(item);
    m_funcList->removeItemWidget(item);
    item = m_funcList->takeItem(from);
    m_funcList->insertItem(to, item);
    if (widget)
        m_funcList->setItemWidget(item, widget);
    m_funcList->setCurrentItem(item);
    if (auto* card = qobject_cast<FT_FunctionCard*>(widget))
        card->adjustHeight();
    m_statusBar->showMessage(tr("Function moved"), 1500);
    markDirty();
}

void FT_Edit::clearAllCards()
{
    while (m_funcList->count() > 0) {
        QListWidgetItem* item = m_funcList->item(0);
        QWidget* w = m_funcList->itemWidget(item);
        m_funcList->removeItemWidget(item);
        delete m_funcList->takeItem(0);
        if (w)
            w->deleteLater();
    }
    m_statusBar->showMessage(tr("All functions cleared"), 2000);
    updateEmptyHint();
    markDirty();
}

bool FT_Edit::exportConfig()
{
    FT_FunctionDocument doc;
    int rowCount = 0;
    for (int i = 0; i < m_funcList->count(); ++i) {
        QListWidgetItem* item = m_funcList->item(i);
        auto* card = qobject_cast<FT_FunctionCard*>(m_funcList->itemWidget(item));
        if (!card)
            continue;
        doc.push_back(card->toConfig());
        rowCount += doc.last().rows.size();
    }

    QString hexErr;
    if (!payloadHexOk(&hexErr)) {
        QMessageBox::warning(this, tr("Save failed"), hexErr);
        return false;
    }

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Config"),
        QStringLiteral("ft_config.json"),
        tr("JSON config (*.json)"));
    if (path.isEmpty())
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Save failed"),
            tr("Cannot write file:\n%1\n%2").arg(path, file.errorString()));
        return false;
    }
    const std::string text = ftDocumentToJson(doc).dump(2);
    const QByteArray bytes = QByteArray::fromStdString(text);
    const qint64 n = file.write(bytes);
    if (n != bytes.size() || !file.flush()) {
        const QString err = file.errorString();
        file.close();
        QMessageBox::warning(this, tr("Save failed"),
            tr("Cannot write file:\n%1\n%2").arg(path, err));
        return false;
    }
    file.close();

    markClean();
    m_statusBar->showMessage(
        tr("Saved %1 function(s) / %2 command row(s) -> %3")
            .arg(doc.size()).arg(rowCount).arg(QFileInfo(path).fileName()), 5000);
    return true;
}

void FT_Edit::importConfig()
{
    if (!confirmDiscardIfDirty(tr("Load Config")))
        return;
    if (m_funcList->count() > 0 && !m_dirty) {
        const auto ret = QMessageBox::question(
            this, tr("Load Config"),
            tr("Replace the current %1 function(s) with the file?").arg(m_funcList->count()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Config"), QString(),
        tr("JSON config (*.json)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Load failed"),
            tr("Cannot read file:\n%1\n%2").arg(path, file.errorString()));
        return;
    }
    const QByteArray raw = file.readAll();
    file.close();

    FtJson root;
    try {
        root = FtJson::parse(std::string(raw.constData(), static_cast<size_t>(raw.size())));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Load failed"),
            tr("The file is not a valid config JSON:\n%1").arg(QString::fromUtf8(e.what())));
        return;
    }

    FT_FunctionDocument doc;
    try {
        doc = ftDocumentFromJson(root);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Load failed"),
            tr("Malformed config structure:\n%1").arg(QString::fromUtf8(e.what())));
        return;
    }

    m_suppressDirty = true;
    clearAllCards();

    int rowCount = 0;
    for (const FT_FunctionCardConfig& cardCfg : doc) {
        FT_FunctionCard* card = addCard();
        card->applyConfig(cardCfg);
        rowCount += cardCfg.rows.size();
    }
    m_suppressDirty = false;
    markClean();

    m_statusBar->showMessage(
        tr("Loaded %1 function(s) / %2 command row(s)")
            .arg(doc.size()).arg(rowCount), 5000);
}

void FT_Edit::markDirty()
{
    if (m_suppressDirty)
        return;
    m_dirty = true;
}

void FT_Edit::markClean()
{
    m_dirty = false;
}

bool FT_Edit::confirmDiscardIfDirty(const QString& actionTitle)
{
    if (!m_dirty)
        return true;

    const auto ret = QMessageBox::question(
        this, actionTitle,
        tr("There are unsaved changes. Discard them?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return ret == QMessageBox::Yes;
}

bool FT_Edit::payloadHexOk(QString* errorDetail) const
{
    for (int i = 0; i < m_funcList->count(); ++i) {
        auto* card = qobject_cast<FT_FunctionCard*>(
            m_funcList->itemWidget(m_funcList->item(i)));
        if (!card)
            continue;
        const FT_FunctionCardConfig cfg = card->toConfig();
        for (int r = 0; r < cfg.rows.size(); ++r) {
            if (!std::holds_alternative<FT_TboxConfig>(cfg.rows[r]))
                continue;
            const QString& payload = std::get<FT_TboxConfig>(cfg.rows[r]).payload;
            if (!ftAllHexByteTokens(payload, true)) {
                if (errorDetail) {
                    *errorDetail = tr("Function \"%1\", command row %2: payload must be hex bytes "
                                      "separated by spaces (e.g. 22 66).")
                                       .arg(cfg.title.isEmpty() ? tr("(untitled)") : cfg.title)
                                       .arg(r + 1);
                }
                return false;
            }
        }
    }
    return true;
}

void FT_Edit::closeEvent(QCloseEvent* event)
{
    QWidget::closeEvent(event);
}