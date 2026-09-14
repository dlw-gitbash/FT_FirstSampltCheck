#pragma once

#include "core/LogModel.h"
#include "core/LogFilterProxy.h"
#include <QWidget>
#include <QTableView>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMutex>

class LogToolBar;
class LogHighlighter;

// =====================================================
// LogTableView：日志视图主组件
// 特点：
//   1. QTableView + 自定义 Model + FilterProxy 三层架构
//   2. 自动滚动到底部（可暂停）
//   3. 上下文菜单（复制）
//   4. 列宽自适应和手动调整
//   5. 线程安全日志添加 API
// =====================================================
class LogTableView : public QWidget {
    Q_OBJECT

public:
    explicit LogTableView(QWidget* parent = nullptr);
    ~LogTableView() override;

    // --- 日志添加 API（线程安全） ---
    void addLog(LogEntry entry);
    void addLogs(std::vector<LogEntry>& entries);

    // 便捷日志添加方法
    void addLog(LogLevel level, const QString& logger, const QString& message,
                const QString& thread = QString(), int line = -1);
    void addLogTrace(const QString& logger, const QString& msg, const QString& thread = QString(), int line = -1);
    void addLogDebug(const QString& logger, const QString& msg, const QString& thread = QString(), int line = -1);
    void addLogInfo(const QString& logger, const QString& msg, const QString& thread = QString(), int line = -1);
    void addLogWarn(const QString& logger, const QString& msg, const QString& thread = QString(), int line = -1);
    void addLogError(const QString& logger, const QString& msg, const QString& thread = QString(), int line = -1);
    void addLogFatal(const QString& logger, const QString& msg, const QString& thread = QString(), int line = -1);

    // --- 控制接口 ---
    // 暂停/恢复日志更新
    void setPaused(bool paused);
    bool isPaused() const { return m_paused; }

    // 清空所有日志
    void clear();

    // 设置最大缓冲条目数
    void setMaxEntries(int max);

    // 设置批量刷新间隔（毫秒）
    void setBatchInterval(int ms);

    // 获取内部模型（高级用户可直接操作模型）
    LogModel*       logModel()       { return m_model; }
    LogFilterProxy* filterProxy()    { return m_proxy; }
    QTableView*     tableView()      { return m_tableView; }

    // --- 视图配置 ---
    void setColumnVisible(int column, bool visible);
    void setColumnWidth(int column, int width);
    void restoreDefaultLayout();

signals:
    void logCleared();

private slots:
    void onStatsUpdate(int visible, quint32 total);
    void onAutoScrollToggled(bool enabled);
    void onPauseToggled(bool paused);
    void onSelectionChanged();
    void onCustomContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupConnections();
    void scrollToBottom();
    void applyDefaultColumnWidths();
    void copySelectedRows();

    QTableView*     m_tableView;
    LogModel*       m_model;
    LogFilterProxy* m_proxy;
    LogToolBar*     m_toolBar;
    LogHighlighter* m_delegate;

    bool m_autoScroll = true;
    bool m_paused = false;

    // 上下文菜单
    QMenu*   m_contextMenu;
    QAction* m_copyAction;
    QAction* m_copyRowAction;
    QAction* m_selectAllAction;
};