#include "LogToolBar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

LogToolBar::LogToolBar(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LogToolBar::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    // 搜索框
    layout->addWidget(new QLabel(QStringLiteral("搜索:")));

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("输入搜索关键词..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMinimumWidth(180);
    m_searchEdit->setMaximumWidth(300);
    // 防抖：用户停止输入 200ms 后触发搜索
    auto* searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    searchTimer->setInterval(200);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this, searchTimer]() {
        searchTimer->start();
    });
    connect(searchTimer, &QTimer::timeout, this, [this]() {
        if (m_proxy) {
            m_proxy->setSearchText(m_searchEdit->text());
        }
        emit searchTextChanged(m_searchEdit->text());
    });
    // 回车立即搜索
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this, searchTimer]() {
        searchTimer->stop();
        if (m_proxy) {
            m_proxy->setSearchText(m_searchEdit->text());
        }
        emit searchTextChanged(m_searchEdit->text());
    });
    layout->addWidget(m_searchEdit);

    // 正则开关
    m_regexBtn = new QPushButton(QStringLiteral(".*"));
    m_regexBtn->setCheckable(true);
    m_regexBtn->setToolTip(QStringLiteral("正则表达式模式"));
    m_regexBtn->setFixedWidth(32);
    m_regexBtn->setStyleSheet(
        "QPushButton { padding: 2px; }"
        "QPushButton:checked { background-color: #0078D4; color: white; border-radius: 3px; }");
    connect(m_regexBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (m_proxy) {
            m_proxy->setRegexMode(checked);
            m_proxy->setSearchText(m_searchEdit->text());
        }
    });
    layout->addWidget(m_regexBtn);

    // 大小写敏感
    m_caseSensitiveCheck = new QCheckBox(QStringLiteral("Aa"));
    m_caseSensitiveCheck->setToolTip(QStringLiteral("大小写敏感"));
    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_proxy) {
            m_proxy->setCaseSensitive(checked);
            m_proxy->setSearchText(m_searchEdit->text());
        }
    });
    layout->addWidget(m_caseSensitiveCheck);

    // 分隔符
    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    layout->addWidget(sep1);

    // 搜索范围
    layout->addWidget(new QLabel(QStringLiteral("搜索范围:")));
    m_scopeCombo = new QComboBox;
    m_scopeCombo->addItem(QStringLiteral("全部"), static_cast<int>(LogFilterProxy::ScopeAll));
    m_scopeCombo->addItem(QStringLiteral("消息"), static_cast<int>(LogFilterProxy::ScopeMessage));
    connect(m_scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        int scope = m_scopeCombo->currentData().toInt();
        if (m_proxy) {
            m_proxy->setSearchScopes(static_cast<LogFilterProxy::SearchScopes>(scope));
            m_proxy->setSearchText(m_searchEdit->text());
        }
    });
    layout->addWidget(m_scopeCombo);

    // 分隔符
    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::VLine);
    layout->addWidget(sep2);

    // 级别过滤按钮
    layout->addWidget(new QLabel(QStringLiteral("级别:")));
    buildLevelButton(LogLevel::Trace, QStringLiteral("T"));
    buildLevelButton(LogLevel::Debug, QStringLiteral("D"));
    buildLevelButton(LogLevel::Info,  QStringLiteral("I"));
    buildLevelButton(LogLevel::Warn,  QStringLiteral("W"));
    buildLevelButton(LogLevel::Error, QStringLiteral("E"));
    buildLevelButton(LogLevel::Fatal, QStringLiteral("F"));

    // 分隔符
    auto* sep3 = new QFrame;
    sep3->setFrameShape(QFrame::VLine);
    layout->addWidget(sep3);

    // 自动滚动
    m_autoScrollBtn = new QPushButton(QStringLiteral("↓ 自动滚动"));
    m_autoScrollBtn->setCheckable(true);
    m_autoScrollBtn->setChecked(true);
    m_autoScrollBtn->setStyleSheet(
        "QPushButton:checked { background-color: #0078D4; color: white; border-radius: 3px; padding: 2px 6px; }"
        "QPushButton:!checked { padding: 2px 6px; }");
    connect(m_autoScrollBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_autoScroll = checked;
        m_autoScrollBtn->setText(checked ? QStringLiteral("↓ 自动滚动") : QStringLiteral("→ 手动滚动"));
        emit autoScrollToggled(checked);
    });
    layout->addWidget(m_autoScrollBtn);

    // 暂停/恢复
    m_pauseBtn = new QPushButton(QStringLiteral("暂停"));
    m_pauseBtn->setCheckable(true);
    m_pauseBtn->setStyleSheet(
        "QPushButton:checked { background-color: #CC0000; color: white; border-radius: 3px; padding: 2px 6px; }"
        "QPushButton:!checked { padding: 2px 6px; }");
    connect(m_pauseBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_paused = checked;
        m_pauseBtn->setText(checked ? QStringLiteral("▶ 已暂停") : QStringLiteral("暂停"));
        emit pauseRequested(checked);
    });
    layout->addWidget(m_pauseBtn);

    // 清空
    m_clearBtn = new QPushButton(QStringLiteral("清空"));
    m_clearBtn->setStyleSheet("QPushButton { padding: 2px 8px; }");
    connect(m_clearBtn, &QPushButton::clicked, this, &LogToolBar::clearRequested);
    layout->addWidget(m_clearBtn);

    // stretch
    layout->addStretch();

    // 统计信息
    m_statsLabel = new QLabel(QStringLiteral("显示: 0 / 总计: 0"));
    m_statsLabel->setStyleSheet("color: #666; padding-right: 4px;");
    layout->addWidget(m_statsLabel);
}

void LogToolBar::buildLevelButton(LogLevel level, const QString& label)
{
    auto* btn = new QPushButton(label);
    btn->setCheckable(true);
    btn->setChecked(true);
    btn->setFixedSize(26, 24);
    btn->setToolTip(QString::fromLatin1(LogLevelHelper::levelName(level)));

    QColor color = LogLevelHelper::levelColor(level);
    QString colorName = color.name();
    QString bgColor = LogLevelHelper::levelBgColor(level).name();
    btn->setStyleSheet(
        QString("QPushButton { font-weight: bold; font-size: 11px; color: %1; "
                "background-color: %2; border: 1px solid %1; border-radius: 3px; padding: 0; }"
                "QPushButton:checked { background-color: %1; color: white; }"
                "QPushButton:hover { border-width: 2px; }")
            .arg(colorName, bgColor));

    connect(btn, &QPushButton::toggled, this, [this, level](bool visible) {
        m_levelVisible[level] = visible;
        if (m_proxy) {
            m_proxy->setLevelFilter(level, visible);
        }
        emit levelFilterChanged(level, visible);
    });

    m_levelButtons[level] = btn;
    m_levelVisible[level] = true;
    layout()->addWidget(btn);
}

void LogToolBar::setFilterProxy(LogFilterProxy* proxy)
{
    m_proxy = proxy;
}

void LogToolBar::updateStats(int visible, quint32 total)
{
    m_statsLabel->setText(QStringLiteral("显示: %1 / 总计: %2").arg(visible).arg(total));
}