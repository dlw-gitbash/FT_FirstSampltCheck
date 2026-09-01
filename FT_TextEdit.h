#pragma once
#include <QTextEdit>
#include <QString>
#include "FT_AbstractHintWidget.h"

class QPaintEvent;
class QResizeEvent;
class QEvent;

class FT_TextEdit : public QTextEdit, public FT_AbstractHintWidget
{
    Q_OBJECT
public:
    explicit FT_TextEdit(QWidget *parent = nullptr);
    void setText(const QString &text);
    void setAlignment(Qt::Alignment align);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void changeEvent(QEvent *e) override;
    void applyMargins() override;

private:
    void updateVerticalCenter();
    bool m_verticalCenter = false;
    int  m_inUpdateVC     = 0;
};