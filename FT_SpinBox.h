#pragma once
#include <QSpinBox>
#include <QString>
#include "FT_AbstractHintWidget.h"

class QPaintEvent;
class QResizeEvent;
class QEvent;

class FT_SpinBox : public QSpinBox, public FT_AbstractHintWidget
{
    Q_OBJECT
public:
    explicit FT_SpinBox(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void changeEvent(QEvent *e) override;
    void applyMargins() override;

private:
    static constexpr int kBtnWidth = 20;
};