#include "TB_CM_LogSearch.h"

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextFormat>

#include <algorithm>

TB_CM_LogSearch::TB_CM_LogSearch(QPlainTextEdit *edit, QObject *parent)
    : QObject(parent),
      m_edit(edit),
      m_insertedBlocks(0),
      m_currentOrdinal(-1),
      m_lastEmitted(0),
      m_batching(false)
{
}

void TB_CM_LogSearch::setKeyword(const QString &keyword)
{
    if (m_keyword == keyword)
        return;
    m_keyword = keyword;
    rebuildFromDocument();
    if (m_batching)
        return;
    const int count = static_cast<int>(m_hits.size());
    if (count != m_lastEmitted)
    {
        m_lastEmitted = count;
        emit hitsChanged(count);
    }
}
QString TB_CM_LogSearch::keyword() const
{
    return m_keyword;
}
void TB_CM_LogSearch::beginRebuild()
{
    m_batching = true;
    m_insertedBlocks = 0;
    m_hits.clear();
    m_currentOrdinal = -1;
    m_edit->setExtraSelections({});
}
void TB_CM_LogSearch::beginInsert()
{
    m_batching = true;
}
void TB_CM_LogSearch::onLineInserted(const QString &line)
{
    const qint64 ordinal = m_insertedBlocks++;
    if (m_keyword.isEmpty())
        return;
    if (line.contains(m_keyword, Qt::CaseInsensitive))
        m_hits.push_back(ordinal);
}
void TB_CM_LogSearch::endBatch()
{
    m_batching = false;
    purgeDropped();
    if (m_currentOrdinal >= 0)
        applySelection();
    const int count = static_cast<int>(m_hits.size());
    if (count != m_lastEmitted)
    {
        m_lastEmitted = count;
        emit hitsChanged(count);
    }
}
void TB_CM_LogSearch::clearSelection()
{
    m_edit->setExtraSelections({});
}
int TB_CM_LogSearch::hitCount()
{
    purgeDropped();
    return static_cast<int>(m_hits.size());
}
void TB_CM_LogSearch::findNext()
{
    purgeDropped();
    if (m_hits.empty())
    {
        m_currentOrdinal = -1;
        clearSelection();
        return;
    }
    auto it = std::upper_bound(m_hits.begin(), m_hits.end(), m_currentOrdinal);
    if (it == m_hits.end())
        it = m_hits.begin();
    m_currentOrdinal = *it;
    selectCurrentHit();
}
void TB_CM_LogSearch::findPrev()
{
    purgeDropped();
    if (m_hits.empty())
    {
        m_currentOrdinal = -1;
        clearSelection();
        return;
    }
    auto it = std::lower_bound(m_hits.begin(), m_hits.end(), m_currentOrdinal);
    if (it == m_hits.begin())
        it = m_hits.end();
    --it;
    m_currentOrdinal = *it;
    selectCurrentHit();
}
int TB_CM_LogSearch::docBlockCount() const
{
    const QTextDocument *document = m_edit->document();
    if (document->isEmpty())
        return 0;
    return static_cast<int>(document->blockCount());
}
qint64 TB_CM_LogSearch::droppedBlocks() const
{
    const qint64 blocks = docBlockCount();
    return (m_insertedBlocks > blocks) ? (m_insertedBlocks - blocks) : 0;
}
int TB_CM_LogSearch::blockOfOrdinal(qint64 ordinal) const
{
    const qint64 blockNumber = ordinal - droppedBlocks();
    if (blockNumber < 0)
        return -1;
    return static_cast<int>(blockNumber);
}
void TB_CM_LogSearch::purgeDropped()
{
    if (m_batching)
        return;
    const qint64 dropped = droppedBlocks();
    if (dropped <= 0)
        return;
    while (!m_hits.empty() && m_hits.front() < dropped)
        m_hits.pop_front();
    if (m_currentOrdinal >= 0 && m_currentOrdinal < dropped)
        m_currentOrdinal = -1;
}
void TB_CM_LogSearch::rebuildFromDocument()
{
    m_hits.clear();
    m_currentOrdinal = -1;
    const int blocks = docBlockCount();
    m_insertedBlocks = blocks;
    if (m_keyword.isEmpty() || blocks == 0)
        return;

    const QTextDocument *document = m_edit->document();
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
    {
        if (block.text().contains(m_keyword, Qt::CaseInsensitive))
            m_hits.push_back(block.blockNumber());
    }
}
void TB_CM_LogSearch::applySelection()
{
    if (m_currentOrdinal < 0)
    {
        clearSelection();
        return;
    }
    const int blockNumber = blockOfOrdinal(m_currentOrdinal);
    if (blockNumber < 0)
    {
        clearSelection();
        return;
    }
    const QTextBlock block = m_edit->document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
    {
        clearSelection();
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(0xff, 0xd6, 0x00));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = QTextCursor(block);
    m_edit->setExtraSelections({ selection });
}
void TB_CM_LogSearch::selectCurrentHit()
{
    applySelection();

    const int blockNumber = blockOfOrdinal(m_currentOrdinal);
    if (blockNumber < 0)
        return;
    const QTextBlock block = m_edit->document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
        return;

    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::EndOfBlock);
    m_edit->setTextCursor(cursor);
    m_edit->ensureCursorVisible();
    emit scrolledToHit();
}