#pragma once

#include <QListWidget>
#include <QList>
#include <QString>
#include <QResizeEvent>

class QListWidgetItem;
class FT_FunctionWidgetItem;

class FT_Edit : public QListWidget
{
    Q_OBJECT
public:
    explicit FT_Edit(QWidget *parent = nullptr);

    FT_FunctionWidgetItem *addSchedule(const QString &title = QString());

    QList<FT_FunctionWidgetItem *> schedules() const;

    QString toConfig() const;
    void fromConfig(const QString &cfg);

signals:
    void scheduleAdded(FT_FunctionWidgetItem *item);
    void scheduleRemoved(FT_FunctionWidgetItem *item);

protected:
    void showContextMenu(const QPoint &pos);
    void showContextMenuAt(const QPoint &globalPos);
    void resizeEvent(QResizeEvent *e) override;

private:
    void attachItem(FT_FunctionWidgetItem *itemWidget, QListWidgetItem *listItem);
};