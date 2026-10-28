#include <TB_CM_FilterBar.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>

namespace {

QToolButton* makeButton(QWidget* parent, const QString& objectName, const QString& text,
                        bool checkable, bool checked)
{
    auto* button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setText(text);
    button->setCheckable(checkable);
    button->setChecked(checked);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

} // namespace

TB_CM_FilterBar::TB_CM_FilterBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("TB_CM_FilterBar"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);
    search_ = new QLineEdit(this);
    search_->setObjectName(QStringLiteral("search"));
    search_->setPlaceholderText(tr("过滤日志（支持正则）"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumWidth(160);
    layout->addWidget(search_, 1);
    regexButton_ = makeButton(this, QStringLiteral("regexButton"), QStringLiteral(".*"), true, false);
    regexButton_->setToolTip(tr("使用正则表达式"));
    layout->addWidget(regexButton_);
    caseButton_ = makeButton(this, QStringLiteral("caseButton"), QStringLiteral("Aa"), true, false);
    caseButton_->setToolTip(tr("区分大小写"));
    layout->addWidget(caseButton_);
    layout->addSpacing(6);
    wrapButton_ = makeButton(this, QStringLiteral("wrapButton"), tr("换行"), true, true);
    wrapButton_->setToolTip(tr("自动换行"));
    layout->addWidget(wrapButton_);
    followButton_ = makeButton(this, QStringLiteral("followButton"), tr("滚动"), true, true);
    followButton_->setToolTip(tr("自动滚动到最新日志"));
    layout->addWidget(followButton_);
    bottomButton_ = makeButton(this, QStringLiteral("bottomButton"), tr("到底"), false, false);
    bottomButton_->setToolTip(tr("跳转到最新日志"));
    layout->addWidget(bottomButton_);
    clearButton_ = makeButton(this, QStringLiteral("clearButton"), tr("清空"), false, false);
    clearButton_->setToolTip(tr("清空全部日志"));
    layout->addWidget(clearButton_);
    counter_ = new QLabel(QStringLiteral("0 / 0"), this);
    counter_->setObjectName(QStringLiteral("counter"));
    counter_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    counter_->setMinimumWidth(96);
    layout->addWidget(counter_);
    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(120);
    connect(search_, &QLineEdit::textChanged, this, &TB_CM_FilterBar::onSearchTextChanged);
    connect(debounce_, &QTimer::timeout, this, &TB_CM_FilterBar::filterChanged);
    connect(regexButton_, &QToolButton::toggled, this, &TB_CM_FilterBar::onFilterOptionToggled);
    connect(caseButton_, &QToolButton::toggled, this, &TB_CM_FilterBar::onFilterOptionToggled);
    connect(wrapButton_, &QToolButton::toggled, this, &TB_CM_FilterBar::wrapChanged);
    connect(followButton_, &QToolButton::toggled, this, &TB_CM_FilterBar::followChanged);
    connect(bottomButton_, &QToolButton::clicked, this,
            &TB_CM_FilterBar::scrollToBottomRequested);
    connect(clearButton_, &QToolButton::clicked, this, &TB_CM_FilterBar::clearRequested);
    updateStyles();
}
Filter TB_CM_FilterBar::filter() const
{
    Filter filter;
    filter.text = search_->text();
    filter.mode = regexButton_->isChecked() ? MatchMode::Regex : MatchMode::Substring;
    filter.caseSensitive = caseButton_->isChecked();
    filter.searchCategory = true;
    return filter;
}
void TB_CM_FilterBar::setFilterText(const QString& text)
{
    if (search_->text() == text) {
        return;
    }
    const QSignalBlocker blocker(search_);
    search_->setText(text);
    emit filterChanged();
}
QString TB_CM_FilterBar::filterText() const
{
    return search_->text();
}
void TB_CM_FilterBar::setWrapEnabled(bool enabled)
{
    const QSignalBlocker blocker(this);
    wrapButton_->setChecked(enabled);
}
bool TB_CM_FilterBar::isWrapEnabled() const
{
    return wrapButton_->isChecked();
}
void TB_CM_FilterBar::setFollowTail(bool enabled)
{
    const QSignalBlocker blocker(this);
    followButton_->setChecked(enabled);
}
bool TB_CM_FilterBar::isFollowTail() const
{
    return followButton_->isChecked();
}
void TB_CM_FilterBar::setCounts(int visible, int total)
{
    const QLocale locale;
    counter_->setText(QStringLiteral("%1 / %2")
                          .arg(locale.toString(visible), locale.toString(total)));
}
void TB_CM_FilterBar::setTheme(const Theme& theme)
{
    theme_ = theme;
    updateStyles();
}
void TB_CM_FilterBar::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        debounce_->stop();
        emit filterChanged();
        return;
    }
    debounce_->start();
}
void TB_CM_FilterBar::onFilterOptionToggled()
{
    emit filterChanged();
}
void TB_CM_FilterBar::updateStyles()
{
    const Theme& theme = theme_;
    QString sheet = QStringLiteral(
        "#TB_CM_FilterBar { background: @bg; border-bottom: 1px solid @sep; }"
        "QLineEdit#search { background: @input; color: @text; border: 1px solid @sep;"
        " border-radius: 3px; padding: 3px 6px; selection-background-color: @sel;"
        " selection-color: @text; }"
        "QLineEdit#search:focus { border-color: @cat; }"
        "QLabel#counter { color: @muted; }"
        "QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px;"
        " padding: 2px 5px; color: @muted; font-size: 11px; }"
        "QToolButton:hover { background: @hover; }"
        "QToolButton:checked { background: @checked; border-color: @sep; }"
        "QToolButton#regexButton:checked, QToolButton#caseButton:checked { color: @cat; }"
        "QToolButton#bottomButton { color: @cat; }"
        "QToolButton#clearButton { color: @error; }");
    sheet.replace(QLatin1String("@bg"), theme.background.name());
    sheet.replace(QLatin1String("@sep"), theme.separator.name());
    sheet.replace(QLatin1String("@input"), theme.inputBackground.name());
    sheet.replace(QLatin1String("@text"), theme.text.name());
    sheet.replace(QLatin1String("@muted"), theme.textMuted.name());
    sheet.replace(QLatin1String("@sel"), theme.selection.name());
    sheet.replace(QLatin1String("@cat"), theme.category.name());
    sheet.replace(QLatin1String("@hover"), theme.hover.name());
    sheet.replace(QLatin1String("@checked"), theme.checkedBackground.name());
    sheet.replace(QLatin1String("@error"), theme.levelColors[size_t(Level::Error)].name());
    setStyleSheet(sheet);
}
