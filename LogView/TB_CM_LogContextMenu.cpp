#include "TB_CM_LogContextMenu.h"

#include <QAction>
#include <QKeySequence>

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma execution_character_set("utf-8")
#endif

TB_CM_LogContextMenu::TB_CM_LogContextMenu(QWidget *parent)
    : QMenu(parent)
{
    m_copyAction = new QAction(tr("Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip(tr("Copy selected logs to clipboard"));
    addAction(m_copyAction);

    m_selectAllAction = new QAction(tr("Select All"), this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_selectAllAction->setStatusTip(tr("Select all visible logs"));
    addAction(m_selectAllAction);

    addSeparator();

    m_findPrevAction = new QAction(tr("Find Previous"), this);
    m_findPrevAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3));
    addAction(m_findPrevAction);

    m_findNextAction = new QAction(tr("Find Next"), this);
    m_findNextAction->setShortcut(QKeySequence(Qt::Key_F3));
    addAction(m_findNextAction);

    addSeparator();

    m_clearAction = new QAction(tr("Clear All Logs"), this);
    m_clearAction->setStatusTip(tr("Clear the entire log buffer"));
    addAction(m_clearAction);
}
QAction *TB_CM_LogContextMenu::copyAction() const
{
    return m_copyAction;
}
QAction *TB_CM_LogContextMenu::selectAllAction() const
{
    return m_selectAllAction;
}
QAction *TB_CM_LogContextMenu::findPrevAction() const
{
    return m_findPrevAction;
}
QAction *TB_CM_LogContextMenu::findNextAction() const
{
    return m_findNextAction;
}
QAction *TB_CM_LogContextMenu::clearAction() const
{
    return m_clearAction;
}
void TB_CM_LogContextMenu::popupAt(const QPoint &globalPos, bool hasSelection, bool hasSearch)
{
    m_copyAction->setEnabled(hasSelection);
    m_findPrevAction->setEnabled(hasSearch);
    m_findNextAction->setEnabled(hasSearch);
    exec(globalPos);
}
