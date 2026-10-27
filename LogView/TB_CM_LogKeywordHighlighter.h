#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QString>

class QTextDocument;

class TB_CM_LogKeywordHighlighter : public QSyntaxHighlighter
{
public:
    explicit TB_CM_LogKeywordHighlighter(QTextDocument *document, QObject *parent = nullptr);
    void setKeyword(const QString &keyword);
    QString keyword() const;
protected:
    void highlightBlock(const QString &text) override;
private:
    QString m_keyword;
    QTextCharFormat m_format;
};
