#pragma once

#include <TB_CM_Model.h>
#include <TB_CM_Theme.h>

#include <QAbstractItemDelegate>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QTextLayout>

class TB_CM_Delegate final : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit TB_CM_Delegate(QObject* parent = nullptr);

    void setLogModel(const TB_CM_Model* model);
    void setTheme(const TB_CM_Theme& theme);
    void setWrapEnabled(bool enabled);
    void setFilter(const TB_CM_Filter& filter);
    void setFallbackWidth(int width);

    int fallbackWidth() const { return fallbackWidth_; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    void updateMetrics();
    QString buildContent(const TB_CM_Record& record) const;
    int measure(const TB_CM_Record& record, int textWidth) const;
    int lineCount(const QString& text, int textWidth) const;
    int paragraphLines(const QString& paragraph, int textWidth) const;
    QList<QTextLayout::FormatRange> buildFormats(const TB_CM_Record& record,
                                                 const QString& content) const;
    const QString& timeText(const TB_CM_Record& record) const;

private:
    const TB_CM_Model* model_ = nullptr;
    TB_CM_Theme theme_;
    QFontMetrics metrics_;
    int lineHeight_ = 1;
    int timeWidth_ = 0;
    int levelWidth_ = 0;
    int gutterWidth_ = 0;
    int maxCharWidth_ = 0;
    int fallbackWidth_ = 0;
    bool wrapEnabled_ = true;
    QString matchText_;
    TB_CM_MatchMode matchMode_ = TB_CM_MatchMode::Substring;
    bool matchCaseSensitive_ = false;
    QRegularExpression matchRegex_;
    bool matchRegexValid_ = false;
};
