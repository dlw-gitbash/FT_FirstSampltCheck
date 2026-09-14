#include "LogFilterProxy.h"
#include "LogModel.h"

LogFilterProxy::LogFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_regexMode(false)
    , m_caseSensitive(false)
    , m_scopes(ScopeAll)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void LogFilterProxy::setSearchText(const QString& text)
{
    m_searchText = text;

    if (m_regexMode && !text.isEmpty()) {
        auto pattern = m_caseSensitive
            ? QRegularExpression(text)
            : QRegularExpression(text, QRegularExpression::CaseInsensitiveOption);
        if (pattern.isValid()) {
            m_searchRegex = pattern;
        }
    }

    invalidateFilter();
}

void LogFilterProxy::setRegexMode(bool enabled)
{
    m_regexMode = enabled;
    // 重新应用搜索文本以更新正则
    if (!m_searchText.isEmpty()) {
        setSearchText(m_searchText);
    }
}

void LogFilterProxy::setCaseSensitive(bool enabled)
{
    m_caseSensitive = enabled;
    setFilterCaseSensitivity(enabled ? Qt::CaseSensitive : Qt::CaseInsensitive);
    if (m_regexMode && !m_searchText.isEmpty()) {
        setSearchText(m_searchText);
    }
    invalidateFilter();
}

void LogFilterProxy::setSearchScopes(SearchScopes scopes)
{
    m_scopes = scopes;
    invalidateFilter();
}

void LogFilterProxy::setLevelFilter(LogLevel level, bool visible)
{
    if (visible) {
        m_hiddenLevels.remove(level);
    } else {
        m_hiddenLevels.insert(level);
    }
    invalidateFilter();
}

void LogFilterProxy::setLevelsVisible(const QSet<LogLevel>& levels)
{
    m_hiddenLevels.clear();
    for (int i = 0; i <= static_cast<int>(LogLevel::Fatal); ++i) {
        LogLevel lv = static_cast<LogLevel>(i);
        if (!levels.contains(lv)) {
            m_hiddenLevels.insert(lv);
        }
    }
    invalidateFilter();
}

bool LogFilterProxy::isLevelVisible(LogLevel level) const
{
    return !m_hiddenLevels.contains(level);
}

void LogFilterProxy::showAllLevels()
{
    m_hiddenLevels.clear();
    invalidateFilter();
}

int LogFilterProxy::mapToSourceRow(int proxyRow) const
{
    QModelIndex proxyIdx = index(proxyRow, 0);
    QModelIndex sourceIdx = mapToSource(proxyIdx);
    return sourceIdx.isValid() ? sourceIdx.row() : -1;
}

bool LogFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent)

    if (!sourceModel()) return false;

    // 级别过滤
    QModelIndex levelIdx = sourceModel()->index(sourceRow, LogModel::ColLevel);
    int levelValue = sourceModel()->data(levelIdx, Qt::UserRole).toInt();
    auto level = static_cast<LogLevel>(levelValue);
    if (m_hiddenLevels.contains(level)) {
        return false;
    }

    // 文本搜索
    if (m_searchText.isEmpty()) {
        return true;
    }

    // 获取对应列的文本进行匹配
    if (m_scopes & ScopeMessage) {
        QModelIndex msgIdx = sourceModel()->index(sourceRow, LogModel::ColMessage);
        QString msg = sourceModel()->data(msgIdx, Qt::DisplayRole).toString();
        if (matchText(msg)) return true;
    }

    {
        QModelIndex tsIdx = sourceModel()->index(sourceRow, LogModel::ColTimestamp);
        QString ts = sourceModel()->data(tsIdx, Qt::DisplayRole).toString();
        if (matchText(ts)) return true;
    }

    return false;
}

bool LogFilterProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    return QSortFilterProxyModel::lessThan(left, right);
}

bool LogFilterProxy::matchText(const QString& text) const
{
    if (m_regexMode) {
        return m_searchRegex.isValid() && m_searchRegex.match(text).hasMatch();
    }
    return text.contains(m_searchText, m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
}