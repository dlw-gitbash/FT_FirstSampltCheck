#pragma once

#include <QDialog>
#include <QString>

#include "FT_Core.h"

class QComboBox;
class QTextEdit;
class QLabel;
class QDialogButtonBox;

class FT_AdvanceDialog : public QDialog
{
    Q_OBJECT
public:
    using CompareMode = FT_Core::CompareMode;
    using FailAction  = FT_Core::FailAction;

    explicit FT_AdvanceDialog(QWidget *parent = nullptr);

    static QString compareModeToString(CompareMode m)
    { return FT_Core::compareModeToString(m); }
    static bool compareModeFromString(const QString &s, CompareMode *out)
    { return FT_Core::compareModeFromString(s, out); }
    static QString failActionToString(FailAction a)
    { return FT_Core::failActionToString(a); }
    static bool failActionFromString(const QString &s, FailAction *out)
    { return FT_Core::failActionFromString(s, out); }

    CompareMode compareMode() const;
    void setCompareMode(CompareMode m);

    QString compareParam() const;
    void setCompareParam(const QString &hexText);

    QString passMessage() const;
    void setPassMessage(const QString &s);

    QString failMessage() const;
    void setFailMessage(const QString &s);

    FailAction failAction() const;
    void setFailAction(FailAction a);

    static bool isValidHexParam(const QString &text)
    { return FT_Core::isValidHexParam(text); }

    QString toConfig() const;
    void fromConfig(const QString &cfg);

protected:
    void onOkClicked();

private:
    QComboBox        *m_modeCombo = nullptr;
    QTextEdit        *m_paramEdit = nullptr;
    QTextEdit        *m_passEdit  = nullptr;
    QTextEdit        *m_failEdit  = nullptr;
    QComboBox        *m_failCombo = nullptr;
    QDialogButtonBox *m_buttons   = nullptr;
};