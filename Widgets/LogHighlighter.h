#pragma once

#include "LogEntry.h"
#include <QStyledItemDelegate>
#include <QHash>

// =====================================================
// LogHighlighter：自定义渲染委托
// 特点：
//   1. 预计算颜色缓存，避免 paint 时重复查询
//   2. 级别列特殊渲染（带底色圆角标签）
//   3. 优化绘制路径，减少开销
// =====================================================
class LogHighlighter : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit LogHighlighter(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};