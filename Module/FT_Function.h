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

    // 通用尺寸:宽度按内部固定控件+可扩展控件估算,
    // 高度取内部控件最小高的最大值(支持多行 payload 撑高)。
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    // 流式布局打包时使用的最小宽度
    int packMinimumWidth() const;

    // 右键命令菜单的可用性状态(由所属 FT_FunctionGroup 在结构变化时刷新)
    struct MenuState
    {
        bool canMoveUp    = false;
        bool canMoveDown  = false;
        bool canWrap      = false; // 可在此命令前插入硬换行
        bool canUnwrap    = false; // 可与上一条硬换行合并
        bool canSplitStep = false; // 可从此命令拆成新 Step(其前还有命令)
    };
    void setMenuState(const MenuState& state) { m_menuState = state; }

signals:
    void contentChanged();
    void requestRemove();
    void requestResize();
    void requestMoveUp();
    void requestMoveDown();
    void requestWrap();
    void requestUnwrap();
    void requestSplitNewStep();
    // 在本命令之后(同一视觉行)插入一条指定类型的新命令
    void requestInsertAfter(const QString& typeName);

protected:
    virtual void openResponseAdvance() = 0;
    void updateFunctionHeight();
    // 在本命令及其子控件上安装右键菜单事件过滤器(子类构造末尾调用一次)
    void installCommandMenuFilters();

    void contextMenuEvent(QContextMenuEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void showCommandMenu(const QPoint& globalPos);

    QHBoxLayout* m_functionLayout = nullptr;
    MenuState    m_menuState;
};

class FT_TboxFunction : public FT_Function
{
    Q_OBJECT
public:
    explicit FT_TboxFunction(QWidget* parent = nullptr);

    FT_FunctionData toConfig() const override;
    void applyConfig(const FT_FunctionData& data) override;

private:
    void openResponseAdvance() override;
    void updateAdvanceLabel();
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

    FT_FunctionData toConfig() const override;
    void applyConfig(const FT_FunctionData& data) override;

private:
    void openResponseAdvance() override;
    void updateAdvanceLabel();
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

    FT_FunctionData toConfig() const override;
    void applyConfig(const FT_FunctionData& data) override;

private:
    void openResponseAdvance() override;
    void updateAdvanceLabel();
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
