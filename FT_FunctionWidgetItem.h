#pragma once

#include <QWidget>
#include <QList>
#include <QSize>
#include <QPoint>
#include <QString>
#include <QTimer>

class QCheckBox;
class FT_TextEdit;
class QVBoxLayout;
class QWidget;
class FT_FunctionWidget;

class FT_FunctionWidgetItem : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionWidgetItem(QWidget *parent = nullptr);

    bool scheduleEnabled() const;
    void setScheduleEnabled(bool on);

    QString title() const;
    void setTitle(const QString &t);

    void addFunction(FT_FunctionWidget *fw);
    void clearFunctions();

    QList<FT_FunctionWidget *> functions() const;
    FT_FunctionWidget *functionAt(int row) const;
    int functionCount() const;

    static FT_FunctionWidget *createFunction(const QString &type);

    QString toConfig() const;
    void fromConfig(const QString &cfg);

signals:
    void enabledChanged(bool on);
    void titleChanged(const QString &t);
    void functionListChanged();
    void contextMenuRequested(const QPoint &globalPos);
    void sizeHintChanged();

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

private slots:
    void showFunclistMenu(const QPoint &pos);

private:
    void hookFunctionSize(FT_FunctionWidget *fw);

    QCheckBox   *m_enableCheck   = nullptr;
    FT_TextEdit *m_titleEdit     = nullptr;

    QWidget     *m_funcContainer = nullptr;
    QVBoxLayout *m_funcLayout    = nullptr;

    QList<FT_FunctionWidget *> m_functions;
};