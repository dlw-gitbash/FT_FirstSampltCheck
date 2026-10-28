#include <TB_CM_Delegate.h>

#include <QDateTime>
#include <QPainter>
#include <QTextLayout>
#include <QTextOption>

#include <algorithm>
#include <vector>

namespace {

constexpr int AccentWidth = 3;
constexpr int LeftPad = 8;
constexpr int ColumnGap = 10;
constexpr int RightPad = 12;
constexpr int VerticalPad = 3;
constexpr int MaxLines = 512;

QList<QTextLayout::FormatRange> sliceFormats(const QList<QTextLayout::FormatRange>& formats,
                                             int start, int length)
{
    QList<QTextLayout::FormatRange> sliced;
    const int end = start + length;
    for (const QTextLayout::FormatRange& range : formats) {
        const int rangeEnd = range.start + range.length;
        const int from = std::max(range.start, start);
        const int to = std::min(rangeEnd, end);
        if (to <= from) {
            continue;
        }
        QTextLayout::FormatRange item = range;
        item.start = from - start;
        item.length = to - from;
        sliced.append(item);
    }
    return sliced;
}

} // namespace

TB_CM_Delegate::TB_CM_Delegate(QObject* parent)
    : QAbstractItemDelegate(parent)
    , theme_(TB_CM_Theme::dark())
    , metrics_(theme_.font)
{
    updateMetrics();
}
void TB_CM_Delegate::setLogModel(const TB_CM_Model* model)
{
    model_ = model;
}
void TB_CM_Delegate::setTheme(const TB_CM_Theme& theme)
{
    theme_ = theme;
    updateMetrics();
}
void TB_CM_Delegate::setWrapEnabled(bool enabled)
{
    wrapEnabled_ = enabled;
}
void TB_CM_Delegate::setFilter(const TB_CM_Filter& filter)
{
    matchText_ = filter.text;
    matchMode_ = filter.mode;
    matchCaseSensitive_ = filter.caseSensitive;
    matchRegexValid_ = false;
    if (matchMode_ == TB_CM_MatchMode::Regex && !matchText_.isEmpty()) {
        matchRegex_.setPatternOptions(matchCaseSensitive_
                                          ? QRegularExpression::NoPatternOption
                                          : QRegularExpression::CaseInsensitiveOption);
        matchRegex_.setPattern(matchText_);
        matchRegexValid_ = matchRegex_.isValid();
    }
}
void TB_CM_Delegate::setFallbackWidth(int width)
{
    fallbackWidth_ = width;
}
void TB_CM_Delegate::updateMetrics()
{
    metrics_ = QFontMetrics(theme_.font);
    lineHeight_ = std::max(1, metrics_.lineSpacing());
    timeWidth_ = metrics_.horizontalAdvance(QStringLiteral("00:00:00.000"));
    levelWidth_ = metrics_.horizontalAdvance(QStringLiteral("ERROR"));
    gutterWidth_ = AccentWidth + LeftPad + timeWidth_ + ColumnGap + levelWidth_ + ColumnGap;
    maxCharWidth_ = std::max(metrics_.horizontalAdvance(u'W'), metrics_.horizontalAdvance(u'M'));
}
QString TB_CM_Delegate::buildContent(const TB_CM_Record& record) const
{
    if (record.category.isEmpty()) {
        return record.message;
    }
    QString content;
    content.reserve(record.category.size() + record.message.size() + 2);
    content += record.category;
    content += QLatin1String("  ");
    content += record.message;
    return content;
}
const QString& TB_CM_Delegate::timeText(const TB_CM_Record& record) const
{
    if (record.timeText.isEmpty()) {
        record.timeText = QDateTime::fromMSecsSinceEpoch(qint64(record.timestampMs))
                              .toString(QStringLiteral("HH:mm:ss.zzz"));
    }
    return record.timeText;
}
int TB_CM_Delegate::measure(const TB_CM_Record& record, int textWidth) const
{
    if (textWidth <= 0) {
        return lineHeight_ + 2 * VerticalPad;
    }
    if (record.cachedWidth == textWidth && record.cachedHeight > 0) {
        return record.cachedHeight;
    }
    const int height = lineCount(buildContent(record), textWidth) * lineHeight_ + 2 * VerticalPad;
    record.cachedWidth = textWidth;
    record.cachedHeight = height;
    return height;
}
int TB_CM_Delegate::paragraphLines(const QString& paragraph, int textWidth) const
{
    if (!wrapEnabled_ || paragraph.isEmpty() || textWidth <= 0) {
        return 1;
    }
    QTextLayout layout(paragraph, theme_.font);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);
    layout.beginLayout();
    int lines = 0;
    while (lines < MaxLines) {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(qreal(textWidth));
        ++lines;
    }
    layout.endLayout();
    return std::max(1, lines);
}
int TB_CM_Delegate::lineCount(const QString& text, int textWidth) const
{
    if (text.isEmpty()) {
        return 1;
    }
    const bool hasNewline = text.contains(u'\n');
    if (!wrapEnabled_) {
        if (!hasNewline) {
            return 1;
        }
        int lines = 1;
        for (qsizetype pos = text.indexOf(u'\n'); pos >= 0; pos = text.indexOf(u'\n', pos + 1)) {
            ++lines;
        }
        return lines;
    }
    if (!hasNewline) {
        bool wideChars = false;
        for (const QChar ch : text) {
            if (ch.unicode() > 0x7F) {
                wideChars = true;
                break;
            }
        }
        if (!wideChars && text.size() * maxCharWidth_ <= textWidth) {
            return 1;
        }
        return paragraphLines(text, textWidth);
    }
    int lines = 0;
    qsizetype start = 0;
    while (lines < MaxLines) {
        const qsizetype newline = text.indexOf(u'\n', start);
        const qsizetype end = newline < 0 ? text.size() : newline;
        lines += paragraphLines(text.mid(start, end - start), textWidth);
        if (newline < 0) {
            break;
        }
        start = end + 1;
    }
    return std::max(1, std::min(lines, MaxLines));
}
QList<QTextLayout::FormatRange> TB_CM_Delegate::buildFormats(const TB_CM_Record& record,
                                                            const QString& content) const
{
    const int length = int(content.size());
    if (length <= 0) {
        return {};
    }
    std::vector<quint8> styles(size_t(length), 0);
    const int categoryLength = record.category.isEmpty()
        ? 0
        : std::min(length, int(record.category.size()));
    std::fill_n(styles.begin(), categoryLength, quint8(1));
    if (!matchText_.isEmpty()) {
        const auto mark = [&styles, length](int start, int count) {
            const int end = std::min(length, start + count);
            for (int i = std::max(0, start); i < end; ++i) {
                styles[size_t(i)] = 2;
            }
        };
        if (matchMode_ == TB_CM_MatchMode::Regex && matchRegexValid_) {
            QRegularExpressionMatchIterator it = matchRegex_.globalMatch(content);
            int guard = 0;
            while (it.hasNext() && guard++ < MaxLines) {
                const QRegularExpressionMatch match = it.next();
                const int captured = int(match.capturedLength());
                if (captured <= 0) {
                    break;
                }
                mark(int(match.capturedStart()), captured);
            }
        } else {
            const Qt::CaseSensitivity cs = matchCaseSensitive_ ? Qt::CaseSensitive
                                                               : Qt::CaseInsensitive;
            qsizetype from = 0;
            while (from < content.size()) {
                const qsizetype pos = content.indexOf(matchText_, from, cs);
                if (pos < 0) {
                    break;
                }
                mark(int(pos), int(matchText_.size()));
                from = pos + std::max<qsizetype>(1, matchText_.size());
            }
        }
    }
    QList<QTextLayout::FormatRange> formats;
    int i = 0;
    while (i < length) {
        const quint8 style = styles[size_t(i)];
        int j = i + 1;
        while (j < length && styles[size_t(j)] == style) {
            ++j;
        }
        if (style != 0) {
            QTextLayout::FormatRange range;
            range.start = i;
            range.length = j - i;
            if (style == 1) {
                range.format.setForeground(theme_.category);
            } else {
                range.format.setBackground(theme_.matchBackground);
                range.format.setForeground(theme_.matchText);
            }
            formats.append(range);
        }
        i = j;
    }
    return formats;
}
QSize TB_CM_Delegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int minimum = lineHeight_ + 2 * VerticalPad;
    int width = option.rect.width();
    if (width <= 0) {
        width = fallbackWidth_;
    }
    if (model_ == nullptr || width <= 0) {
        return QSize(width, minimum);
    }
    const TB_CM_Record* record = model_->recordAt(index.row());
    if (record == nullptr) {
        return QSize(width, minimum);
    }
    const int textWidth = std::max(1, width - gutterWidth_ - RightPad);
    return QSize(width, measure(*record, textWidth));
}
void TB_CM_Delegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                           const QModelIndex& index) const
{
    if (model_ == nullptr) {
        return;
    }
    const TB_CM_Record* record = model_->recordAt(index.row());
    if (record == nullptr) {
        return;
    }
    const QRect rect = option.rect;
    if (!rect.isValid()) {
        return;
    }
    painter->save();
    painter->setClipRect(rect);
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    painter->fillRect(rect, (record->sequence & 1) ? theme_.alternateBackground
                                                    : theme_.background);
    if (selected) {
        painter->fillRect(rect, theme_.selection);
    } else if (hovered) {
        painter->fillRect(rect, theme_.hover);
    }
    const int level = std::clamp(record->level, int(L_TRACE), int(L_FATAL));
    const QColor levelColor = theme_.levelColors[size_t(level)];
    painter->fillRect(QRect(rect.left(), rect.top(), AccentWidth, rect.height()), levelColor);
    painter->setFont(theme_.font);
    const int firstLineTop = rect.top() + VerticalPad;
    const int timeLeft = rect.left() + AccentWidth + LeftPad;
    painter->setPen(theme_.textMuted);
    painter->drawText(QRect(timeLeft, firstLineTop, timeWidth_, lineHeight_),
                      Qt::AlignLeft | Qt::AlignVCenter, timeText(*record));
    QFont levelFont = theme_.font;
    levelFont.setWeight(QFont::DemiBold);
    painter->setFont(levelFont);
    painter->setPen(levelColor);
    painter->drawText(QRect(timeLeft + timeWidth_ + ColumnGap, firstLineTop, levelWidth_, lineHeight_),
                      Qt::AlignLeft | Qt::AlignVCenter, levelName(level).toString());
    painter->setFont(theme_.font);
    const QString content = buildContent(*record);
    const int textWidth = std::max(1, rect.width() - gutterWidth_ - RightPad);
    const int heightBudget = std::max(1, rect.height() - 2 * VerticalPad);
    QTextOption textOption;
    textOption.setWrapMode(wrapEnabled_ ? QTextOption::WrapAtWordBoundaryOrAnywhere
                                        : QTextOption::NoWrap);
    const QList<QTextLayout::FormatRange> formats = buildFormats(*record, content);
    painter->setPen(theme_.text);
    const QPointF origin(rect.left() + gutterWidth_, rect.top() + VerticalPad);
    // QTextLayout 是单段落布局器，'\n' 不会换行，必须逐段落布局
    int drawnLines = 0;
    qsizetype start = 0;
    while (drawnLines < MaxLines) {
        const qsizetype newline = content.indexOf(u'\n', start);
        const qsizetype end = newline < 0 ? content.size() : newline;
        const QString paragraph = content.mid(start, end - start);
        QTextLayout layout(paragraph, theme_.font);
        layout.setTextOption(textOption);
        if (!formats.isEmpty()) {
            layout.setFormats(sliceFormats(formats, int(start), int(end - start)));
        }
        layout.beginLayout();
        qreal y = 0;
        int paragraphLineCount = 0;
        while (paragraphLineCount < MaxLines) {
            QTextLine line = layout.createLine();
            if (!line.isValid()) {
                break;
            }
            line.setLineWidth(qreal(textWidth));
            line.setPosition(QPointF(0, y));
            y += lineHeight_;
            ++paragraphLineCount;
        }
        layout.endLayout();
        const int paragraphHeight = std::max(1, paragraphLineCount);
        if (qreal(heightBudget) - qreal(drawnLines) * lineHeight_ <= 0) {
            break;
        }
        // 不能用 draw() 的 clip 参数：那是布局坐标，会与行矩形的 painter clip 相交导致只画出首行
        layout.draw(painter, origin + QPointF(0, qreal(drawnLines) * lineHeight_));
        drawnLines += paragraphHeight;
        if (newline < 0) {
            break;
        }
        start = end + 1;
    }
    painter->restore();
}
