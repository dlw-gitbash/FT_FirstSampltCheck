#ifndef FT_FUNCTIONITEM_H
#define FT_FUNCTIONITEM_H

#include <QObject>
#include <QWidget>
#include "FT_Type.h"

class QCheckBox;
class FHintTextEdit;
class FHintSpinBox;
class FT_Function;

class FT_FunctionItem : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionItem(QWidget* parent = nullptr);

    void setFunction(FT_Function* function);
    FT_Function* function() const { return m_function; }

    FT_FunctionItemConfig toConfig() const;
    void applyConfig(const FT_FunctionItemConfig& cfg);

    void adjustHeight();

    QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;

signals:
    void contentChanged();
    void removeRequested();
    void moveUpRequested();
    void moveDownRequested();

private:
    void connectFunction(FT_Function* function);

    QCheckBox*     m_enableCheck  = nullptr;
    FHintTextEdit* m_titleEdit    = nullptr;
    FT_Function*   m_function     = nullptr;
    FHintSpinBox*  m_delaySpin    = nullptr;
    int            m_cachedHeight  = 0;
    bool           m_loading       = false;
    bool           m_adjusting     = false;
    bool           m_pendingAdjust = false;
};

#endif // FT_FUNCTIONITEM_H