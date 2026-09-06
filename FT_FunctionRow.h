#ifndef FT_FUNCTIONROW_H
#define FT_FUNCTIONROW_H

#include <QWidget>
#include "FT_FunctionConfig.h"

class QHBoxLayout;

class FT_FunctionRow : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionRow(QWidget* parent = nullptr);
    ~FT_FunctionRow() override;

    QHBoxLayout* rowLayout() const { return m_rowLayout; }

    virtual QString typeName() const = 0;

    virtual FT_FunctionRowData config() const = 0;
    virtual void applyConfig(const FT_FunctionRowData& data) = 0;

signals:
    void contentChanged();
    void requestRemove();
    void requestResize();

protected:
    QHBoxLayout* m_rowLayout = nullptr;
};

#endif // FT_FUNCTIONROW_H