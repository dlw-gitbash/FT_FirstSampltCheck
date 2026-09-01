#include "FT_FunctionWidget.h"

#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

FT_FunctionWidget::FT_FunctionWidget(const QString &typeName, QWidget *parent)
    : QWidget(parent)
    , m_typeName(typeName)
{
}

FT_FunctionWidget::~FT_FunctionWidget() = default;

QString FT_FunctionWidget::functionType() const
{
    return m_typeName;
}

void FT_FunctionWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu(this);
    QAction *adv = menu.addAction(tr("Response Advance"));
    menu.addSeparator();
    QAction *add = menu.addAction(tr("Add TBox Function"));
    QAction *chosen = menu.exec(e->globalPos());
    if (chosen == adv)
        responseAdvance();
    else if (chosen == add)
        emit addFunctionRequested();
}

void FT_FunctionWidget::responseAdvance()
{
}