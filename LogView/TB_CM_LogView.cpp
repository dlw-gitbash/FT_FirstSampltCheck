#include "TB_CM_LogView.h"

#include "TB_CM_LogListView.h"
#include "TB_CM_LogModel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QToolButton* makeButton(QWidget* parent, const QString& objectName, const QString& text,
                        bool checkable, bool checked)
{
    auto* button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setText(text);
    button->setCheckable(checkable);
    button->setChecked(checked);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

} // namespace

TB_CM_LogView::TB_CM_LogView(QWidget* parent)
    : QWidget(parent)
    , model_(new TB_CM_LogModel(this))
    , listView_(new TB_CM_LogListView(this))
{
    listView_->setModel(model_);
    initFilterBar();
    initConnections();
    updateStyles();
}
TB_CM_LogView::~TB_CM_LogView() = default;
void TB_CM_LogView::appendLog(const QString& message, quint64 timestampMs, int level)
{
    model_->appendLog(message, timestampMs, level);
}
void TB_CM_LogView::appendLog(const TB_CM_LogEntry& entry)
{
    model_->appendLog(entry);
}
void TB_CM_LogView::clear()
{
    model_->clear();
}
void TB_CM_LogView::scrollToBottom()
{
    listView_->setFollowTail(true);
    listView_->scrollToBottom();
}
void TB_CM_LogView::initFilterBar()
{
    auto* bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("TB_CM_FilterBar"));
    bar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(8, 4, 8, 4);
    barLayout->setSpacing(4);
    search_ = new QLineEdit(bar);
    search_->setObjectName(QStringLiteral("search"));
    search_->setPlaceholderText(tr("过滤日志（支持正则）"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumWidth(160);
    barLayout->addWidget(search_, 1);
    regexButton_ = makeButton(bar, QStringLiteral("regexButton"), QStringLiteral(".*"), true, false);
    regexButton_->setToolTip(tr("使用正则表达式"));
    barLayout->addWidget(regexButton_);
    caseButton_ = makeButton(bar, QStringLiteral("caseButton"), QStringLiteral("Aa"), true, false);
    caseButton_->setToolTip(tr("区分大小写"));
    barLayout->addWidget(caseButton_);
    barLayout->addSpacing(6);
    wrapButton_ = makeButton(bar, QStringLiteral("wrapButton"), tr("换行"), true, true);
    wrapButton_->setToolTip(tr("自动换行"));
    barLayout->addWidget(wrapButton_);
    followButton_ = makeButton(bar, QStringLiteral("followButton"), tr("滚动"), true, true);
    followButton_->setToolTip(tr("自动滚动到最新日志"));
    barLayout->addWidget(followButton_);
    bottomButton_ = makeButton(bar, QStringLiteral("bottomButton"), tr("到底"), false, false);
    bottomButton_->setToolTip(tr("跳转到最新日志"));
    barLayout->addWidget(bottomButton_);
    clearButton_ = makeButton(bar, QStringLiteral("clearButton"), tr("清空"), false, false);
    clearButton_->setToolTip(tr("清空全部日志"));
    barLayout->addWidget(clearButton_);
    counter_ = new QLabel(QStringLiteral("0 / 0"), bar);
    counter_->setObjectName(QStringLiteral("counter"));
    counter_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    counter_->setMinimumWidth(96);
    barLayout->addWidget(counter_);
    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(120);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(bar);
    layout->addWidget(listView_, 1);
}
void TB_CM_LogView::initConnections()
{
    connect(search_, &QLineEdit::textChanged, this, &TB_CM_LogView::onSearchTextChanged);
    connect(debounce_, &QTimer::timeout, this, &TB_CM_LogView::onFilterChanged);
    connect(regexButton_, &QToolButton::toggled, this, &TB_CM_LogView::onFilterOptionToggled);
    connect(caseButton_, &QToolButton::toggled, this, &TB_CM_LogView::onFilterOptionToggled);
    connect(wrapButton_, &QToolButton::toggled, this, &TB_CM_LogView::onWrapChanged);
    connect(followButton_, &QToolButton::toggled, this, &TB_CM_LogView::onFollowChanged);
    connect(bottomButton_, &QToolButton::clicked, this, &TB_CM_LogView::scrollToBottom);
    connect(clearButton_, &QToolButton::clicked, this, &TB_CM_LogView::clear);
    connect(listView_, &TB_CM_LogListView::clearRequested, this, &TB_CM_LogView::clear);
    connect(listView_, &TB_CM_LogListView::followTailChanged, this,
            &TB_CM_LogView::onViewFollowTailChanged);
    connect(model_, &TB_CM_LogModel::countsChanged, this, &TB_CM_LogView::onCountsChanged);
}
void TB_CM_LogView::updateStyles()
{
    setStyleSheet(QStringLiteral(
        "#TB_CM_FilterBar { background: transparent; }"
        "QToolButton { color: black; background-color: rgb(211, 211, 211);"
        " border-radius: 1px; padding: 2px 6px; }"
        "QToolButton:hover { background-color: rgb(192, 192, 192); }"
        "QToolButton:pressed { background-color: rgb(211, 211, 211);"
        " padding-left: 7px; padding-top: 3px; }"
        "QToolButton:checked { background-color: rgb(192, 192, 192); }"
        "QLineEdit#search { background-color: white; border-style: solid; border-width: 1px;"
        " border-top-color: rgba(255, 255, 255, 0); border-right-color: rgba(255, 255, 255, 0);"
        " border-left-color: rgba(255, 255, 255, 0); border-bottom-color: rgb(218, 218, 218);"
        " padding: 3px 6px; }"
        "QLineEdit#search:hover { border-bottom-color: rgb(129, 129, 129); }"
        "QLineEdit#search:pressed { border-bottom-color: rgb(129, 129, 129); }"));
}
void TB_CM_LogView::onFilterChanged()
{
    TB_CM_Filter filter;
    filter.text = search_->text();
    filter.mode = regexButton_->isChecked() ? TB_CM_MatchMode::Regex : TB_CM_MatchMode::Substring;
    filter.caseSensitive = caseButton_->isChecked();
    filter.searchCategory = true;
    model_->setFilter(filter);
}
void TB_CM_LogView::onFilterOptionToggled()
{
    debounce_->stop();
    onFilterChanged();
}
void TB_CM_LogView::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        debounce_->stop();
        onFilterChanged();
        return;
    }
    debounce_->start();
}
void TB_CM_LogView::onWrapChanged(bool enabled)
{
    listView_->setWrapEnabled(enabled);
}
void TB_CM_LogView::onFollowChanged(bool enabled)
{
    listView_->setFollowTail(enabled);
}
void TB_CM_LogView::onViewFollowTailChanged(bool enabled)
{
    const QSignalBlocker blocker(followButton_);
    followButton_->setChecked(enabled);
}
void TB_CM_LogView::onCountsChanged(int visible, int total)
{
    const QLocale locale;
    counter_->setText(QStringLiteral("%1 / %2")
                          .arg(locale.toString(visible), locale.toString(total)));
    emit countsChanged(visible, total);
}
