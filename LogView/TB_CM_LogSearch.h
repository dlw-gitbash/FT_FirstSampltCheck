#pragma once

#include <QObject>
#include <QString>

#include <deque>

class QPlainTextEdit;

class TB_CM_LogSearch : public QObject
{
    Q_OBJECT
public:
    explicit TB_CM_LogSearch(QPlainTextEdit *edit, QObject *parent = nullptr);

    void setKeyword(const QString &keyword);
    QString keyword() const;

    void beginRebuild();
    void beginInsert();
    void onLineInserted(const QString &line);
    void endBatch();

    void clearSelection();
    int hitCount();
    void findNext();
    void findPrev();

signals:
    void scrolledToHit();
    void hitsChanged(int count);

private:
    void rebuildFromDocument();
    void purgeDropped();
    void applySelection();
    void selectCurrentHit();
    int docBlockCount() const;
    qint64 droppedBlocks() const;
    int blockOfOrdinal(qint64 ordinal) const;

    QPlainTextEdit *m_edit;
    QString m_keyword;
    std::deque<qint64> m_hits;
    qint64 m_insertedBlocks;
    qint64 m_currentOrdinal;
    int m_lastEmitted;
    bool m_batching;
};