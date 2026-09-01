#pragma once

#include <QWidget>
#include <QString>

class QContextMenuEvent;

class FT_FunctionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionWidget(const QString &typeName, QWidget *parent = nullptr);
    ~FT_FunctionWidget() override;

    QString functionType() const;

    virtual QString toConfig() const = 0;
    virtual void fromConfig(const QString &cfg) = 0;

signals:
    void sizeHintChanged();
    void addFunctionRequested();

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

    virtual void responseAdvance();

    QString m_typeName;
};