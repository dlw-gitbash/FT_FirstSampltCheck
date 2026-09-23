#ifndef FT_FUNCTION_H
#define FT_FUNCTION_H

#include <QWidget>
#include <QHBoxLayout>
#include "FT_Type.h"

class QCheckBox;
class QLineEdit;
class FHintTextEdit;
class FHintSpinBox;
class FHintComboBox;
class QLabel;

class FT_Function : public QWidget
{
    Q_OBJECT
public:
    explicit FT_Function(QWidget* parent = nullptr);
    ~FT_Function() override;

    QHBoxLayout* functionLayout() const { return m_functionLayout; }

    virtual FT_FunctionData toConfig() const = 0;
    virtual void applyConfig(const FT_FunctionData& data) = 0;

signals:
    void contentChanged();
    void requestRemove();
    void requestResize();
    void requestMoveUp();
    void requestMoveDown();

protected:
    QHBoxLayout* m_functionLayout = nullptr;
};

class FT_TboxFunction : public FT_Function
{
    Q_OBJECT
public:
    explicit FT_TboxFunction(QWidget* parent = nullptr);

    QSize sizeHint() const override;

    FT_FunctionData toConfig() const override;
    void applyConfig(const FT_FunctionData& data) override;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void showFunctionContextMenu(const QPoint& globalPos);
    void openResponseAdvance();
    void updateAdvanceLabel();
    void updateFunctionHeight();
    static QString resultModeText(FtResultMode mode);

    FHintComboBox* m_combo   = nullptr;
    FHintTextEdit* m_payload = nullptr;
    QLabel*        m_advance = nullptr;

    int              m_originalComboCount = 0;
    FT_AdvanceConfig m_advanceCfg;
};

class FT_IicWriteFunction : public FT_Function
{
    Q_OBJECT
public:
    explicit FT_IicWriteFunction(QWidget* parent = nullptr);

    QSize sizeHint() const override;

    FT_FunctionData toConfig() const override;
    void applyConfig(const FT_FunctionData& data) override;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void showFunctionContextMenu(const QPoint& globalPos);
    void openResponseAdvance();
    void updateAdvanceLabel();
    void updateFunctionHeight();
    static QString resultModeText(FtResultMode mode);

    FHintSpinBox*  m_port    = nullptr;
    FHintTextEdit* m_reg     = nullptr;
    FHintTextEdit* m_payload = nullptr;
    QLabel*        m_advance = nullptr;

    FT_AdvanceConfig m_advanceCfg;
};

class FT_IicWriteReadFunction : public FT_Function
{
    Q_OBJECT
public:
    explicit FT_IicWriteReadFunction(QWidget* parent = nullptr);

    QSize sizeHint() const override;

    FT_FunctionData toConfig() const override;
    void applyConfig(const FT_FunctionData& data) override;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void showFunctionContextMenu(const QPoint& globalPos);
    void openResponseAdvance();
    void updateAdvanceLabel();
    void updateFunctionHeight();
    static QString resultModeText(FtResultMode mode);

    FHintSpinBox*  m_port       = nullptr;
    FHintTextEdit* m_reg        = nullptr;
    FHintTextEdit* m_payload    = nullptr;
    FHintSpinBox*  m_readLength = nullptr;
    QLabel*        m_advance    = nullptr;

    FT_AdvanceConfig m_advanceCfg;
};

namespace FtFunctionFactory
{
    FT_Function* create(const QString& typeName);
    QString defaultTypeName();
}

#endif // FT_FUNCTION_H