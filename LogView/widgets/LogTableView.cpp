#include "LogTableView.h"
#include "LogToolBar.h"
#include "core/LogHighlighter.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QClipboard>
#include <QApplication>
#include <QDateTime>

LogTableView::LogTableView(QWidget* parent)
    : QWidget(parent)
    , m_tableView(nullptr)
    , m_model(nullptr)
    , m_proxy(nullptr)
    , m_toolBar(nullptr)
    , m_delegate(nullptr)
    , m_contextMenu(nullptr)
{
    setupUI();
    setupConnections();
}

LogTableView::~LogTableView()
{
    // Children are deleted by Qt's parent-child mechanism
}

void LogTableView::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- 工具栏 ---
    m_toolBar = new LogToolBar(this);
    layout->addWidget(m_toolBar);

    // --- Model ---
    m_model = new LogModel(1000000, this);

    // --- Filter Proxy ---
    m_proxy = new LogFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterKeyColumn(-1); // 由 filterAcceptsRow 自行决定
    // 默认按日志序号降序（新日志在底部）
    m_proxy->sort(LogModel::ColTimestamp, Qt::AscendingOrder);

    m_toolBar->setFilterProxy(m_proxy);

    // --- TableView ---
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxy);

    // 高性能配置
    m_tableView->setShowGrid(false);
    m_tableView->setAlternatingRowColors(false); // 由 delegate 自己画背景
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setWordWrap(false);
    m_tableView->setTextElideMode(Qt::ElideRight);
    m_tableView->setSortingEnabled(false);

    // 表头
    auto* header = m_tableView->horizontalHeader();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionsClickable(false);
    header->setSectionsMovable(false);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* vHeader = m_tableView->verticalHeader();
    vHeader->setVisible(false);
    vHeader->setDefaultSectionSize(20);

    applyDefaultColumnWidths();

    // 启用上下文菜单
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    // --- Delegate ---
    m_delegate = new LogHighlighter(this);
    m_tableView->setItemDelegate(m_delegate);

    layout->addWidget(m_tableView);

    // 右键菜单
    m_contextMenu = new QMenu(this);
    m_copyAction = m_contextMenu->addAction(QStringLiteral("复制选中内容"));
    m_copyRowAction = m_contextMenu->addAction(QStringLiteral("复制整行"));
    m_contextMenu->addSeparator();
    m_selectAllAction = m_contextMenu->addAction(QStringLiteral("全选"));
}

void LogTableView::setupConnections()
{
    // 模型统计更新 → 工具栏
    connect(m_model, &LogModel::statsChanged, this, &LogTableView::onStatsUpdate);

    // 工具栏信号
    connect(m_toolBar, &LogToolBar::autoScrollToggled, this, &LogTableView::onAutoScrollToggled);
    connect(m_toolBar, &LogToolBar::pauseRequested, this, &LogTableView::onPauseToggled);
    connect(m_toolBar, &LogToolBar::clearRequested, this, [this]() { clear(); });

    // 模型数据变化 → 自动滚动（连接到 proxy 的 layoutChanged/rowsInserted）
    connect(m_proxy, &QAbstractItemModel::rowsInserted, this, [this]() {
        if (m_autoScroll && !m_paused) {
            scrollToBottom();
        }
    });

    // 右键菜单
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &LogTableView::onCustomContextMenu);
    connect(m_copyAction, &QAction::triggered, this, &LogTableView::copySelectedRows);
    connect(m_copyRowAction, &QAction::triggered, this, [this]() { copySelectedRows(); });
    connect(m_selectAllAction, &QAction::triggered, m_tableView, &QTableView::selectAll);
}

void LogTableView::applyDefaultColumnWidths()
{
    auto* header = m_tableView->horizontalHeader();
    header->resizeSection(LogModel::ColTimestamp, 170);
    header->resizeSection(LogModel::ColLevel,     60);
    // ColMessage 通过 stretchLastSection 自动占满
}

// ==================== 日志添加 API ====================

void LogTableView::addLog(LogEntry entry)
{
    if (m_paused) return;
    m_model->addEntry(std::move(entry));
}

void LogTableView::addLogs(std::vector<LogEntry>& entries)
{
    if (m_paused) return;
    m_model->addEntries(entries);
}

void LogTableView::addLog(LogLevel level, const QString& logger, const QString& message,
                           const QString& thread, int line)
{
    LogEntry entry;
    entry.timestamp  = QDateTime::currentMSecsSinceEpoch();
    entry.level      = level;
    entry.logger     = logger;
    entry.message    = message;
    entry.threadName = thread.isEmpty() ? QStringLiteral("main") : thread;
    entry.lineNumber = line;
    addLog(std::move(entry));
}

void LogTableView::addLogTrace(const QString& logger, const QString& msg, const QString& thread, int line)
{ addLog(LogLevel::Trace, logger, msg, thread, line); }

void LogTableView::addLogDebug(const QString& logger, const QString& msg, const QString& thread, int line)
{ addLog(LogLevel::Debug, logger, msg, thread, line); }

void LogTableView::addLogInfo(const QString& logger, const QString& msg, const QString& thread, int line)
{ addLog(LogLevel::Info, logger, msg, thread, line); }

void LogTableView::addLogWarn(const QString& logger, const QString& msg, const QString& thread, int line)
{ addLog(LogLevel::Warn, logger, msg, thread, line); }

void LogTableView::addLogError(const QString& logger, const QString& msg, const QString& thread, int line)
{ addLog(LogLevel::Error, logger, msg, thread, line); }

void LogTableView::addLogFatal(const QString& logger, const QString& msg, const QString& thread, int line)
{ addLog(LogLevel::Fatal, logger, msg, thread, line); }

// ==================== 控制接口 ====================

void LogTableView::setPaused(bool paused)
{
    m_paused = paused;
}

void LogTableView::clear()
{
    m_model->clear();
    emit logCleared();
}

void LogTableView::setMaxEntries(int max)
{
    m_model->setMaxEntries(max);
}

void LogTableView::setBatchInterval(int ms)
{
    m_model->setBatchInterval(ms);
}

void LogTableView::setColumnVisible(int column, bool visible)
{
    m_tableView->setColumnHidden(column, !visible);
}

void LogTableView::setColumnWidth(int column, int width)
{
    m_tableView->setColumnWidth(column, width);
}

void LogTableView::restoreDefaultLayout()
{
    applyDefaultColumnWidths();
    for (int i = 0; i < LogModel::ColumnCount; ++i) {
        m_tableView->setColumnHidden(i, false);
    }
}

// ==================== 内部槽函数 ====================

void LogTableView::onStatsUpdate(int visible, quint32 total)
{
    m_toolBar->updateStats(visible, total);
}

void LogTableView::onAutoScrollToggled(bool enabled)
{
    m_autoScroll = enabled;
    if (enabled && !m_paused) {
        scrollToBottom();
    }
}

void LogTableView::onPauseToggled(bool paused)
{
    m_paused = paused;
}

void LogTableView::onSelectionChanged()
{
    // Placeholder for future use
}

void LogTableView::scrollToBottom()
{
    int rowCount = m_proxy->rowCount();
    if (rowCount > 0) {
        m_tableView->scrollToBottom();
    }
}

void LogTableView::onCustomContextMenu(const QPoint& pos)
{
    QModelIndex index = m_tableView->indexAt(pos);
    if (index.isValid()) {
        m_contextMenu->exec(m_tableView->viewport()->mapToGlobal(pos));
    }
}

void LogTableView::copySelectedRows()
{
    QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QStringList lines;
    for (const QModelIndex& idx : selected) {
        QStringList cols;
        for (int c = 0; c < LogModel::ColumnCount; ++c) {
            QModelIndex cell = m_proxy->index(idx.row(), c);
            cols << cell.data(Qt::DisplayRole).toString();
        }
        lines << cols.join('\t');
    }

    QApplication::clipboard()->setText(lines.join('\n'));
}