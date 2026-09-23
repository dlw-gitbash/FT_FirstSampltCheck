#include "FT_Edit.h"
#include "Module/FT_Function.h"
#include "Module/FT_FunctionItem.h"
#include "Module/FT_FunctionList.h"
#include "Module/FT_FunctionTree.h"
#include "Module/FT_Project.h"
#include "Module/FT_Data.h"

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
#include <variant>

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
    m_project  = new FT_Project(m_funcList);

    splitter->addWidget(m_funcTree);
    splitter->addWidget(m_funcList);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 860});
    root->addWidget(splitter, 1);

    m_statusBar = new QStatusBar(this);
    root->addWidget(m_statusBar);

    buildTree();
    buildItemList();

    connect(m_funcTree, &QTreeWidget::itemDoubleClicked,
            this, &FT_Edit::onTreeItemDoubleClicked);
    connect(m_funcList, &FT_FunctionList::treeNodeDropped,
            this, &FT_Edit::onTreeNodeDropped);
    connect(m_funcList, &FT_FunctionList::itemMoved,
            this, &FT_Edit::onItemMoved);
    connect(m_funcList, &FT_FunctionList::internalReorderRequested,
            this, &FT_Edit::onInternalReorder);
    connect(m_funcList, &FT_FunctionList::emptyAreaDoubleClicked,
            this, &FT_Edit::onEmptyAreaDoubleClicked);
    connect(m_funcList, &FT_FunctionList::itemEnterPressed,
            this, &FT_Edit::onItemEnterPressed);

    auto* delSc = new QShortcut(QKeySequence(QKeySequence::Delete), this);
    connect(delSc, &QShortcut::activated, this, &FT_Edit::deleteCurrentItem);

    m_statusBar->showMessage(
        tr("Double-click or drag Functions / TBox Command to add a command"));
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
    makeBtn(iconDir + QStringLiteral("insert.png"), tr("Insert"), [this]() { addItem(); });
    makeBtn(iconDir + QStringLiteral("clear.png"),  tr("Clear"),  [this]() {
        if (m_funcList->count() == 0)
            return;
        const auto ret = QMessageBox::question(
            this, tr("Clear All"),
            tr("Remove all %1 function(s)?").arg(m_funcList->count()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
            clearAllItems();
    });
    makeBtn(iconDir + QStringLiteral("up.png"),   tr("Up"),   [this]() { moveCurrentItem(-1); });
    makeBtn(iconDir + QStringLiteral("down.png"), tr("Down"), [this]() { moveCurrentItem(1); });
    lay->addStretch(1);
}

void FT_Edit::buildTree()
{
    auto* functions = new QTreeWidgetItem(m_funcTree, QStringList{tr("Functions")});
    functions->setData(0, Qt::UserRole, NodeFunctions);

    auto* tbox = new QTreeWidgetItem(functions, QStringList{tr("TBox Command")});
    tbox->setData(0, Qt::UserRole, NodeTboxCommand);

    auto* iicWrite = new QTreeWidgetItem(functions, QStringList{tr("I2C Write")});
    iicWrite->setData(0, Qt::UserRole, NodeIicWrite);

    auto* iicWriteRead = new QTreeWidgetItem(functions, QStringList{tr("I2C Write+Read")});
    iicWriteRead->setData(0, Qt::UserRole, NodeIicWriteRead);

    functions->setExpanded(true);
    m_funcTree->setCurrentItem(functions);

    m_funcTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_funcTree, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(m_funcTree);
        QAction* addTbox = menu.addAction(
            style()->standardIcon(QStyle::SP_FileIcon),
            tr("Add TBox Command"));
        QAction* addI2cW = menu.addAction(
            style()->standardIcon(QStyle::SP_FileIcon),
            tr("Add I2C Write"));
        QAction* addI2cWR = menu.addAction(
            style()->standardIcon(QStyle::SP_FileIcon),
            tr("Add I2C Write+Read"));
        QAction* chosen = menu.exec(m_funcTree->viewport()->mapToGlobal(pos));
        if (chosen == addTbox)
            addTboxItem();
        else if (chosen == addI2cW)
            addIicWriteItem();
        else if (chosen == addI2cWR)
            addIicWriteReadItem();
    });
}

void FT_Edit::buildItemList()
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
        tr("Double-click or drag \"Functions\" / \"TBox Command\" here to add a command"));
    m_emptyHint->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_funcList->viewport()->installEventFilter(this);

    m_funcList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_funcList, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        const bool hasSel    = (m_funcList->currentRow() >= 0);
        const bool canMoveUp = (m_funcList->currentRow() > 0);
        const bool canMoveDown = (m_funcList->currentRow() >= 0 &&
                                  m_funcList->currentRow() < m_funcList->count() - 1);

        QMenu menu(m_funcList);

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
        QAction* up = menu.addAction(
            style()->standardIcon(QStyle::SP_ArrowUp),
            tr("Move Up"));
        up->setEnabled(canMoveUp);
        QAction* down = menu.addAction(
            style()->standardIcon(QStyle::SP_ArrowDown),
            tr("Move Down"));
        down->setEnabled(canMoveDown);

        menu.addSeparator();
        QAction* del = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove Selected"));
        del->setEnabled(hasSel);
        QAction* clear = menu.addAction(
            style()->standardIcon(QStyle::SP_DialogResetButton),
            tr("Clear All"));
        clear->setEnabled(m_funcList->count() > 0);

        QAction* chosen = menu.exec(m_funcList->viewport()->mapToGlobal(pos));
        if (chosen == addTbox)
            addTboxItem();
        else if (chosen == addI2cW)
            addIicWriteItem();
        else if (chosen == addI2cWR)
            addIicWriteReadItem();
        else if (chosen == up)
            moveCurrentItem(-1);
        else if (chosen == down)
            moveCurrentItem(1);
        else if (chosen == del)
            deleteCurrentItem();
        else if (chosen == clear) {
            const auto ret = QMessageBox::question(
                this, tr("Clear All"),
                tr("Remove all %1 function(s)?").arg(m_funcList->count()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::Yes)
                clearAllItems();
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
        addTboxItem();
        break;
    case NodeIicWrite:
        addIicWriteItem();
        break;
    case NodeIicWriteRead:
        addIicWriteReadItem();
        break;
    default:
        break;
    }
}

void FT_Edit::onTreeNodeDropped(int nodeType, int atIndex)
{
    if (nodeType == NodeFunctions || nodeType == NodeTboxCommand)
        addItem(atIndex);
    else if (nodeType == NodeIicWrite)
        addIicWriteItem(atIndex);
    else if (nodeType == NodeIicWriteRead)
        addIicWriteReadItem(atIndex);
}

void FT_Edit::onItemMoved(int /*fromIndex*/, int toIndex)
{
    if (auto* item = itemAt(toIndex))
        item->adjustHeight();
    m_statusBar->showMessage(tr("Function moved"), 1500);
    m_project->markDirty();
}

void FT_Edit::onInternalReorder(int from, int to)
{
    // 拖拽落点 to 是“插入位置”（0..count()），直接交给统一的 config 轮转实现。
    moveFunctionRow(from, to);
}

void FT_Edit::moveFunctionRow(int from, int insertionPos)
{
    const int count = m_funcList->count();
    if (from < 0 || from >= count)
        return;

    // 插入位置 -> 最终行索引
    int target = qBound(0, insertionPos, count);
    if (target > from)
        --target;
    if (target == from)
        return;

    FT_FunctionItem* movingItem = itemAt(from);
    if (!movingItem)
        return;

    // 行控件/QListWidgetItem 原地不动，仅在行间轮转 config。
    // 否则 takeItem/removeItemWidget 会销毁行控件（deleteLater），
    // 造成空白行与悬空指针（再次重排即崩溃）。
    const FT_FunctionItemConfig movingCfg = movingItem->toConfig();
    if (target > from) {
        for (int i = from; i < target; ++i) {
            FT_FunctionItem* dst = itemAt(i);
            FT_FunctionItem* src = itemAt(i + 1);
            if (dst && src)
                dst->applyConfig(src->toConfig());
        }
    } else {
        for (int i = from; i > target; --i) {
            FT_FunctionItem* dst = itemAt(i);
            FT_FunctionItem* src = itemAt(i - 1);
            if (dst && src)
                dst->applyConfig(src->toConfig());
        }
    }

    if (FT_FunctionItem* targetItem = itemAt(target))
        targetItem->applyConfig(movingCfg);

    m_funcList->setCurrentRow(target);
    m_statusBar->showMessage(tr("Function moved"), 1500);
    m_project->markDirty();
}

void FT_Edit::onEmptyAreaDoubleClicked()
{
    addItem();
}

void FT_Edit::onItemEnterPressed(int /*index*/)
{
    addTboxItem();
}

FT_FunctionItem* FT_Edit::itemAt(int index) const
{
    if (index < 0 || index >= m_funcList->count())
        return nullptr;
    QListWidgetItem* item = m_funcList->item(index);
    return qobject_cast<FT_FunctionItem*>(m_funcList->itemWidget(item));
}

FT_FunctionItem* FT_Edit::addFunctionItem(FT_Function* func, int atIndex, const QString& statusMessage)
{
    auto* funcItem = new FT_FunctionItem();
    funcItem->setFunction(func);

    auto* item = new QListWidgetItem();
    constexpr int kOuterBorderV = 2;
    item->setSizeHint(QSize(0, funcItem->sizeHint().height() + kOuterBorderV));

    if (atIndex < 0 || atIndex >= m_funcList->count()) {
        m_funcList->addItem(item);
    } else {
        m_funcList->insertItem(atIndex, item);
    }
    m_funcList->setItemWidget(item, funcItem);
    m_funcList->setCurrentItem(item);

    funcItem->adjustHeight();

    connect(funcItem, &FT_FunctionItem::contentChanged, this, [this]() {
        m_project->markDirty();
    });
    connect(funcItem, &FT_FunctionItem::removeRequested, this, [this, funcItem]() {
        removeItem(funcItem);
    });
    connect(funcItem, &FT_FunctionItem::moveUpRequested, this, [this, funcItem]() {
        moveItemUp(funcItem);
    });
    connect(funcItem, &FT_FunctionItem::moveDownRequested, this, [this, funcItem]() {
        moveItemDown(funcItem);
    });

    m_statusBar->showMessage(statusMessage, 2000);
    updateEmptyHint();
    m_project->markDirty();
    return funcItem;
}

FT_FunctionItem* FT_Edit::addItem(int atIndex)
{
    FT_Function* func = FtFunctionFactory::create(FtFunctionFactory::defaultTypeName());
    if (!func) {
        m_statusBar->showMessage(tr("Failed to create function"), 3000);
        return nullptr;
    }
    return addFunctionItem(func, atIndex, tr("New function added"));
}

void FT_Edit::addTboxItem(int atIndex)
{
    FT_Function* func = FtFunctionFactory::create(QStringLiteral("TBoxCommand"));
    if (!func) {
        m_statusBar->showMessage(tr("Failed to create TBox function"), 3000);
        return;
    }
    addFunctionItem(func, atIndex, tr("New TBox command added"));
}

void FT_Edit::addIicWriteItem(int atIndex)
{
    FT_Function* func = FtFunctionFactory::create(QStringLiteral("IicWrite"));
    if (!func) {
        m_statusBar->showMessage(tr("Failed to create I2C Write function"), 3000);
        return;
    }
    addFunctionItem(func, atIndex, tr("New I2C Write function added"));
}

void FT_Edit::addIicWriteReadItem(int atIndex)
{
    FT_Function* func = FtFunctionFactory::create(QStringLiteral("IicWriteRead"));
    if (!func) {
        m_statusBar->showMessage(tr("Failed to create I2C Write+Read function"), 3000);
        return;
    }
    addFunctionItem(func, atIndex, tr("New I2C Write+Read function added"));
}

FT_FunctionItem* FT_Edit::currentItem() const
{
    QListWidgetItem* item = m_funcList->currentItem();
    if (!item)
        return nullptr;
    return qobject_cast<FT_FunctionItem*>(m_funcList->itemWidget(item));
}

void FT_Edit::removeItem(FT_FunctionItem* funcItem)
{
    if (!funcItem)
        return;

    for (int i = 0; i < m_funcList->count(); ++i) {
        QListWidgetItem* item = m_funcList->item(i);
        if (m_funcList->itemWidget(item) == funcItem) {
            m_funcList->removeItemWidget(item);
            delete m_funcList->takeItem(i);
            funcItem->deleteLater();
            break;
        }
    }
    m_statusBar->showMessage(tr("Function removed"), 2000);
    updateEmptyHint();
    m_project->markDirty();
}

void FT_Edit::deleteCurrentItem()
{
    QWidget* fw = QApplication::focusWidget();
    if (qobject_cast<QTextEdit*>(fw) ||
        qobject_cast<QLineEdit*>(fw) ||
        qobject_cast<QAbstractSpinBox*>(fw) ||
        qobject_cast<QComboBox*>(fw))
        return;

    FT_FunctionItem* funcItem = currentItem();
    if (!funcItem)
        return;

    const auto ret = QMessageBox::question(
        this, tr("Remove Function"),
        tr("Remove the selected function?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes)
        removeItem(funcItem);
}

void FT_Edit::moveCurrentItem(int delta)
{
    const int from = m_funcList->currentRow();
    if (from < 0) {
        m_statusBar->showMessage(tr("Nothing to move"), 1500);
        return;
    }

    // 换算为“插入位置”：上移插到 from-1 之前；下移插到 from+1 之后
    const int insertionPos = (delta < 0) ? (from - 1) : (from + 2);
    if (insertionPos < 0 || insertionPos > m_funcList->count()) {
        m_statusBar->showMessage(tr("Nothing to move"), 1500);
        return;
    }

    moveFunctionRow(from, insertionPos);
}

void FT_Edit::moveItemUp(FT_FunctionItem* funcItem)
{
    if (!funcItem)
        return;
    for (int i = 0; i < m_funcList->count(); ++i) {
        if (m_funcList->itemWidget(m_funcList->item(i)) == funcItem) {
            if (i == 0) {
                m_statusBar->showMessage(tr("Already at top"), 1500);
                return;
            }
            m_funcList->setCurrentRow(i);
            moveCurrentItem(-1);
            return;
        }
    }
}

void FT_Edit::moveItemDown(FT_FunctionItem* funcItem)
{
    if (!funcItem)
        return;
    for (int i = 0; i < m_funcList->count(); ++i) {
        if (m_funcList->itemWidget(m_funcList->item(i)) == funcItem) {
            if (i == m_funcList->count() - 1) {
                m_statusBar->showMessage(tr("Already at bottom"), 1500);
                return;
            }
            m_funcList->setCurrentRow(i);
            moveCurrentItem(1);
            return;
        }
    }
}

void FT_Edit::clearAllItems()
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
    m_project->markDirty();
}

bool FT_Edit::exportConfig()
{
    FT_FunctionDocument doc = m_project->collectDocument();

    QString hexErr;
    for (int i = 0; i < doc.size(); ++i) {
        const FT_FunctionItemConfig& cfg = doc[i];
        auto checkPayload = [&hexErr, &cfg](const QString& payload, const QString& fieldName) {
            const QStringList tokens = ftSplitWs(payload);
            if (!tokens.isEmpty() && !ftAllHexByteTokens(tokens)) {
                hexErr = tr("Function \"%1\": %2 must be hex bytes "
                            "separated by spaces (e.g. 22 66).")
                             .arg(cfg.title.isEmpty() ? tr("(untitled)") : cfg.title, fieldName);
                return false;
            }
            return true;
        };

        if (std::holds_alternative<FT_TboxConfig>(cfg.functionData)) {
            const FT_TboxConfig& tbox = std::get<FT_TboxConfig>(cfg.functionData);
            if (!checkPayload(tbox.payload, tr("payload")))
                break;
        } else if (std::holds_alternative<FT_IicWriteConfig>(cfg.functionData)) {
            const FT_IicWriteConfig& iw = std::get<FT_IicWriteConfig>(cfg.functionData);
            if (!checkPayload(iw.reg, tr("reg")))
                break;
            if (!checkPayload(iw.payload, tr("payload")))
                break;
        } else if (std::holds_alternative<FT_IicWriteReadConfig>(cfg.functionData)) {
            const FT_IicWriteReadConfig& iwr = std::get<FT_IicWriteReadConfig>(cfg.functionData);
            if (!checkPayload(iwr.reg, tr("reg")))
                break;
            if (!checkPayload(iwr.payload, tr("payload")))
                break;
        }
    }
    if (!hexErr.isEmpty()) {
        QMessageBox::warning(this, tr("Save failed"), hexErr);
        return false;
    }

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Config"),
        QStringLiteral("ft_config.json"),
        tr("JSON config (*.json)"));
    if (path.isEmpty())
        return false;

    if (!m_project->exportToFile(path)) {
        QMessageBox::warning(this, tr("Save failed"), m_project->lastError());
        return false;
    }

    m_statusBar->showMessage(
        tr("Saved %1 function(s) -> %2")
            .arg(doc.size()).arg(QFileInfo(path).fileName()), 5000);
    return true;
}

void FT_Edit::importConfig()
{
    if (!m_project->isDirty()) {
        if (m_funcList->count() > 0) {
            const auto ret = QMessageBox::question(
                this, tr("Load Config"),
                tr("Replace the current %1 function(s) with the file?").arg(m_funcList->count()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return;
        }
    } else {
        const auto ret = QMessageBox::question(
            this, tr("Load Config"),
            tr("There are unsaved changes. Discard them?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Load Config"), QString(),
        tr("JSON config (*.json)"));
    if (path.isEmpty())
        return;

    if (!m_project->importFromFile(path)) {
        QMessageBox::warning(this, tr("Load failed"), m_project->lastError());
        return;
    }

    FT_FunctionDocument doc = m_project->collectDocument();
    m_statusBar->showMessage(
        tr("Loaded %1 function(s)").arg(doc.size()), 5000);
}

void FT_Edit::closeEvent(QCloseEvent* event)
{
    event->accept();
}