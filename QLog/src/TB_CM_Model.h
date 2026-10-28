#pragma once

#include <TB_CM_Document.h>

#include <QAbstractListModel>
#include <QRegularExpression>

#include <vector>

enum class TB_CM_MatchMode : quint8 {
    Substring,
    Regex,
};

struct TB_CM_Filter {
    QString text;
    TB_CM_MatchMode mode = TB_CM_MatchMode::Substring;
    bool caseSensitive = false;
    bool searchCategory = true;
};

class TB_CM_Model final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        LevelRole = Qt::UserRole + 1,
        MessageRole,
        CategoryRole,
        TimestampRole,
    };

    explicit TB_CM_Model(QObject* parent = nullptr);

    void appendBatch(std::vector<TB_CM_LogEntry>&& entries);
    void clear();
    void setFilter(const TB_CM_Filter& filter);
    void invalidateLayoutCache();

    int capacity() const { return document_.capacity(); }
    int totalCount() const { return document_.size(); }
    int visibleCount() const { return int(visible_.size()); }
    const TB_CM_Filter& filter() const { return filter_; }

    const TB_CM_Record* recordAt(int row) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

signals:
    void countsChanged(int visible, int total);
    void rowsAppended(int firstRow, int lastRow);
    void filterChanged();

private:
    bool matches(const TB_CM_Record& record) const;
    void rebuildVisible();
    void compileFilter();

private:
    TB_CM_Document document_;
    std::vector<qint64> visible_;
    TB_CM_Filter filter_;
    QRegularExpression regex_;
    bool regexValid_ = true;
};
