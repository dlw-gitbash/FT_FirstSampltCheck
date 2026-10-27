#include "TB_CM_LogView.h"

#include "TB_CM_LogContextMenu.h"
#include "TB_CM_LogSearch.h"

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextOption>
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
#include <QElapsedTimer>

#include <algorithm>
#include <utility>

namespace
{
constexpr int kMaxBatchPerFlush = 500;
constexpr int kFlushBudgetMs = 50;

const QString &levelPrefixFor(int level)
{
    static const QString prefixes[LogLevel::Count] = {
        QStringLiteral(" - TRACE | "),
        QStringLiteral(" - DEBUG | "),
        QStringLiteral(" - INFO | "),
        QStringLiteral(" - WARN | "),
        QStringLiteral(" - ERROR | "),
        QStringLiteral(" - CRITICAL | ")
    };
    if (level < LogLevel::Trace || level >= LogLevel::Count)
        level = LogLevel::Info;
    return prefixes[level];
}

QString formatLogLine(quint64 timestampMs, int level, const QString &message)
{
    const qint64 rawTs = (timestampMs != 0) ? static_cast<qint64>(timestampMs)
                                            : QDateTime::currentMSecsSinceEpoch();
    const qint64 ts = (rawTs > 0) ? rawTs : QDateTime::currentMSecsSinceEpoch();

    const QString *body = &message;
    QString flattened;
    if (message.contains(QLatin1Char('\r')) || message.contains(QLatin1Char('\n')))
    {
        flattened = message;
        flattened.replace(QStringLiteral("\r\n"), QStringLiteral(" "));
        flattened.replace(QLatin1Char('\r'), QLatin1Char(' '));
        flattened.replace(QLatin1Char('\n'), QLatin1Char(' '));
        body = &flattened;
    }

    const qint64 second = ts / 1000;
    const int msec = static_cast<int>(ts % 1000);
    thread_local qint64 cachedSecond = -1;
    thread_local QString cachedPrefix;
    if (second != cachedSecond)
    {
        cachedSecond = second;
        cachedPrefix = QDateTime::fromMSecsSinceEpoch(second * 1000)
                           .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    const QString &levelPrefix = levelPrefixFor(level);

    QString line;
    line.reserve(cachedPrefix.size() + 4 + levelPrefix.size() + body->size());
    line += cachedPrefix;
    line += QLatin1Char('.');
    QChar millis[3];
    millis[0] = QChar(static_cast<ushort>('0' + msec / 100));
    millis[1] = QChar(static_cast<ushort>('0' + (msec / 10) % 10));
    millis[2] = QChar(static_cast<ushort>('0' + msec % 10));
    line.append(millis, 3);
    line += levelPrefix;
    line += *body;
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

    const QColor levelColors[LogLevel::Count] = {
        QColor(0x80, 0x80, 0x80),
        QColor(0x00, 0x00, 0xff),
        QColor(0x00, 0x00, 0x00),
        QColor(0xff, 0x88, 0x00),
        QColor(0xff, 0x00, 0x00),
        QColor(0xcc, 0x00, 0x66)
    };
    for (int i = 0; i < LogLevel::Count; ++i)
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
        m_floatNewLogBtn->setIconSize(QSize(14, 14));
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
    m_edit->setContextMenuPolicy(Qt::NoContextMenu);
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
    m_mainLayout->addWidget(m_filterBar);
    m_mainLayout->addWidget(m_edit, 1);
}
void TB_CM_LogView::initContextMenu()
{
    m_contextMenu = new TB_CM_LogContextMenu(this);
    QAction *actions[] = {
        m_contextMenu->copyAction(),
        m_contextMenu->selectAllAction(),
        m_contextMenu->findPrevAction(),
        m_contextMenu->findNextAction(),
        m_contextMenu->clearAction()
    };
    for (QAction *action : actions)
    {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(action);
    }
}
void TB_CM_LogView::showContextMenu(const QPoint &globalPos)
{
    if (!m_contextMenu)
        return;
    const bool hasSelection = m_edit->textCursor().hasSelection();
    m_contextMenu->popupAt(globalPos, hasSelection, m_search->hitCount() > 0);
}
void TB_CM_LogView::onCopyAction()
{
    if (m_filterEdit->hasFocus())
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

    connect(m_search, &TB_CM_LogSearch::scrolledToHit, this, [this]()
            { m_autoScroll = false; });
    connect(m_search, &TB_CM_LogSearch::hitsChanged, this, &TB_CM_LogView::searchHitsChanged);

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

    int dropped = 0;
    {
        QMutexLocker locker(&m_mutex);
        if (m_pending.size() >= static_cast<size_t>(DEFAULT_PENDING_MAX))
        {
            m_pending.pop_front();
            m_droppedCount.fetch_add(1, std::memory_order_relaxed);
            ++dropped;
        }
        m_pending.push_back(std::move(record));
    }
    if (dropped > 0)
        emit logsDropped(dropped);
    if (!m_flushScheduled.load(std::memory_order_acquire))
    {
        if (QThread::currentThread() == thread())
            scheduleFlush();
        else
            QMetaObject::invokeMethod(this, &TB_CM_LogView::scheduleFlush, Qt::QueuedConnection);
    }
}
void TB_CM_LogView::appendLogs(const QVector<TB_CM_LogEntry> &entries)
{
    const int count = static_cast<int>(entries.size());
    if (count == 0)
        return;

    QVector<LogRecord> prepared;
    prepared.reserve(count);
    for (const TB_CM_LogEntry &entry : entries)
    {
        LogRecord record;
        record.level = entry.level;
        record.line = formatLogLine(entry.timestampMs, entry.level, entry.message);
        prepared.append(std::move(record));
    }

    int dropped = 0;
    {
        QMutexLocker locker(&m_mutex);
        for (LogRecord &record : prepared)
        {
            if (m_pending.size() >= static_cast<size_t>(DEFAULT_PENDING_MAX))
            {
                m_pending.pop_front();
                m_droppedCount.fetch_add(1, std::memory_order_relaxed);
                ++dropped;
            }
            m_pending.push_back(std::move(record));
        }
    }

    if (dropped > 0)
        emit logsDropped(dropped);
    if (!m_flushScheduled.load(std::memory_order_acquire))
    {
        if (QThread::currentThread() == thread())
            scheduleFlush();
        else
            QMetaObject::invokeMethod(this, &TB_CM_LogView::scheduleFlush, Qt::QueuedConnection);
    }
}
void TB_CM_LogView::scheduleFlush()
{
    m_flushScheduled.store(true, std::memory_order_release);
    if (m_flushTimer->isActive())
        return;
    const int delay = m_flushNow ? 0 : m_flushIntervalMs;
    m_flushNow = false;
    m_flushTimer->start(delay);
}
void TB_CM_LogView::flushNow()
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, &TB_CM_LogView::flushNow, Qt::QueuedConnection);
        return;
    }
    if (m_flushTimer)
        m_flushTimer->stop();
    m_flushScheduled.store(false, std::memory_order_release);

    QElapsedTimer budget;
    budget.start();
    for (;;)
    {
        QVector<LogRecord> batch;
        size_t backlog = 0;
        if (!takeBatch(batch, backlog))
            break;
        applyPending(batch);
        if (budget.elapsed() >= kFlushBudgetMs)
            break;
    }

    bool empty = false;
    {
        QMutexLocker locker(&m_mutex);
        empty = m_pending.empty();
    }
    if (!empty)
    {
        m_flushNow = true;
        scheduleFlush();
    }
}
bool TB_CM_LogView::takeBatch(QVector<LogRecord> &batch, size_t &backlog)
{
    QMutexLocker locker(&m_mutex);
    const size_t pendingCount = m_pending.size();
    if (pendingCount == 0)
        return false;

    const size_t take = std::min(pendingCount, static_cast<size_t>(kMaxBatchPerFlush));
    batch.reserve(static_cast<int>(take));
    for (size_t i = 0; i < take; ++i)
    {
        batch.append(std::move(m_pending.front()));
        m_pending.pop_front();
    }
    backlog = m_pending.size();
    return true;
}
void TB_CM_LogView::flushPending()
{
    m_flushScheduled.store(false, std::memory_order_release);

    QVector<LogRecord> batch;
    size_t backlog = 0;
    if (!takeBatch(batch, backlog))
        return;

    QElapsedTimer timer;
    timer.start();
    const int inserted = applyPending(batch);
    emit flushTimed(inserted, timer.nsecsElapsed());

    if (backlog > 0)
    {
        m_flushNow = true;
        scheduleFlush();
    }
}
int TB_CM_LogView::applyPending(const QVector<LogRecord> &batch)
{
    const int batchSize = static_cast<int>(batch.size());
    if (batchSize == 0)
        return 0;

    const bool stayAtBottom = m_autoScroll;

    if (m_overflowPolicy == OP_CLEAR_ALL
        && static_cast<int>(m_records.size()) + batchSize > m_maxLogs)
    {
        const int from = qMax(0, batchSize - m_maxLogs);
        m_records.clear();
        for (int i = from; i < batchSize; ++i)
            m_records.push_back(batch.at(i));

        m_edit->clear();
        m_search->beginRebuild();
        const int inserted = insertLines(batch, from, batchSize);
        m_search->endBatch();

        if (stayAtBottom)
            scrollToBottom();
        emit logAppended(inserted);
        return inserted;
    }

    for (const LogRecord &record : batch)
        m_records.push_back(record);
    while (static_cast<int>(m_records.size()) > m_maxLogs)
        m_records.pop_front();

    m_search->beginInsert();
    const int inserted = insertLines(batch, 0, batchSize);
    m_search->endBatch();

    if (stayAtBottom)
        scrollToBottom();
    emit logAppended(inserted);
    return inserted;
}
int TB_CM_LogView::insertLines(const QVector<LogRecord> &records, int from, int to)
{
    const int total = static_cast<int>(records.size());
    if (from < 0)
        from = 0;
    if (to > total)
        to = total;
    if (from >= to)
        return 0;

    const bool hasFilter = !m_filterKeyword.isEmpty();
    const QString &filter = m_filterKeyword;

    QVector<int> visible;
    visible.reserve(to - from);
    for (int i = from; i < to; ++i)
    {
        if (hasFilter && records.at(i).line.indexOf(filter, 0, Qt::CaseInsensitive) < 0)
            continue;
        visible.append(i);
    }
    const int count = static_cast<int>(visible.size());
    if (count == 0)
        return 0;

    QPlainTextEdit *edit = m_edit;
    const bool docWasEmpty = edit->document()->isEmpty();

    QTextCursor cursor(edit->document());
    cursor.movePosition(QTextCursor::End);

    bool firstChunk = true;
    cursor.beginEditBlock();
    int k = 0;
    while (k < count)
    {
        const int level = records.at(visible.at(k)).level;
        int end = k + 1;
        while (end < count && records.at(visible.at(end)).level == level)
            ++end;

        QString chunk;
        qsizetype needed = static_cast<qsizetype>(end - k);
        for (int t = k; t < end; ++t)
            needed += records.at(visible.at(t)).line.size();
        chunk.reserve(needed + 1);

        for (int t = k; t < end; ++t)
        {
            const QString &line = records.at(visible.at(t)).line;
            if (t > k)
                chunk += QLatin1Char('\n');
            chunk += line;
            m_search->onLineInserted(line);
        }
        if (!docWasEmpty || !firstChunk)
            chunk.prepend(QLatin1Char('\n'));
        firstChunk = false;

        cursor.insertText(chunk, levelFormat(level));
        k = end;
    }
    cursor.endEditBlock();

    return count;
}
void TB_CM_LogView::rebuildDocument()
{
    const bool stayAtBottom = m_autoScroll;

    m_edit->setUpdatesEnabled(false);

    m_edit->clear();
    m_search->beginRebuild();
    m_search->setKeyword(m_filterKeyword);

    if (!m_records.empty())
    {
        QVector<LogRecord> snapshot;
        snapshot.reserve(static_cast<int>(m_records.size()));
        for (const LogRecord &record : m_records)
            snapshot.append(record);
        insertLines(snapshot, 0, static_cast<int>(snapshot.size()));
    }
    m_search->endBatch();

    m_edit->setUpdatesEnabled(true);
    m_edit->viewport()->update();

    m_autoScroll = stayAtBottom;
    if (stayAtBottom)
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
    if (m_filterEdit->text() != keyword)
    {
        QSignalBlocker blocker(m_filterEdit);
        m_filterEdit->setText(keyword);
    }
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
    if (trimmed != m_filterKeyword)
    {
        m_filterKeyword = trimmed;
        rebuildDocument();
        return;
    }
    m_search->setKeyword(trimmed);
}
void TB_CM_LogView::search(const QString &keyword)
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, [this, keyword]()
                                  { search(keyword); }, Qt::QueuedConnection);
        return;
    }
    setFilterKeyword(keyword);
    if (!keyword.trimmed().isEmpty())
        m_search->findNext();
}
void TB_CM_LogView::findNext()
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, &TB_CM_LogView::findNext, Qt::QueuedConnection);
        return;
    }
    m_search->findNext();
}
void TB_CM_LogView::findPrev()
{
    if (QThread::currentThread() != thread())
    {
        QMetaObject::invokeMethod(this, &TB_CM_LogView::findPrev, Qt::QueuedConnection);
        return;
    }
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
    const QString keyword = m_filterEdit->text();
    if (keyword.trimmed() != m_filterKeyword)
        applyFilterAndSearch(keyword);
    findNext();
}
void TB_CM_LogView::scrollToBottom()
{
    m_autoScroll = true;
    if (!m_edit->textCursor().hasSelection())
    {
        m_edit->moveCursor(QTextCursor::End);
        m_edit->ensureCursorVisible();
    }
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

    bool trimmed = false;
    while (static_cast<int>(m_records.size()) > m_maxLogs)
    {
        m_records.pop_front();
        trimmed = true;
    }
    if (trimmed)
        rebuildDocument();
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
TB_CM_LogView::E_OverflowPolicy TB_CM_LogView::overflowPolicy() const
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
    m_search->beginRebuild();
    m_search->endBatch();
    m_autoScroll = true;
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
const QPlainTextEdit *TB_CM_LogView::textEdit() const
{
    return m_edit;
}
QTextCharFormat TB_CM_LogView::levelFormat(int level) const
{
    if (level < LogLevel::Trace || level >= LogLevel::Count)
        level = LogLevel::Info;
    return m_levelFormats[level];
}
QString TB_CM_LogView::levelToString(int level)
{
    switch (level)
    {
    case LogLevel::Info:
        return tr("INFO");
    case LogLevel::Debug:
        return tr("DEBUG");
    case LogLevel::Warn:
        return tr("WARN");
    case LogLevel::Error:
        return tr("ERROR");
    case LogLevel::Trace:
        return tr("TRACE");
    case LogLevel::Critical:
        return tr("CRITICAL");
    default:
        return tr("UNKNOWN");
    }
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