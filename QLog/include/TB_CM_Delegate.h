#pragma once

#include <TB_CM_Types.h>

#include <QAbstractItemDelegate>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QTextLayout>

class TB_CM_Model;

class TB_CM_Delegate final : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit TB_CM_Delegate(QObject* parent = nullptr);

    void setLogModel(const TB_CM_Model* model);
    void setTheme(const Theme& theme);
    void setWrapEnabled(bool enabled);
    void setFilter(const Filter& filter);

    const Theme& theme() const { return theme_; }
    bool isWrapEnabled() const { return wrapEnabled_; }
    void setFallbackWidth(int width) { fallbackWidth_ = width; }
    int fallbackWidth() const { return fallbackWidth_; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void updateMetrics();
    QString buildContent(const Record& record) const;
    int measure(const Record& record, const QString& content, int textWidth) const;
    int lineCount(const QString& text, int textWidth) const;
    int paragraphLines(const QString& paragraph, int textWidth) const;
    QList<QTextLayout::FormatRange> buildFormats(const Record& record, const QString& content) const;
    const QString& timeText(const Record& record) const;

private:
    const TB_CM_Model* model_ = nullptr;
    Theme theme_;
    QFontMetrics metrics_;
    int lineHeight_ = 1;
    int timeWidth_ = 0;
    int levelWidth_ = 0;
    int gutterWidth_ = 0;
    int maxCharWidth_ = 0;
    int fallbackWidth_ = 0;
    bool wrapEnabled_ = true;
    QString matchText_;
    MatchMode matchMode_ = MatchMode::Substring;
    bool matchCaseSensitive_ = false;
    QRegularExpression matchRegex_;
    bool matchRegexValid_ = false;
};
