#pragma once

#include <QObject>
#include <QString>
#include <QList>

class QPlainTextEdit;
class TB_CM_LogKeywordHighlighter;

class TB_CM_LogSearch : public QObject
{
    Q_OBJECT
public:
    explicit TB_CM_LogSearch(QPlainTextEdit *edit, QObject *parent = nullptr);
    void setKeyword(const QString &keyword);
    QString keyword() const;
    void invalidate();
    void clearHits();
    void clearSelection();
    int hitCount() const;
    void findNext();
    void findPrev();
signals:
    void scrolledToHit();
private:
    void ensureHits() const;
    void gotoHit(bool forward);
    void selectCurrentHit();
    QPlainTextEdit *m_edit;
    TB_CM_LogKeywordHighlighter *m_highlighter;
    QString m_keyword;
    mutable QList<int> m_hitBlocks;
    mutable int m_currentHit;
    mutable bool m_hitsDirty;
};
