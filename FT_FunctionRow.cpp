#include "FT_FunctionRow.h"

#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QStyle>

FT_FunctionRow::FT_FunctionRow(QWidget* parent)
    : QWidget(parent)
{
    m_rowLayout = new QHBoxLayout(this);
    m_rowLayout->setContentsMargins(0, 0, 0, 0);
    m_rowLayout->setSpacing(6);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* remove = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove this row"));
        if (menu.exec(mapToGlobal(pos)) == remove)
            emit requestRemove();
    });
}

FT_FunctionRow::~FT_FunctionRow() = default;