#ifndef FT_EDIT_H
#define FT_EDIT_H

#include <QWidget>
#include <QString>

class QTreeWidgetItem;
class QLabel;
class QStatusBar;
class QCloseEvent;
class FT_FunctionItem;
class FT_FunctionGroup;
class FT_FunctionTree;
class FT_FunctionList;
class FT_Function;
class FT_Project;
struct FT_FunctionItemConfig;
class QByteArray;

class FT_Edit : public QWidget
{
    Q_OBJECT
public:
    explicit FT_Edit(QWidget* parent = nullptr);

    FT_FunctionItem* addItem(int atIndex = -1);
    void addTboxItem(int atIndex = -1);
    void addIicWriteItem(int atIndex = -1);
    void addIicWriteReadItem(int atIndex = -1);
    // 把一条命令追加到当前选中 Step 的命令组末尾;
    // 没有任何 Step 时退化为新建一个含该命令的 Step。
    void addCommandToCurrentStep(const QString& typeName);

private slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onTreeNodeDropped(int nodeType, int atIndex);
    void onCommandDroppedToGap(const QByteArray& payload, int atIndex);
    void onItemMoved(int fromIndex, int toIndex);
    void onInternalReorder(int from, int to);
    void onEmptyAreaDoubleClicked();
    void onItemEnterPressed(int index);

private:
    enum NodeType
    {
        NodeFunctions   = 1,
        NodeTboxCommand = 2,
        NodeIicWrite    = 3,
        NodeIicWriteRead = 4
    };

    void buildButtonBar();
    void buildTree();
    void buildItemList();

    FT_FunctionItem* currentItem() const;
    FT_FunctionItem* itemAt(int index) const;
    int rowOfItem(FT_FunctionItem* item) const;
    int rowOfGroup(FT_FunctionGroup* group) const;

    // 新建一个 Step 行并插入到 atIndex(0..count(),-1=末尾),完成全部信号挂接。
    FT_FunctionItem* createStep(const FT_FunctionItemConfig& cfg, int atIndex);
    void wireStepSignals(FT_FunctionItem* step);
    QString typeNameFromNode(int nodeType) const;
    void onNodeIntoGroup(FT_FunctionGroup* dest, int nodeType, int flat, bool hardBreak);
    void onCommandIntoGroup(FT_FunctionGroup* dest, const QByteArray& payload,
                            int flat, bool hardBreak);
    void splitStepAt(int row, int flat);
    void insertEmptyStepAfter(int row);
    void mergeStepWithNext(int row);

    void removeItem(FT_FunctionItem* item);
    void deleteCurrentItem();
    void moveCurrentItem(int delta);
    void moveItemUp(FT_FunctionItem* item);
    void moveItemDown(FT_FunctionItem* item);
    // 把第 from 行移动到插入位置 insertionPos（取值 0..count()）。
    // 注意：QListWidget 的 setItemWidget 控件绑定在 model index 上，
    // takeItem/removeItemWidget 都会立即 deleteLater 掉行控件，
    // 因此重排只能在固定的行之间轮转 config，不能移动 QListWidgetItem。
    void moveFunctionRow(int from, int insertionPos);

    bool exportConfig();
    void importConfig();
    void clearAllItems();

    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void updateEmptyHint();

    QWidget*         m_buttonBar     = nullptr;
    FT_FunctionTree* m_funcTree      = nullptr;
    FT_FunctionList* m_funcList      = nullptr;
    FT_Project*      m_project       = nullptr;
    QLabel*          m_emptyHint     = nullptr;
    QStatusBar*      m_statusBar     = nullptr;
};

#endif // FT_EDIT_H