#pragma once

#include <TB_CM_Document.h>
#include <TB_CM_Types.h>

#include <QAbstractListModel>
#include <QRegularExpression>

#include <vector>

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

    void appendBatch(std::vector<Record>&& records);
    void clear();
    void setCapacity(int capacity);

    void setFilter(const Filter& filter);
    void invalidateLayoutCache();

    int capacity() const { return document_.capacity(); }
    int totalCount() const { return document_.size(); }
    int visibleCount() const { return int(visible_.size()); }
    const Filter& filter() const { return filter_; }
    bool filterIsValid() const { return regexValid_; }

    const Record* recordAt(int row) const;
    const TB_CM_Document& document() const { return document_; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

signals:
    void countsChanged(int visible, int total);
    void rowsAppended(int firstRow, int lastRow);
    void filterChanged();

private:
    bool matches(const Record& record) const;
    void rebuildVisible();
    void compileFilter();

private:
    TB_CM_Document document_;
    std::vector<qint64> visible_;
    Filter filter_;
    QRegularExpression regex_;
    bool regexValid_ = true;
};
