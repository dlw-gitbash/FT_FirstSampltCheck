#ifndef FT_FUNCTIONTREE_H
#define FT_FUNCTIONTREE_H

#include <QTreeWidget>

class FT_FunctionTree : public QTreeWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionTree(QWidget* parent = nullptr);

protected:
    QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
};

#endif // FT_FUNCTIONTREE_H