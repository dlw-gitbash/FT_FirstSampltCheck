#pragma once

#include "LogFilterProxy.h"
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class QHBoxLayout;

// =====================================================
// LogToolBar：日志工具栏
// 功能：搜索框、级别过滤、正则开关、自动滚动、清空等
// =====================================================
class LogToolBar : public QWidget {
    Q_OBJECT

public:
    explicit LogToolBar(QWidget* parent = nullptr);

    void setFilterProxy(LogFilterProxy* proxy);

signals:
    void searchTextChanged(const QString& text);
    void levelFilterChanged(LogLevel level, bool visible);
    void autoScrollToggled(bool enabled);
    void clearRequested();
    void pauseRequested(bool paused);

public slots:
    void updateStats(int visible, quint32 total);

private:
    void setupUI();
    void buildLevelButton(LogLevel level, const QString& label);

    LogFilterProxy* m_proxy = nullptr;

    QLineEdit*  m_searchEdit;
    QPushButton* m_regexBtn;
    QComboBox*  m_scopeCombo;
    QCheckBox*  m_caseSensitiveCheck;

    QHash<LogLevel, QPushButton*> m_levelButtons;
    QHash<LogLevel, bool>         m_levelVisible;

    QPushButton* m_autoScrollBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_clearBtn;
    QLabel*      m_statsLabel;

    bool m_autoScroll = true;
    bool m_paused = false;
};