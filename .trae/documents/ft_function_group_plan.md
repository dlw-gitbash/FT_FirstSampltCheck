# FT_FunctionGroup 命令容器(组内折行/拆分 Step/嵌套拖拽)实施计划

## 需求结论(已与用户确认)

1. `FT_FunctionItem::m_function`(单条)→ `FT_FunctionGroup`(一 Step 内可容纳多条 `FT_Function`)。
2. **换行语义(两者都要)**:
   - 组内折行:命令在同一 Step 内强制另起视觉行,Step 高度随行数增加;
   - 拆分 Step:可把某条命令起拆成外层新 `FT_FunctionItem`;反向可把下一 Step 合并进来。
3. **排布**:宽度不够时自动流式折行(软换行),右键/拖拽产生的是强制断点(硬换行)。
4. **拖拽范围**:组内重排/折行、跨 Step 拖动、左侧树节点直接拖入某个 Step 的组内;外层整行重排保留。
5. 右键菜单与拖拽两条路径都能触发换行;**必须保持现有自适应高度链**。

## Repository Research(现状)

- 外层 `FT_FunctionList`(QListWidget):行控件用 `setItemWidget` 绑定。已证实 Qt6 中 `takeItem/removeItemWidget` 会对行控件 `deleteLater`,所以**外层重排只能轮转 config、不能移动 item**(现有 `moveFunctionRow` 已如此实现,继续沿用)。
- `FT_FunctionItem`:QHBoxLayout `[勾选 28][标题 120][m_function, stretch=1][延时 80]`;高度链为
  `FT_Function::requestResize → FT_FunctionItem::adjustHeight() → setFixedHeight + outer QListWidgetItem::setSizeHint`;
  内含重入保护 `m_adjusting/m_pendingAdjust`;`resizeEvent` 也触发 `adjustHeight`。
- `FT_Function` 三个子类各自重复实现了相同的右键菜单(Response Advance / Move Up / Down / Remove)与 `eventFilter`;`updateFunctionHeight()` 以 `setMinimumHeight + requestResize` 驱动高度。
- `FHintTextEdit` 随内容自动增高(`WidgetWidth` 换行 → `heightChanged`);其宽度依赖父布局给的 stretch 空间。
- 数据:`FT_FunctionItemConfig{ enabled,title,FT_FunctionData functionData,delayMs }`,文档 = `QVector<item>`;JSON `version=1`,item 字段 `enabled/title/delayMs/function{...}`。
- `FT_Project`:采集/导入导出;hex 校验只扫 Tbox payload;导入时重建全部行。
- `FT_Test` 不访问命令内部数据,改造不受影响。
- 拖拽 MIME:树节点 `application/x-ft-node`(int);外层内部重排靠 `startDrag` 缓存 `m_dragSourceRow`,`dropEvent` 发 `internalReorderRequested(from,to)`。

## 目标结构

```
FT_FunctionList(QListWidget,行=Step,不移动 item,只轮转 item-config)
└─ FT_FunctionItem  [勾选|标题| FT_FunctionGroup(stretch) |延时]
                     └─ FtFlowBreakLayout:扁平持有 FT_Function* 序列
                        + 每个 FT_Function 带 breakBefore(硬换行)标志
                        宽度不足自动软换行;几何变化 → 重算总高 → item.adjustHeight()
```

数据模型:
```cpp
struct FT_FunctionItemConfig {
    bool enabled = true;
    QString title;
    int delayMs = 1000;
    QVector<QVector<FT_FunctionData>> lines; // 每个内层 vector = 一条硬换行行;
                                            // 行内多条命令宽度不足时仍可软折行
};
```
- 空 `lines` 合法(空 Step,显示虚框拖放区)。
- JSON 升级 `version=2`:item 写 `"lines":[[fn,fn],[fn]]`;**读取兼容 v1**:无 `lines` 但有旧 `"function"` 对象时 → 单行单命令。
- 命令的扁平序号 flat = 跨行顺序累加(拖拽坐标用 line/col 与 flat 互换)。

## Files and Modules

- `Module/FT_Type.h`:`FT_FunctionItemConfig.functionData` → `lines`(二维)。
- `Module/FT_Data.{h,cpp}`:item 读写改为 lines;保留 v1 读取路径;新增 `ftLinesToJson/ftLinesFromJson`(内部)。
- **新增** `Module/FT_FunctionGroup.{h,cpp}`:`FT_FunctionGroup` 容器 + `FtFlowBreakLayout`(QLayout),加入 CMakeLists。
- `Module/FT_Function.{h,cpp}`:
  - 基类新增 `virtual void openResponseAdvance() = 0;` 与统一的非虚 `showCommandMenu(globalPos, flags)`;
  - 新增信号 `requestWrap()/requestUnwrap()/requestSplitNewStep()`;`requestRemove/requestMoveUp/requestMoveDown` 保留(改为"组内命令前移/后移"语义);
  - 三个子类删除各自重复的菜单/eventFilter 实现,只保留 Advance 对话框与高度逻辑;增加 `preferredWidth()`(默认按内部控件 minimumSizeHint 估算)供布局分配宽度。
- `Module/FT_FunctionItem.{h,cpp}`:`m_function` → `m_functionGroup`;`toConfig/applyConfig` 走 group;`adjustHeight` 以 group 高度为内容高;右键菜单新增"在后面换行(新空 Step)""与下一 Step 合并";高度链接 group 的 `requestResize`。
- `Module/FT_FunctionList.{h,cpp}`:
  - 新增 MIME `application/x-ft-command`(JSON:命令 config + 源 item 行 + flat 序号);
  - 外层 `dragEnter/dragMove/dropEvent` 增加命令落点分支(落在行间间隙 → 发 `commandDroppedToGap`);落在组内由 group 自行 accept;
  - 树节点拖入组内时外层不介入(group 优先接收)。
- `FT_Edit.{h,cpp}`:新增/改造槽:
  - 建 item 改为 group 版;`addXxxItem` 默认建含一条命令的 Step;
  - `onNodeIntoGroup(row,line,col,nodeType,hardBreak)`;
  - `onCommandWithinGroups(cfgJson,srcRow,srcFlat,destRow,destLine,destCol,hardBreak)`(唯一索引归一化层);
  - `onCommandDroppedToGap(...)`(命令 → 新 Step);
  - `splitStepAt(row,line,col)`、`insertEmptyStepAfter(row)`、`mergeStepWithNext(row)`;
  - 组内命令 remove/wrap/unwrap/move 由 item/group 信号汇集处理;
  - 导出 hex 校验、`FT_Project` 采集校验改为遍历 lines。
- `Module/FT_Project.{h,cpp}`:`collectDocument` 不变(调 item->toConfig);`validatePayloadHex` 展开二维;`importFromFile` 用新 `applyConfig`。

## 关键设计

### 1. FtFlowBreakLayout(自定义 QLayout)
- 内部维护扁平 `QLayoutItem` 列表 + `QSet<QWidget*> m_breakBefore`。
- `setGeometry(rect)`:贪心打包——按 widget **最小宽**判断能否放入当前视觉行,放不下则软换行;`breakBefore` 的 widget 强制新行;行内剩余宽度按权重(各 `FT_Function::preferredWidth()`,等权兜底)分配给每个命令,保证 payload 输入框有可用宽度(替代今天的 stretch=1)。
- 行高 = 行内各命令在分配宽度下的 `heightForWidth/minimumHeight` 最大值(多行 payload 的命令更高,同现在的对齐效果)。
- `minimumHeightForWidth(width)` / `heightForWidth`;几何或内容变化后若总高变化 → 发 `group->requestResize()`;带重入保护,高度无变化不下发。
- 提供 `hitTest(pos)` → `{flatIndex, side(前/后), hardBreakZone}`,供拖放指示与右键目标计算;硬换新区 = 行首左侧带状区/硬行之间的间隙。

### 2. 高度链(保持原机制)
命令内容变高 → `FT_Function::updateFunctionHeight`(不变)→ group 监听到后让 layout 重算 → group `setMinimumHeight` + `requestResize` →
`FT_FunctionItem::adjustHeight()`:`contentH = max(titleMinH, group->minimumHeight())` → `setFixedHeight` + 外层 `QListWidgetItem::setSizeHint`(沿用现有 `m_adjusting/m_pendingAdjust` 防递归)。
窗口宽度变化路径:item `resizeEvent` → group 宽度变 → layout 软换行结果变 → 同上链。

### 3. 拖拽分工与事件隔离
- **命令拖拽起点**:group 对每个 `FT_Function` 装 eventFilter,仅当按压目标是命令"非编辑区"(命令本体/标签,不是 QTextEdit/QComboBox/QSpinBox)且移动超阈值才启动 `QDrag(x-ft-command)`;编辑区内不发起,保证选文本/下拉正常。
- **整 Step 拖拽**:从 item 边框区(勾选/标题/延时/group 空白 padding)起拖,仍走 `FT_FunctionList::startDrag`(子编辑控件与命令区之外才会冒泡给外层)。
- **落点**:group `acceptDrops=true`,自行处理组内/跨组/树节点,渲染两种插入指示:命令间竖线(inline)与横跨整组的横线(硬换行);落在行间间隙/空白区由外层处理为"新 Step"。
- 经验约束:拖拽起点在 startDrag 固化(srcRow+flat 写入 MIME);**索引归一化只在 FT_Edit 一处做**(先删源,同 item 时修正目标坐标,再插目标)。

### 4. 右键菜单
- 命令菜单(基类统一):Response Advance… / 在此强制换行 / 与上行合并(取消换行)/ 从此拆为新 Step / ─ / 命令前移 / 命令后移 / 删除。
- 组空白区菜单:追加 TBox / I2C Write / I2C Write+Read。
- Item 菜单(标题/勾选/延时区):Move Up/Down、Remove(现有)+ 在后面换行(新空 Step)、与下一 Step 合并。

### 5. 空组与删除语义
- 移走/删光命令后保留空 Step(虚框 + "拖入或双击添加命令"),避免拖拽误删整行;外层 Clear/Remove 才删 Step。
- `splitStepAt`:在 (line,col) 处把扁平序列切成两个 item config,标题/延时:前段保留,新段用默认标题/延时;split 后焦点选中新 Step 首条命令。
- `mergeStepWithNext`:下一 Step 第一行并入当前 Step 最后一行(硬断点解除),其余行依次接入。

## Implementation Steps(依赖序)

1. `FT_Type.h` + `FT_Data`:lines 数据结构与 JSON v2 读写/v1 兼容;先加后用,保证可单独编译。
2. `FT_FunctionGroup` + `FtFlowBreakLayout`:纯容器(增删/折行/合并/hitTest/高度信号),先用最小 harness 验证布局与高度。
3. `FT_Function` 菜单基类化(去三处重复)+ 新信号 + `preferredWidth`。
4. `FT_FunctionItem` 接入 group:toConfig/applyConfig、adjustHeight 新高度链、菜单新动作。
5. `FT_FunctionList` 命令 MIME 与外层落点分支;group 内拖放(组内/跨组/树节点)与指示条。
6. `FT_Edit`:全部新槽、拆分/合并/空 Step、唯一索引归一化;`FT_Project` 校验/导入适配。
7. CMakeLists 加入新文件;全量编译。

## Dependencies and Considerations

- 仅依赖 Qt6 Widgets + nlohmann/json(已在);新增代码用同一 MOC 流程。
- 外层 item 依旧"固定槽 + config 轮转":跨行重排时 group 整体重建(与今天重建单个 function 一致,可接受)。
- JSON 向后兼容是硬要求;保存统一输出 v2。
- 命令宽度:流式行内按 preferredWidth 权重分配,避免 payload 框被压到不可用;最小宽度仍能触发软换行。
- 嵌套拖拽时父子事件优先级:子 group accept 后事件不再冒泡给外层;MIME 三类(node/command/item)严格区分。

## Validation

- 复用上次的真实窗口 harness 方式做自动化验证(临时工程,验证后删除):
  1. 向同一 Step 追加 4 条命令,窄宽度下出现软换行且 Step 外层层高随 `sizeHint` 增大;恢复宽宽度层高回落;
  2. 右键强制换行/取消、拖拽命令间竖线插入、行首横条硬换行,顺序与断点数正确;
  3. 从此拆为新 Step、与下一 Step 合并、拖命令到行间生成新 Step、树节点拖入组内、跨组移动;连续 20+ 次操作无空行/无崩溃(重点回归上次的悬空指针问题);
  4. 序列化:v2 roundtrip(lines 结构/顺序/断点保持);构造 v1 JSON 读入后正确变单行组;
  5. 外层整行 Move Up/Down、Delete、Clear、Save/Load 行为不回归;
  6. payload 多行内容增高仍驱动层高(自适应高度链)。
- MSVC 既有 build 目录全量编译通过;启动截图确认折行/指示条/菜单视觉。

## Risks

- **高度反馈循环**:布局→minHeight→外层 sizeHint→再次布局。沿用现有重入守卫,且总高未变不下发;harness 加压力断言。
- **嵌套拖拽冲突/双重拖拽**:按压来源严格区分(编辑区/命令区/Step chrome);只保留一条 startDrag 路径来源;MIME 分支互斥。
- **跨组移动索引错位**:src 坐标先于一切变更捕获;源先删、同 item 时按扁平序修正目标;只在 FT_Edit 归一化一次。
- **流式宽度分配难看**:preferredWidth 等权兜底,后续可平滑调参,不影响数据层。
- **菜单重构触及三类命令**:行为(Advance 对话框/高度)必须保持,逐类目视回归。
