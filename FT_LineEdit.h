#pragma once
#include <QLineEdit>
#include <QString>
#include "FT_AbstractHintWidget.h"

class QPaintEvent;
class QResizeEvent;
class QEvent;

class FT_LineEdit : public QLineEdit, public FT_AbstractHintWidget
{
    Q_OBJECT
public:
    explicit FT_LineEdit(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void changeEvent(QEvent *e) override;
    void applyMargins() override;
};