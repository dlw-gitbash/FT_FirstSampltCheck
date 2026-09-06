#ifndef FT_ADVANCEDIALOG_H
#define FT_ADVANCEDIALOG_H

#include <QDialog>
#include "FT_FunctionConfig.h"

class QComboBox;
class QTextEdit;
class QLineEdit;
class QStackedWidget;

class FT_AdvanceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FT_AdvanceDialog(QWidget* parent = nullptr);

    void             setConfig(const FT_AdvanceConfig& cfg);
    FT_AdvanceConfig config() const;

private slots:
    void onModeChanged(int index);
    void onAccepted();

private:
    void buildUi();
    static QString normalizeHex(const QString& raw);

    QComboBox*      m_toResult  = nullptr;
    QWidget*        m_dependentGroup = nullptr;
    QStackedWidget* m_targetStack = nullptr;
    QTextEdit*      m_targetSingle = nullptr;
    QLineEdit*      m_targetMin    = nullptr;
    QLineEdit*      m_targetMax    = nullptr;
    QTextEdit*      m_passMessage = nullptr;
    QTextEdit*      m_failMessage = nullptr;
    QComboBox*      m_failTo     = nullptr;
};

#endif // FT_ADVANCEDIALOG_H