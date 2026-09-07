#include "FT_FunctionRow.h"

#include <QHBoxLayout>

FT_FunctionRow::FT_FunctionRow(QWidget* parent)
    : QWidget(parent)
{
    m_rowLayout = new QHBoxLayout(this);
    m_rowLayout->setContentsMargins(0, 0, 0, 0);
    m_rowLayout->setSpacing(6);
}

FT_FunctionRow::~FT_FunctionRow() = default;