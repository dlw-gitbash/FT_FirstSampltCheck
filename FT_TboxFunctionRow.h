#ifndef FT_TBOXFUNCTIONROW_H
#define FT_TBOXFUNCTIONROW_H

#include "FT_FunctionRow.h"
#include "FT_FunctionConfig.h"

class FHintComboBox;
class FHintTextEdit;
class FHintSpinBox;
class QLabel;
class QContextMenuEvent;

class FT_TboxFunctionRow : public FT_FunctionRow
{
    Q_OBJECT
public:
    explicit FT_TboxFunctionRow(QWidget* parent = nullptr);

    QSize sizeHint() const override;

    QString typeName() const override;
    FT_FunctionRowData config() const override;
    void applyConfig(const FT_FunctionRowData& data) override;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void showRowContextMenu(const QPoint& globalPos);
    void openResponseAdvance();
    void updateAdvanceLabel();
    void updateRowHeight();
    static QString resultModeText(FtResultMode mode);

    FHintComboBox* m_combo   = nullptr;
    FHintTextEdit* m_payload = nullptr;
    QLabel*        m_advance = nullptr;
    FHintSpinBox*  m_delay   = nullptr;

    FT_AdvanceConfig m_advanceCfg;
};

#endif // FT_TBOXFUNCTIONROW_H