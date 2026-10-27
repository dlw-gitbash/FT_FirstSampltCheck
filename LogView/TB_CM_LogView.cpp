#include "TB_CM_LogView.h"

#include "TB_CM_LogContextMenu.h"
#include "TB_CM_LogSearch.h"

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextOption>
#include <QTextEdit>
#include <QTextFormat>
#include <QScrollBar>
#include <QPushButton>
#include <QLineEdit>
#include <QAction>
#include <QIcon>
#include <QFont>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QStyle>
#include <QDateTime>
#include <QTimer>
#include <QThread>
#include <QMetaObject>
#include <QSignalBlocker>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

namespace
{
constexpr int kMaxBatchPerFlush = 500;
QLatin1String levelName(int level)
{
    switch (level)
    {
    case L_TRACE:
        return QLatin1String("TRACE");
    case L_DEBUG:
        return QLatin1String("DEBUG");
    case L_INFO:
        return QLatin1String("INFO");
    case L_WARN:
        return QLatin1String("WARN");
    case L_ERROR:
        return QLatin1String("ERROR");
    case L_CRITICAL:
        return QLatin1String("CRITICAL");
    default:
        return QLatin1String("INFO");
    }
}
QString formatLogLine(quint64 timestampMs, int level, const QString &message)
{
    const qint64 ts = (timestampMs != 0) ? static_cast<qint64>(timestampMs)
                                         : QDateTime::currentMSecsSinceEpoch();
    QString body = message;
    if (body.contains(QLatin1Char('\r')))
        body.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    if (body.contains(QLatin1Char('\n')))
        body.replace(QLatin1Char('\n'), QChar(0x23CE));
    QString line;
    line.reserve(32 + body.size());
    line += QDateTime::fromMSecsSinceEpoch(ts).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    line += QLatin1String(" - ");
    line += levelName(level);
    line += QLatin1String(" | ");
    line += body;
    return line;
}
}

TB_CM_LogView::TB_CM_LogView(QWidget *parent)
    : QWidget(parent),
      m_edit(nullptr),
      m_filterBar(nullptr),
      m_filterBarLayout(nullptr),
      m_filterEdit(nullptr),
      m_floatNewLogBtn(nullptr),
      m_clearLogsBtn(nullptr),
      m_mainLayout(nullptr),
      m_flushTimer(nullptr),
      m_searchDebounceTimer(nullptr),
      m_contextMenu(nullptr),
      m_search(nullptr),
      m_maxLogs(DEFAULT_MAX_LOGS),
      m_flushIntervalMs(DEFAULT_FLUSH_INTERVAL_MS),
      m_overflowPolicy(OP_RING_BUFFER),
      m_droppedCount(0),
      m_flushScheduled(false),
      m_autoScroll(true),
      m_flushNow(false)
{
    qRegisterMetaType<TB_CM_LogEntry>("TB_CM_LogEntry");
    qRegisterMetaType<E_OverflowPolicy>("E_OverflowPolicy");
    const QColor levelColors[6] = {
        QColor(0x80, 0x80, 0x80),
        QColor(0x00, 0x00, 0xff),
        QColor(0x00, 0x00, 0x00),
        QColor(0xff, 0x88, 0x00),
        QColor(0xff, 0x00, 0x00),
        QColor(0xcc, 0x00, 0x66)
    };
    for (int i = 0; i < 6; ++i)
        m_levelFormats[i].setForeground(levelColors[i]);
    initFilterBar();
    initView();
    initContextMenu();
    initConnections();
}
TB_CM_LogView::~TB_CM_LogView()
{
    m_flushScheduled.store(false, std::memory_order_release);
    if (m_flushTimer)
        m_flushTimer->stop();
    if (m_searchDebounceTimer)
        m_searchDebounceTimer->stop();
}
void TB_CM_LogView::initFilterBar()
{
    m_filterBar = new QWidget(this);
    m_filterBar->setObjectName("logFilterBar");
    m_filterBarLayout = new QHBoxLayout(m_filterBar);
    m_filterBarLayout->setContentsMargins(0, 0, 0, 0);
    m_filterBarLayout->setSpacing(2);
    m_filterEdit = new QLineEdit(m_filterBar);
    m_filterEdit->setObjectName("filterEdit");
    m_filterEdit->setFixedHeight(30);
    m_filterEdit->setPlaceholderText(tr("Filter / Search (Enter=next, Shift+Enter=prev, F3=next)"));
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFrame(false);
    m_filterEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_floatNewLogBtn = new QPushButton(m_filterBar);
    m_floatNewLogBtn->setObjectName("floatNewLogBtn");
    m_floatNewLogBtn->setToolTip(tr("Jump to the newest logs"));
    m_floatNewLogBtn->setFixedSize(35, 30);
    {
        QIcon bottomIcon(":/widgetsres/tobottom.png");
        if (bottomIcon.isNull() && style())
            bottomIcon = style()->standardIcon(QStyle::SP_ArrowDown);
        m_floatNewLogBtn->setIcon(bottomIcon);
    }
    m_floatNewLogBtn->setCursor(Qt::PointingHandCursor);
    m_floatNewLogBtn->setFocusPolicy(Qt::NoFocus);
    m_clearLogsBtn = new QPushButton(m_filterBar);
    m_clearLogsBtn->setObjectName("clearBtn");
    {
        QIcon clearIcon(":/widgetsres/clear.png");
        if (clearIcon.isNull() && style())
            clearIcon = style()->standardIcon(QStyle::SP_DialogResetButton);
        m_clearLogsBtn->setIcon(clearIcon);
    }
    m_clearLogsBtn->setIconSize(QSize(14, 14));
    m_clearLogsBtn->setToolTip(tr("Clear All Logs"));
    m_clearLogsBtn->setFixedSize(35, 30);
    m_clearLogsBtn->setCursor(Qt::PointingHandCursor);
    m_clearLogsBtn->setFocusPolicy(Qt::NoFocus);
    m_filterBar->setStyleSheet(QLatin1String(
        "QPushButton{\n"
        "color:black;\n"
        "background-color:rgb(211,211,211);\n"
        "border-radius:1px;\n"
        "}\n"
        "QPushButton:hover{\n"
        "background-color:rgb(192,192,192);\n"
        "}\n"
        "QPushButton:pressed{\n"
        "background-color:rgb(211,211,211);\n"
        "padding-left:1px;\n"
        "padding-top:1px;\n"
        "}\n"
        "QLineEdit{\n"
        "background-color:white;\n"
        "border-style:solid;\n"
        "border-width:1px;\n"
        "border-top-color: rgba(255,255,255,0);\n"
        "border-right-color: rgba(255,255,255,0);\n"
        "border-left-color: rgba(255,255,255,0);\n"
        "border-bottom-color: rgb(218,218,218);\n"
        "}\n"
        "QLineEdit:hover{\n"
        "border-bottom-color: rgb(129,129,129);\n"
        "}\n"
        "QLineEdit:pressed{\n"
        "border-bottom-color: rgb(129,129,129);\n"
        "}"));
    m_filterBarLayout->addWidget(m_filterEdit, 1);
    m_filterBarLayout->addWidget(m_floatNewLogBtn, 0, Qt::AlignRight | Qt::AlignVCenter);
    m_filterBarLayout->addWidget(m_clearLogsBtn, 0, Qt::AlignRight | Qt::AlignVCenter);
}
void TB_CM_LogView::initView()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(4, 4, 4, 4);
    m_mainLayout->setSpacing(4);
    m_edit = new QPlainTextEdit(this);
    m_edit->setObjectName("logTextEdit");
    m_edit->setReadOnly(true);
    m_edit->setUndoRedoEnabled(false);
    m_edit->setMaximumBlockCount(m_maxLogs);
    m_edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_edit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_edit->setAutoFillBackground(true);
    m_edit->setFrameShape(QFrame::NoFrame);
    m_edit->setCenterOnScroll(false);
    m_edit->setContextMenuPolicy(Qt::DefaultContextMenu);
    QPalette palette = m_edit->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, QColor(0x1a, 0x1a, 0x1a));
    palette.setColor(QPalette::Highlight, QColor(0xd6, 0xe6, 0xff));
    palette.setColor(QPalette::HighlightedText, QColor(0x1a, 0x1a, 0x1a));
    m_edit->setPalette(palette);
    QFont font("Consolas");
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(9);
    m_edit->setFont(font);
    m_edit->installEventFilter(this);
    m_edit->viewport()->installEventFilter(this);
    m_filterEdit->installEventFilter(this);
    m_search = new TB_CM_LogSearch(m_edit, this);
    connect(m_search, &TB_CM_LogSearch::scrolledToHit, this, [this]()
            { m_autoScroll = false; });
    m_mainLayout->addWidget(m_filterBar);
    m_mainLayout->addWidget(m_edit, 1);
}
void TB_CM_LogView::initContextMenu()
{
    m_contextMenu = new TB_CM_LogContextMenu(this);
    addAction(m_contextMenu->copyAction());
    addAction(m_contextMenu->selectAllAction());
    addAction(m_contextMenu->findPrevAction());
    addAction(m_contextMenu->findNextAction());
    addAction(m_contextMenu->clearAction());
}
void TB_CM_LogView::showContextMenu(const QPoint &globalPos)
{
    if (!m_contextMenu)
        return;
    const bool hasSearch = !m_search->keyword().isEmpty() && m_search->hitCount() > 0;
    m_contextMenu->popupAt(globalPos, m_edit->textCursor().hasSelection(), hasSearch);
}
void TB_CM_LogView::onCopyAction()
{
    if (m_filterEdit->hasFocus() && m_filterEdit->hasSelectedText())
    {
        m_filterEdit->copy();
        return;
    }
    m_edit->copy();
}
void TB_CM_LogView::onSelectAllAction()
{
    if (m_filterEdit->hasFocus())
    {
        m_filterEdit->selectAll();
        return;
    }
    m_edit->selectAll();
}
void TB_CM_LogView::initConnections()
{
    m_flushTimer = new QTimer(this);
    m_flushTimer->setSingleShot(true);
    connect(m_flushTimer, &QTimer::timeout, this, &TB_CM_LogView::flushPending);
    connect(m_edit->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &TB_CM_LogView::onScrollValueChanged);
    connect(m_contextMenu->copyAction(), &QAction::triggered, this, &TB_CM_LogView::onCopyAction);
    connect(m_contextMenu->selectAllAction(), &QAction::triggered, this, &TB_CM_LogView::onSelectAllAction);
    connect(m_contextMenu->findPrevAction(), &QAction::triggered, this, &TB_CM_LogView::findPrev);
    connect(m_contextMenu->findNextAction(), &QAction::triggered, this, &TB_CM_LogView::findNext);
    connect(m_contextMenu->clearAction(), &QAction::triggered, this, &TB_CM_LogView::clearLogs);
    connect(m_floatNewLogBtn, &QPushButton::clicked, this, [this]()
            {
        m_autoScroll = true;
        scrollToBottom(); });
    connect(m_clearLogsBtn, &QPushButton::clicked, this, &TB_CM_LogView::clearLogs);
    m_searchDebounceTimer = new QTimer(this);
    m_searchDebounceTimer->setSingleShot(true);
    m_searchDebounceTimer->setInterval(200);
    connect(m_searchDebounceTimer, &QTimer::timeout, this, [this]()
            { applyFilterAndSearch(m_filterEdit->text()); });
    connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString &)
            { m_searchDebounceTimer->start(); });
    connect(m_filterEdit, &QLineEdit::returnPressed, this, &TB_CM_LogView::onSearchReturnPressed);
}
void TB_CM_LogView::appendLog(const QString &message, quint64 timestampMs, int level)
{
    TB_CM_LogEntry entry;
    entry.message = message;
    entry.timestampMs = timestampMs;
    entry.level = level;
    appendLog(entry);
}
void TB_CM_LogView::appendLog(const TB_CM_LogEntry &entry)
{
    LogRecord record;
    record.level = entry.level;
    record.line = formatLogLine(entry.timestampMs, entry.level, entry.message);
    {
        QMutexLocker locker(&m_mutex);
        if (m_pending.size() >= DEFAULT_PENDING_MAX)
        {
            const int dropCount = m_pending.size() - DEFAULT_PENDING_MAX + 1;
            m_pending.remove(0, dropCount);
            m_droppedCount.fetch_add(dropCount, std::memory_order_relaxed);
        }
        m_pending.append(std::move(record));
    }
    if (!m_flushScheduled.exchange(true, std::memory_order_acq_rel))
    {
        if (QThread::currentThread() == thread())
            startFlushTimer();
        else
            QMetaObject::invokeMethod(this, &TB_CM_LogView::startFlushTimer, Qt::QueuedConnection);
    }
}
void TB_CM_LogView::startFlushTimer()
{
    if (m_flushTimer->isActive())
        return;
    const int delay = m_flushNow ? 0 : m_flushIntervalMs;
    m_flushNow = false;
    m_flushTimer->start(delay);
}
void TB_CM_LogView::flushPending()
{
    m_flushScheduled.store(false, std::memory_order_release);
    QVector<LogRecord> batch;
    int backlog = 0;
    {
        QMutexLocker locker(&m_mutex);
        const int pendingCount = m_pending.size();
        if (pendingCount == 0)
            return;
        const int take = qMin(pendingCount, kMaxBatchPerFlush);
        batch.reserve(take);
        for (int i = 0; i < take; ++i)
            batch.append(std::move(m_pending[i]));
        m_pending.remove(0, take);
        backlog = m_pending.size();
    }
    applyPending(batch);
    if (backlog > 0 && !m_flushScheduled.exchange(true, std::memory_order_acq_rel))
    {
        m_flushNow = true;
        startFlushTimer();
    }
}
void TB_CM_LogView::applyPending(const QVector<LogRecord> &batch)
{
    if (batch.isEmpty())
        return;
    const bool stayAtBottom = m_autoScroll;
    if (m_overflowPolicy == OP_CLEAR_ALL && m_records.size() + batch.size() > m_maxLogs)
    {
        m_records.clear();
        m_edit->clear();
        m_edit->setMaximumBlockCount(m_maxLogs);
        const int from = qMax(0, batch.size() - m_maxLogs);
        m_records.reserve(batch.size() - from);
        for (int i = from; i < batch.size(); ++i)
            m_records.append(batch.at(i));
        insertRecords(batch, from, batch.size());
        m_search->invalidate();
        if (stayAtBottom)
            scrollToBottom();
        emit logAppended(batch.size() - from);
        return;
    }
    m_records.reserve(m_records.size() + batch.size());
    for (const LogRecord &record : batch)
        m_records.append(record);
    const int overflow = m_records.size() - m_maxLogs;
    if (overflow > 0)
        m_records.remove(0, overflow);
    insertRecords(batch, 0, batch.size());
    m_search->invalidate();
    if (stayAtBottom)
        scrollToBottom();
    emit logAppended(batch.size());
}
void TB_CM_LogView::insertRecords(const QVector<LogRecord> &records, int from, int to)
{
    if (from >= to)
        return;
    QTextDocument *document = m_edit->document();
    QTextCursor cursor(document);
    cursor.movePosition(QTextCursor::End);
    const bool hasFilter = !m_filterKeyword.isEmpty();
    bool needNewBlock = !document->isEmpty();
    cursor.beginEditBlock();
    for (int i = from; i < to; ++i)
    {
        const LogRecord &record = records.at(i);
        if (hasFilter && record.line.indexOf(m_filterKeyword, 0, Qt::CaseInsensitive) < 0)
            continue;
        if (needNewBlock)
            cursor.insertBlock();
        cursor.insertText(record.line, levelFormat(record.level));
        needNewBlock = true;
    }
    cursor.endEditBlock();
}
void TB_CM_LogView::rebuildDocument()
{
    m_edit->setUpdatesEnabled(false);
    m_edit->clear();
    m_edit->setMaximumBlockCount(m_maxLogs);
    insertRecords(m_records, 0, m_records.size());
    m_edit->setUpdatesEnabled(true);
    m_edit->viewport()->update();
    m_search->invalidate();
    m_search->clearSelection();
    m_autoScroll = true;
    scrollToBottom();
}
void TB_CM_LogView::setFilterKeyword(const QString &keyword)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, [this, keyword]()
                                  { setFilterKeyword(keyword); }, Qt::QueuedConnection);
        return;
    }
    if (QSignalBlocker blocker(m_filterEdit); m_filterEdit->text() != keyword)
        m_filterEdit->setText(keyword);
    m_searchDebounceTimer->stop();
    applyFilterAndSearch(keyword);
}
QString TB_CM_LogView::filterKeyword() const
{
    return m_filterKeyword;
}
void TB_CM_LogView::applyFilterAndSearch(const QString &keyword)
{
    const QString trimmed = keyword.trimmed();
    const bool filterChanged = (trimmed != m_filterKeyword);
    m_filterKeyword = trimmed;
    if (filterChanged)
        rebuildDocument();
    m_search->setKeyword(trimmed);
    emit searchHitsChanged(m_search->hitCount());
}
void TB_CM_LogView::search(const QString &keyword)
{
    setFilterKeyword(keyword);
    if (!keyword.trimmed().isEmpty())
        m_search->findNext();
}
void TB_CM_LogView::findNext()
{
    m_search->findNext();
}
void TB_CM_LogView::findPrev()
{
    m_search->findPrev();
}
QString TB_CM_LogView::searchKeyword() const
{
    return m_search->keyword();
}
int TB_CM_LogView::searchHitCount() const
{
    return m_search->hitCount();
}
void TB_CM_LogView::onSearchReturnPressed()
{
    m_searchDebounceTimer->stop();
    const QString trimmed = m_filterEdit->text().trimmed();
    if (trimmed != m_filterKeyword || trimmed != m_search->keyword())
        applyFilterAndSearch(m_filterEdit->text());
    else
        findNext();
}
void TB_CM_LogView::scrollToBottom()
{
    m_autoScroll = true;
    m_edit->moveCursor(QTextCursor::End);
    m_edit->ensureCursorVisible();
    QScrollBar *bar = m_edit->verticalScrollBar();
    bar->setValue(bar->maximum());
}
void TB_CM_LogView::onScrollValueChanged(int value)
{
    const QScrollBar *bar = m_edit->verticalScrollBar();
    m_autoScroll = (value >= bar->maximum() - 2);
}
void TB_CM_LogView::setMaxLogs(int maxLogs)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, [this, maxLogs]()
                                  { setMaxLogsImpl(maxLogs); }, Qt::QueuedConnection);
        return;
    }
    setMaxLogsImpl(maxLogs);
}
void TB_CM_LogView::setMaxLogsImpl(int maxLogs)
{
    if (maxLogs < 100)
        maxLogs = 100;
    if (m_maxLogs == maxLogs)
        return;
    m_maxLogs = maxLogs;
    m_edit->setMaximumBlockCount(m_maxLogs);
    if (m_records.size() > m_maxLogs)
    {
        m_records.remove(0, m_records.size() - m_maxLogs);
        rebuildDocument();
    }
}
int TB_CM_LogView::maxLogs() const
{
    return m_maxLogs;
}
void TB_CM_LogView::setOverflowPolicy(E_OverflowPolicy policy)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, [this, policy]()
                                  { setOverflowPolicy(policy); }, Qt::QueuedConnection);
        return;
    }
    m_overflowPolicy = policy;
}
E_OverflowPolicy TB_CM_LogView::overflowPolicy() const
{
    return m_overflowPolicy;
}
void TB_CM_LogView::clearLogs()
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, [this]()
                                  { clearLogs(); }, Qt::QueuedConnection);
        return;
    }
    {
        QMutexLocker locker(&m_mutex);
        m_pending.clear();
    }
    m_records.clear();
    m_edit->clear();
    m_edit->setMaximumBlockCount(m_maxLogs);
    m_search->clearHits();
    m_search->clearSelection();
    m_autoScroll = true;
    emit searchHitsChanged(0);
    emit logsCleared();
}
qint64 TB_CM_LogView::droppedCount() const
{
    return m_droppedCount.load(std::memory_order_acquire);
}
void TB_CM_LogView::resetDroppedCount()
{
    m_droppedCount.store(0, std::memory_order_release);
}
void TB_CM_LogView::setFlushInterval(int ms)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, [this, ms]()
                                  { setFlushInterval(ms); }, Qt::QueuedConnection);
        return;
    }
    if (ms < 1)
        ms = 1;
    m_flushIntervalMs = ms;
}
int TB_CM_LogView::flushInterval() const
{
    return m_flushIntervalMs;
}
QPlainTextEdit *TB_CM_LogView::textEdit() const
{
    return m_edit;
}
QTextCharFormat TB_CM_LogView::levelFormat(int level) const
{
    if (level < 0 || level > 5)
        level = L_INFO;
    return m_levelFormats[level];
}
QString TB_CM_LogView::levelToString(int level)
{
    switch (level)
    {
    case L_INFO:
        return tr("INFO");
    case L_DEBUG:
        return tr("DEBUG");
    case L_WARN:
        return tr("WARN");
    case L_ERROR:
        return tr("ERROR");
    case L_TRACE:
        return tr("TRACE");
    case L_CRITICAL:
        return tr("CRITICAL");
    default:
        return tr("UNKNOWN");
    }
}
QString TB_CM_LogView::nowTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
}
bool TB_CM_LogView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ContextMenu && watched == m_edit->viewport())
    {
        showContextMenu(static_cast<QContextMenuEvent *>(event)->globalPos());
        return true;
    }
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_F3)
        {
            if (ke->modifiers() & Qt::ShiftModifier)
                findPrev();
            else
                findNext();
            return true;
        }
        if (watched == m_filterEdit && ke->modifiers() == Qt::ShiftModifier &&
            (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter))
        {
            m_searchDebounceTimer->stop();
            findPrev();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
