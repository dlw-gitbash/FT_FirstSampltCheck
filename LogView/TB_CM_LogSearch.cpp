#include "TB_CM_LogSearch.h"

#include "TB_CM_LogKeywordHighlighter.h"

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextEdit>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

TB_CM_LogSearch::TB_CM_LogSearch(QPlainTextEdit *edit, QObject *parent)
    : QObject(parent),
      m_edit(edit),
      m_highlighter(new TB_CM_LogKeywordHighlighter(edit->document(), this)),
      m_currentHit(-1),
      m_hitsDirty(true)
{
}
void TB_CM_LogSearch::setKeyword(const QString &keyword)
{
    m_keyword = keyword;
    m_highlighter->setKeyword(keyword);
    m_hitsDirty = true;
    m_hitBlocks.clear();
    m_currentHit = -1;
    ensureHits();
    if (!keyword.isEmpty() && !m_hitBlocks.isEmpty())
    {
        m_currentHit = 0;
        selectCurrentHit();
    }
    else
    {
        m_edit->setExtraSelections({});
    }
}
QString TB_CM_LogSearch::keyword() const
{
    return m_keyword;
}
void TB_CM_LogSearch::invalidate()
{
    m_hitsDirty = true;
    m_hitBlocks.clear();
    m_currentHit = -1;
}
void TB_CM_LogSearch::clearHits()
{
    m_hitBlocks.clear();
    m_currentHit = -1;
    m_hitsDirty = false;
}
void TB_CM_LogSearch::clearSelection()
{
    m_edit->setExtraSelections({});
}
int TB_CM_LogSearch::hitCount() const
{
    ensureHits();
    return m_hitBlocks.size();
}
void TB_CM_LogSearch::findNext()
{
    gotoHit(true);
}
void TB_CM_LogSearch::findPrev()
{
    gotoHit(false);
}
void TB_CM_LogSearch::ensureHits() const
{
    if (!m_hitsDirty)
        return;
    m_hitsDirty = false;
    m_hitBlocks.clear();
    m_currentHit = -1;
    if (m_keyword.isEmpty())
        return;
    int blockNumber = 0;
    for (QTextBlock block = m_edit->document()->begin(); block.isValid(); block = block.next(), ++blockNumber)
    {
        if (block.text().contains(m_keyword, Qt::CaseInsensitive))
            m_hitBlocks.append(blockNumber);
    }
}
void TB_CM_LogSearch::gotoHit(bool forward)
{
    ensureHits();
    const int count = m_hitBlocks.size();
    if (count <= 0)
        return;
    if (m_currentHit < 0)
        m_currentHit = forward ? 0 : count - 1;
    else if (forward)
        m_currentHit = (m_currentHit + 1) % count;
    else
        m_currentHit = (m_currentHit - 1 + count) % count;
    selectCurrentHit();
}
void TB_CM_LogSearch::selectCurrentHit()
{
    if (m_currentHit < 0 || m_currentHit >= m_hitBlocks.size())
    {
        m_edit->setExtraSelections({});
        return;
    }
    const QTextBlock block = m_edit->document()->findBlockByNumber(m_hitBlocks.at(m_currentHit));
    if (!block.isValid())
    {
        m_edit->setExtraSelections({});
        return;
    }
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(0xff, 0xd6, 0x00));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = QTextCursor(block);
    m_edit->setExtraSelections({selection});
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::EndOfBlock);
    m_edit->setTextCursor(cursor);
    m_edit->ensureCursorVisible();
    emit scrolledToHit();
}
