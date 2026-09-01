#pragma once

#include <QComboBox>
#include <QString>
#include <QStringList>
#include "FT_AbstractHintWidget.h"

class QResizeEvent;
class QPaintEvent;
class QEvent;
class QPainter;
class QRect;
class QShowEvent;

class FT_ComBox : public QComboBox, public FT_AbstractHintWidget
{
    Q_OBJECT
public:
    explicit FT_ComBox(QWidget *parent = nullptr);

    void addItem(const QString &text);
    void addItems(const QStringList &texts);

    void setPlaceholderText(const QString &text);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void changeEvent(QEvent *e) override;
    void showPopup() override;

    void applyMargins() override;
    void ftPaintExtra(QPainter &p, const QRect &area) override;
};