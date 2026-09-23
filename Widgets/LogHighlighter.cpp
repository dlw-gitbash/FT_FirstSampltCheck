#include "LogHighlighter.h"
#include "LogModel.h"
#include <QPainter>
#include <QApplication>

LogHighlighter::LogHighlighter(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void LogHighlighter::paint(QPainter* painter,
                           const QStyleOptionViewItem& option,
                           const QModelIndex& index) const
{
    painter->save();

    // 绘制选中/悬停背景
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 自定义绘制背景色
    QColor bgColor = index.data(Qt::BackgroundRole).value<QColor>();
    if (bgColor.isValid() && !(opt.state & QStyle::State_Selected)) {
        painter->fillRect(opt.rect, bgColor);
    } else if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, opt.palette.highlight());
    } else if (opt.state & QStyle::State_MouseOver) {
        painter->fillRect(opt.rect, QColor("#E8F0FE"));
    } else {
        painter->fillRect(opt.rect, opt.palette.base());
    }

    // Level 列特殊渲染
    if (index.column() == LogModel::ColLevel) {
        QString text = index.data(Qt::DisplayRole).toString();
        int levelValue = index.data(Qt::UserRole).toInt();
        auto level = static_cast<LogLevel>(levelValue);
        QColor fg = LogLevelHelper::levelColor(level);
        QColor levelBg = LogLevelHelper::levelBgColor(level);

        // 绘制圆角标签
        QRect tagRect = opt.rect.adjusted(4, 2, -4, -2);
        int radius = 4;

        painter->setRenderHint(QPainter::Antialiasing);

        if (opt.state & QStyle::State_Selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(fg);
            painter->drawRoundedRect(tagRect, radius, radius);
            painter->setPen(Qt::white);
        } else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(levelBg.darker(110));
            painter->drawRoundedRect(tagRect, radius, radius);

            QFont font = opt.font;
            font.setBold(true);
            int ps = font.pixelSize();
            if (ps > 0) {
                font.setPixelSize(ps - 1);
            }
            painter->setFont(font);
            painter->setPen(fg);
        }

        painter->drawText(tagRect, Qt::AlignCenter, text);
    } else {
        // 普通列渲染
        QColor fg = index.data(Qt::ForegroundRole).value<QColor>();
        if (!fg.isValid() || (opt.state & QStyle::State_Selected)) {
            fg = opt.palette.highlightedText().color();
        }

        painter->setPen(fg);
        QFont font = opt.font;
        if (index.column() == LogModel::ColMessage) {
            // 消息列使用等宽字体
            font.setFamily("Consolas");
        }
        painter->setFont(font);

        QString text = index.data(Qt::DisplayRole).toString();
        QTextOption textOpt;
        textOpt.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        textOpt.setWrapMode(QTextOption::NoWrap);

        QRect textRect = opt.rect.adjusted(6, 0, -6, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                          opt.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width()));
    }

    // 绘制行间分隔线（微妙的分隔线提升可读性）
    painter->setPen(QColor(0, 0, 0, 20));
    painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());

    painter->restore();
}

QSize LogHighlighter::sizeHint(const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), 18));
    return size;
}