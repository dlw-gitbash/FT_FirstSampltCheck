#ifndef FT_FUNCTIONCARD_H
#define FT_FUNCTIONCARD_H

#include <QWidget>
#include "FT_FunctionConfig.h"
#include "FHintWidgets.h"

class QCheckBox;
class FT_FunctionRow;

class FT_FunctionCard : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionCard(QWidget* parent = nullptr);

    void setRow(FT_FunctionRow* row);

    FT_FunctionCardConfig toConfig() const;
    void applyConfig(const FT_FunctionCardConfig& cfg);

    void adjustHeight();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;

signals:
    void contentChanged();
    void removeRequested();

private:
    void connectRow(FT_FunctionRow* row);

    QCheckBox*      m_enableCheck  = nullptr;
    FHintTextEdit*  m_titleEdit    = nullptr;
    FT_FunctionRow* m_row          = nullptr;
    int             m_cachedHeight = 0;
    bool            m_loading      = false;
    bool            m_adjusting    = false;
};

#endif // FT_FUNCTIONCARD_H