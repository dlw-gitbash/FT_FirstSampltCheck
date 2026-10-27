#pragma once

#include <QMenu>

class QAction;

class TB_CM_LogContextMenu : public QMenu
{
    Q_OBJECT
public:
    explicit TB_CM_LogContextMenu(QWidget *parent = nullptr);
    QAction *copyAction() const;
    QAction *selectAllAction() const;
    QAction *findPrevAction() const;
    QAction *findNextAction() const;
    QAction *clearAction() const;
    void popupAt(const QPoint &globalPos, bool hasSelection, bool hasSearch);
private:
    QAction *m_copyAction;
    QAction *m_selectAllAction;
    QAction *m_findPrevAction;
    QAction *m_findNextAction;
    QAction *m_clearAction;
};
