#pragma once

#include "FT_FunctionWidget.h"
#include "FT_AdvanceDialog.h"
#include <QSize>
#include <QString>

class FT_ComBox;
class FT_LineEdit;
class FT_TextEdit;
class QLabel;

class FT_TBoxFunctionWidget : public FT_FunctionWidget
{
    Q_OBJECT
public:
    explicit FT_TBoxFunctionWidget(QWidget *parent = nullptr);

    QString command() const;
    void setCommand(const QString &cmd);

    QString payload() const;
    void setPayload(const QString &p);

    int delay() const;
    void setDelay(int ms);

    FT_AdvanceDialog::CompareMode compareMode() const;
    QString compareParam() const;
    QString passMessage() const;
    QString failMessage() const;
    FT_AdvanceDialog::FailAction failAction() const;

    QString toConfig() const override;
    void fromConfig(const QString &cfg) override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void showEvent(QShowEvent *e) override;
    void responseAdvance() override;

private slots:
    void adjustPayloadHeight();

private:
    void updateAdvanceSummary();

    FT_ComBox   *m_commandEdit  = nullptr;
    FT_TextEdit *m_payloadEdit  = nullptr;
    QLabel      *m_advanceLabel = nullptr;
    FT_LineEdit *m_delayEdit    = nullptr;

    int m_payloadTextHeight = 56;

    FT_AdvanceDialog::CompareMode m_mode       = FT_AdvanceDialog::CompareMode::DontCare;
    QString                        m_param;
    QString                        m_passMsg;
    QString                        m_failMsg;
    FT_AdvanceDialog::FailAction   m_failAction = FT_AdvanceDialog::FailAction::Stop;
};