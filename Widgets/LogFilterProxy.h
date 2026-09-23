#pragma once

#include "LogEntry.h"
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QSet>
#include <QDateTime>

// =====================================================
// LogFilterProxy：高性能过滤代理模型
// 特点：
//   1. 文本搜索（支持正则/普通模式）
//   2. 日志级别过滤（可多选）
//   3. 搜索范围可选（消息/模块/线程）
//   4. 复用 QSortFilterProxyModel 的高效过滤机制
// =====================================================
class LogFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    enum SearchScope {
        ScopeMessage   = 0x01,
        ScopeTimestamp = 0x02,
        ScopeAll       = ScopeMessage | ScopeTimestamp
    };
    Q_DECLARE_FLAGS(SearchScopes, SearchScope)

    explicit LogFilterProxy(QObject* parent = nullptr);

    // --- 文本搜索 ---
    void setSearchText(const QString& text);
    QString searchText() const { return m_searchText; }

    // 设置是否使用正则表达式
    void setRegexMode(bool enabled);
    bool isRegexMode() const { return m_regexMode; }

    // 设置是否大小写敏感
    void setCaseSensitive(bool enabled);

    // 设置搜索范围
    void setSearchScopes(SearchScopes scopes);
    SearchScopes searchScopes() const { return m_scopes; }

    // --- 级别过滤 ---
    void setLevelFilter(LogLevel level, bool visible);
    void setLevelsVisible(const QSet<LogLevel>& levels);
    bool isLevelVisible(LogLevel level) const;
    void showAllLevels();

    // --- 其他 ---
    // 获取原始模型中的行索引
    int mapToSourceRow(int proxyRow) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    bool matchText(const QString& text) const;

    QString          m_searchText;
    QRegularExpression m_searchRegex;
    bool             m_regexMode;
    bool             m_caseSensitive;
    SearchScopes     m_scopes;

    QSet<LogLevel>   m_hiddenLevels;  // 隐藏的级别集合
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LogFilterProxy::SearchScopes)