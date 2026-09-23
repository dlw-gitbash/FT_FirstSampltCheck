#ifndef FT_FUNCTIONGROUP_H
#define FT_FUNCTIONGROUP_H

#include <QWidget>
#include <QLayout>
#include <QList>
#include <QSet>
#include <QVector>
#include "FT_Type.h"

class FT_Function;
class QLayoutItem;
class QPaintEvent;
class QContextMenuEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMouseEvent;

// 拖拽 MIME 类型(组内命令 / 左侧树节点)
inline constexpr const char kFtNodeMime[]    = "application/x-ft-node";
inline constexpr const char kFtCommandMime[] = "application/x-ft-command";

// 组内拖放命中结果
struct FtGroupHit
{
    int  flat       = 0;   // 插入到该 flat 序号之前(0..count)
    bool hardBreak  = false; // true=新起一条硬换行;false=行内插入
    bool valid      = false;
};

// 命令拖拽的全局源信息(QDrag 为应用模态,进程内同时只有一个拖拽)
struct FtCommandDragSourceInfo
{
    QWidget* group = nullptr;
    int      flat  = -1;
};
FtCommandDragSourceInfo ftCommandDragSource();
void ftSetCommandDragSource(QWidget* group, int flat);
void ftClearCommandDragSource();

// ============================================================
// FtFlowBreakLayout — 支持“软折行 + 硬换行断点”的流式布局
//   软折行:宽度不足时贪心打包自动换行;
//   硬换行:breakBefore(widget) 强制该命令另起一行。
// ============================================================
class FtFlowBreakLayout : public QLayout
{
    Q_OBJECT
public:
    explicit FtFlowBreakLayout(QWidget* parent);
    ~FtFlowBreakLayout() override;

    // QLayout 必须实现的接口
    void addItem(QLayoutItem* item) override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    int count() const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;
    void setGeometry(const QRect& rect) override;
    Qt::Orientations expandingDirections() const override;

    void insertWidgetItem(int index, QWidget* widget);
    QWidget* widgetAt(int index) const;
    int indexOfWidget(QWidget* widget) const;
    void moveItem(int from, int to);

    void setBreakBefore(QWidget* widget, bool on);
    bool breakBefore(QWidget* widget) const;

    FtGroupHit hitTest(const QPoint& pos) const;
    QRect caretRect(const FtGroupHit& hit) const;

    int contentHeight() const { return m_cachedHeight; }

signals:
    void heightChanged(int height);

private:
    struct Arranged
    {
        QVector<QRect> rects;   // 每个 item 的最终几何
        QList<QVector<int>> rows; // 每行包含的 item 序号
        int height = 0;
    };
    Arranged arrange(const QRect& rect, bool applyGeometry) const;

    QList<QLayoutItem*> m_items;
    QSet<QWidget*>      m_breaks;
    mutable int         m_cachedHeight = 44;
    mutable Arranged    m_last;
};

// ============================================================
// FT_FunctionGroup — 一个 Step 内的命令容器
//   持有扁平的 FT_Function 序列,硬换行信息记录在布局断点中。
// ============================================================
class FT_FunctionGroup : public QWidget
{
    Q_OBJECT
public:
    explicit FT_FunctionGroup(QWidget* parent = nullptr);

    void applyLines(const FT_FunctionLines& lines);
    FT_FunctionLines toLines() const;

    int count() const;
    FT_Function* functionAt(int flat) const;
    int indexOfFunction(FT_Function* function) const;

    FT_Function* insertFunction(const QString& typeName, int flat, bool hardBreak);
    FT_Function* insertConfig(const FT_FunctionData& data, int flat, bool hardBreak);
    void removeFunctionAt(int flat);
    void setBreakBefore(int flat, bool on);
    void moveFlat(int fromFlat, int toFlat, bool hardBreak);
    FT_Function* appendFunction(const QString& typeName);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    FtGroupHit hitTest(const QPoint& pos) const;
    void showDropCaret(const FtGroupHit& hit);
    void hideDropCaret();

signals:
    void contentChanged();
    void structureChanged();
    void requestResize();
    void splitStepRequested(int flat);
    void nodeDroppedAt(int nodeType, int flat, bool hardBreak);
    void commandDroppedAt(const QByteArray& payload, int flat, bool hardBreak);
    // 组菜单触发的 Step 级操作(由所属 FT_FunctionItem 转发给 FT_Edit)
    void requestInsertStepAfter();
    void requestMergeWithNext();

protected:
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void handleLayoutHeight(int height);

private:
    FT_Function* createWidget(const QString& typeName);
    void wireFunction(FT_Function* function);
    void installDragFilter(FT_Function* function);
    void refreshMenuStates();
    void relayout();
    void showGroupMenu(const QPoint& globalPos, const QPoint& localPos);
    void beginCommandDrag(FT_Function* function, const QPoint& pressPos);

    FtFlowBreakLayout* m_layout = nullptr;

    FtGroupHit m_caret;
    bool       m_arranging          = false;
    bool       m_relayoutRequested  = false;

    FT_Function* m_pressFunction = nullptr;
    QPoint       m_pressPos;
};

#endif // FT_FUNCTIONGROUP_H
