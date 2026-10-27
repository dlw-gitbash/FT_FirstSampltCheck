#include "TB_CM_LogKeywordHighlighter.h"

#include <QTextDocument>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

TB_CM_LogKeywordHighlighter::TB_CM_LogKeywordHighlighter(QTextDocument *document, QObject *parent)
    : QSyntaxHighlighter(document)
{
    if (parent)
        setParent(parent);
    m_format.setBackground(QColor(255, 255, 0));
    m_format.setForeground(QColor(0, 0, 0));
}
void TB_CM_LogKeywordHighlighter::setKeyword(const QString &keyword)
{
    if (m_keyword == keyword)
        return;
    m_keyword = keyword;
    rehighlight();
}
QString TB_CM_LogKeywordHighlighter::keyword() const
{
    return m_keyword;
}
void TB_CM_LogKeywordHighlighter::highlightBlock(const QString &text)
{
    if (m_keyword.isEmpty())
        return;
    const int length = m_keyword.size();
    int pos = 0;
    while ((pos = text.indexOf(m_keyword, pos, Qt::CaseInsensitive)) >= 0)
    {
        setFormat(pos, length, m_format);
        pos += length;
    }
}
