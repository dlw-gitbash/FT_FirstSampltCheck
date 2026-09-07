#include "FT_FunctionCard.h"
#include "FT_FunctionRow.h"
#include "FT_FunctionFactory.h"

#include <QCheckBox>
#include <QTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QTimer>
#include <QResizeEvent>

namespace {
constexpr int kCardMargin    = 2;
constexpr int kHeaderSpacing = 6;
constexpr int kTitleMinH     = 36;
constexpr int kCardDefaultH  = 48;
constexpr int kListItemPadV  = 2;
}

FT_FunctionCard::FT_FunctionCard(QWidget* parent)
    : QWidget(parent)
{
    m_enableCheck = new QCheckBox(this);
    m_enableCheck->setFixedWidth(28);
    m_enableCheck->setChecked(true);
    m_enableCheck->setToolTip(tr("Enable/disable this Function"));
    m_enableCheck->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_titleEdit = new FHintTextEdit(tr("Title"), this);
    m_titleEdit->setPlaceholderText(tr("Function Title"));
    m_titleEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_titleEdit->setPlainText(tr("New Item"));
    m_titleEdit->setContextMenuPolicy(Qt::NoContextMenu);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(kCardMargin, kCardMargin, kCardMargin, kCardMargin);
    root->setSpacing(kHeaderSpacing);
    root->setAlignment(Qt::AlignTop);
    root->addWidget(m_enableCheck, 0);
    root->addWidget(m_titleEdit, 1);

    connect(m_titleEdit, &QTextEdit::textChanged, this, [this]() {
        adjustHeight();
        emit contentChanged();
    });
    connect(m_titleEdit, &FHintTextEdit::heightChanged, this, [this](int) {
        adjustHeight();
    });

    connect(m_enableCheck, &QCheckBox::toggled, this, &FT_FunctionCard::contentChanged);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        QAction* del = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            tr("Remove this Function"));

        QAction* chosen = menu.exec(mapToGlobal(pos));
        if (chosen == del)
            emit removeRequested();
    });

    QTimer::singleShot(0, this, &FT_FunctionCard::adjustHeight);
}

void FT_FunctionCard::connectRow(FT_FunctionRow* row)
{
    connect(row, &FT_FunctionRow::contentChanged,
            this, &FT_FunctionCard::contentChanged);
    connect(row, &FT_FunctionRow::requestRemove,
            this, &FT_FunctionCard::removeRequested);
    connect(row, &FT_FunctionRow::requestResize, this, [this]() {
        if (!m_loading)
            adjustHeight();
    });
}

void FT_FunctionCard::setRow(FT_FunctionRow* row)
{
    if (m_row == row)
        return;

    if (m_row) {
        layout()->removeWidget(m_row);
        m_row->deleteLater();
        m_row = nullptr;
    }

    m_row = row;
    if (!m_row) {
        if (!m_loading)
            adjustHeight();
        return;
    }

    if (auto* hbox = qobject_cast<QHBoxLayout*>(layout()))
        hbox->addWidget(m_row, 3, Qt::AlignTop);
    connectRow(m_row);

    if (!m_loading) {
        adjustHeight();
        emit contentChanged();
    }
}

FT_FunctionCardConfig FT_FunctionCard::toConfig() const
{
    FT_FunctionCardConfig cfg;
    cfg.enabled = m_enableCheck->isChecked();
    QString title = m_titleEdit->toPlainText().trimmed();
    title.replace(QLatin1Char('\r'), QLatin1Char(' '));
    title.replace(QLatin1Char('\n'), QLatin1Char(' '));
    cfg.title = title.simplified();
    if (m_row)
        cfg.rows.push_back(m_row->config());
    return cfg;
}

void FT_FunctionCard::applyConfig(const FT_FunctionCardConfig& cfg)
{
    m_loading = true;

    m_enableCheck->setChecked(cfg.enabled);
    m_titleEdit->setPlainText(cfg.title);

    if (m_row) {
        layout()->removeWidget(m_row);
        m_row->deleteLater();
        m_row = nullptr;
    }

    if (!cfg.rows.isEmpty()) {
        FT_FunctionRow* row = FtFunctionFactory::create(ftRowDataTypeName(cfg.rows.first()));
        if (row) {
            m_row = row;
            if (auto* hbox = qobject_cast<QHBoxLayout*>(layout()))
                hbox->addWidget(m_row, 3, Qt::AlignTop);
            connectRow(m_row);
            m_row->applyConfig(cfg.rows.first());
        }
    }

    m_loading = false;
    emit contentChanged();

    QTimer::singleShot(0, this, &FT_FunctionCard::adjustHeight);
}

void FT_FunctionCard::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_loading && !m_adjusting)
        adjustHeight();
}

void FT_FunctionCard::adjustHeight()
{
    if (m_adjusting)
        return;
    m_adjusting = true;

    int rowMinH = 0;
    if (m_row) {
        rowMinH = m_row->minimumHeight();
        if (rowMinH <= 0) {
            const int sh = m_row->sizeHint().height();
            rowMinH = sh > 0 ? sh : m_row->height();
        }
    }
    const int titleMinH = m_titleEdit ? m_titleEdit->minimumHeight() : kTitleMinH;
    const int contentH = qMax(titleMinH, rowMinH);
    const int cardH = contentH + kCardMargin * 2;

    m_cachedHeight = cardH;
    setFixedHeight(cardH);
    updateGeometry();

    m_adjusting = false;

    QWidget* vp = parentWidget();
    QListWidget* outer = vp ? qobject_cast<QListWidget*>(vp->parentWidget()) : nullptr;
    if (!outer)
        return;

    for (int i = 0; i < outer->count(); ++i) {
        QListWidgetItem* outerItem = outer->item(i);
        if (outer->itemWidget(outerItem) == this) {
            outerItem->setSizeHint(QSize(0, cardH + kListItemPadV));
            break;
        }
    }
}

QSize FT_FunctionCard::sizeHint() const
{
    return QSize(0, m_cachedHeight > 0 ? m_cachedHeight : kCardDefaultH);
}

QSize FT_FunctionCard::minimumSizeHint() const
{
    return sizeHint();
}