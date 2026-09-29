#ifndef FT_FUNCTIONITEM_H
#define FT_FUNCTIONITEM_H

#include <QObject>
#include <QWidget>
#include "FT_Type.h"

class QCheckBox;
class FHintTextEdit;
class FT_FunctionGroup;

class FT_FunctionItem : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionItem(QWidget* parent = nullptr);

    FT_FunctionGroup* group() const { return m_group; }

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
    void insertStepAfterRequested();
    void mergeWithNextRequested();
    void splitStepRequested(int flat);

private:
    // 本步骤在外层步骤列表中的行号(-1 = 不在列表中);outCount 返回列表总行数。
    int stepRow(int* outCount);

    QCheckBox*          m_enableCheck  = nullptr;
    FHintTextEdit*      m_titleEdit    = nullptr;
    FT_FunctionGroup*   m_group        = nullptr;
    int                 m_cachedHeight  = 0;
    bool                m_loading       = false;
    bool                m_adjusting     = false;
    bool                m_pendingAdjust = false;
};

#endif // FT_FUNCTIONITEM_H
