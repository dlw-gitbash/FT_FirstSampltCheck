#include "FT_Function.h"
#include "FT_Widget.h"
#include "FT_Data.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QFont>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QRegularExpression>
#include <QMenu>
#include <QAction>
#include <QPalette>
#include <QContextMenuEvent>
#include <QEvent>
#include <QColor>
#include <QStyle>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTimer>
#include <QResizeEvent>

namespace {
constexpr int kFunctionDefaultHeight = 36;
}

FT_Function::FT_Function(QWidget* parent)
    : QWidget(parent)
{
    m_functionLayout = new QHBoxLayout(this);
    m_functionLayout->setContentsMargins(0, 0, 0, 0);
    m_functionLayout->setSpacing(6);
}

FT_Function::~FT_Function() = default;

int FT_Function::packMinimumWidth() const
{
    int w = 0;
    int n = 0;
    for (int i = 0; i < m_functionLayout->count(); ++i) {
        QWidget* cw = m_functionLayout->itemAt(i)->widget();
        if (!cw)
            continue;
        int cwMin = qMax(cw->minimumSize().width(), cw->minimumSizeHint().width());
        // 固定宽度控件的 minimumSizeHint 可能大于其 fixed 宽度,
        // 打包最小值必须受 maximumSize 约束,否则 min > max 会让换行判定失效。
        const int cwMax = cw->maximumSize().width();
        if (cwMax < QWIDGETSIZE_MAX)
            cwMin = qMin(cwMin, cwMax);
        w += cwMin;
        ++n;
    }
    const QMargins mg = m_functionLayout->contentsMargins();
    w += qMax(0, n - 1) * m_functionLayout->spacing();
    w += mg.left() + mg.right();
    return qMax(w, 1);
}

QSize FT_Function::sizeHint() const
{
    int inner = kFHintMinHeight;
    for (int i = 0; i < m_functionLayout->count(); ++i) {
        QWidget* cw = m_functionLayout->itemAt(i)->widget();
        if (!cw)
            continue;
        inner = qMax(inner, qMax(cw->minimumHeight(), cw->minimumSizeHint().height()));
    }
    const QMargins mg = m_functionLayout->contentsMargins();
    const int h = qMax(kFunctionDefaultHeight, inner + mg.top() + mg.bottom());
    return QSize(packMinimumWidth(), h);
}

QSize FT_Function::minimumSizeHint() const
{
    return sizeHint();
}

void FT_Function::updateFunctionHeight()
{
    const int h = sizeHint().height();
    if (h != minimumHeight()) {
        setMinimumHeight(h);
        updateGeometry();
        emit requestResize();
    }
}

void FT_Function::installCommandMenuFilters()
{
    installEventFilter(this);
    const QList<QWidget*> kids = findChildren<QWidget*>();
    for (QWidget* w : kids)
        w->installEventFilter(this);
}

bool FT_Function::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::ContextMenu) {
        // 文本/行输入控件保留系统原生右键菜单(复制、粘贴等)
        if (qobject_cast<QTextEdit*>(watched) || qobject_cast<QLineEdit*>(watched))
            return QWidget::eventFilter(watched, event);
        auto* ce = static_cast<QContextMenuEvent*>(event);
        showCommandMenu(ce->globalPos());
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void FT_Function::contextMenuEvent(QContextMenuEvent* event)
{
    showCommandMenu(event->globalPos());
}

void FT_Function::showCommandMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    // 命令区填满组后没有空白可右键,插入入口也放在命令菜单里:
    // 新命令插到本命令之后、同一视觉行(放不下会自动软折行)。
    QMenu* insertMenu = menu.addMenu(
        style()->standardIcon(QStyle::SP_FileIcon),
        tr("Insert Command After"));
    QAction* insTbox  = insertMenu->addAction(tr("TBox Command"));
    QAction* insIicW  = insertMenu->addAction(tr("I2C Write"));
    QAction* insIicWR = insertMenu->addAction(tr("I2C Write+Read"));
    menu.addSeparator();

    QAction* advance = menu.addAction(
        style()->standardIcon(QStyle::SP_FileDialogDetailedView),
        tr("Response Advance..."));
    menu.addSeparator();
    QAction* wrap = menu.addAction(tr("Wrap to New Line"));
    QAction* unwrap = menu.addAction(tr("Join Previous Line"));
    QAction* split = menu.addAction(tr("Split into New Step"));
    wrap->setEnabled(m_menuState.canWrap);
    unwrap->setEnabled(m_menuState.canUnwrap);
    split->setEnabled(m_menuState.canSplitStep);
    menu.addSeparator();
    QAction* up = menu.addAction(
        style()->standardIcon(QStyle::SP_ArrowUp),
        tr("Move Up"));
    QAction* down = menu.addAction(
        style()->standardIcon(QStyle::SP_ArrowDown),
        tr("Move Down"));
    up->setEnabled(m_menuState.canMoveUp);
    down->setEnabled(m_menuState.canMoveDown);
    menu.addSeparator();
    QAction* del = menu.addAction(
        style()->standardIcon(QStyle::SP_TrashIcon),
        tr("Remove"));

    QAction* chosen = menu.exec(globalPos);
    if (!chosen)
        return;
    if (chosen == insTbox)
        emit requestInsertAfter(QString::fromLatin1(kFtTypeTbox));
    else if (chosen == insIicW)
        emit requestInsertAfter(QString::fromLatin1(kFtTypeIicWrite));
    else if (chosen == insIicWR)
        emit requestInsertAfter(QString::fromLatin1(kFtTypeIicWriteRead));
    else if (chosen == advance)
        openResponseAdvance();
    else if (chosen == wrap)
        emit requestWrap();
    else if (chosen == unwrap)
        emit requestUnwrap();
    else if (chosen == split)
        emit requestSplitNewStep();
    else if (chosen == up)
        emit requestMoveUp();
    else if (chosen == down)
        emit requestMoveDown();
    else if (chosen == del)
        emit requestRemove();
}

FT_Function* FtFunctionFactory::create(const QString& typeName)
{
    if (typeName == QLatin1String("TBoxCommand"))
        return new FT_TboxFunction();
    if (typeName == QLatin1String("IicWrite"))
        return new FT_IicWriteFunction();
    if (typeName == QLatin1String("IicWriteRead"))
        return new FT_IicWriteReadFunction();
    return nullptr;
}

QString FtFunctionFactory::defaultTypeName()
{
    return QStringLiteral("TBoxCommand");
}

class FT_AdvanceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FT_AdvanceDialog(QWidget* parent = nullptr);

    void             setConfig(const FT_AdvanceConfig& cfg);
    FT_AdvanceConfig getConfig() const;

private slots:
    void onModeChanged(int index);
    void onAccepted();

private:
    void buildUi();
    static QString normalizeHex(const QString& raw);

    QComboBox*      m_toResult  = nullptr;
    QWidget*        m_dependentGroup = nullptr;
    QStackedWidget* m_targetStack = nullptr;
    QTextEdit*      m_targetSingle = nullptr;
    QLineEdit*      m_targetMin    = nullptr;
    QLineEdit*      m_targetMax    = nullptr;
    QTextEdit*      m_passMessage = nullptr;
    QTextEdit*      m_failMessage = nullptr;
    QComboBox*      m_failTo     = nullptr;
};

FT_AdvanceDialog::FT_AdvanceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Response Advance"));
    buildUi();
}

void FT_AdvanceDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(10);

    auto addField = [](QVBoxLayout* parent, const QString& title, QWidget* editor) {
        auto* box = new QVBoxLayout();
        box->setContentsMargins(0, 0, 0, 0);
        box->setSpacing(2);
        auto* label = new QLabel(title);
        QFont lf = label->font();
        lf.setBold(true);
        label->setFont(lf);
        box->addWidget(label);
        box->addWidget(editor);
        parent->addLayout(box);
    };

    m_toResult = new QComboBox(this);
    m_toResult->addItem(tr("DontCare"),  static_cast<int>(FtResultMode::DontCare));
    m_toResult->addItem(tr("Equal"),     static_cast<int>(FtResultMode::Equal));
    m_toResult->addItem(tr("Range"),     static_cast<int>(FtResultMode::Range));
    m_toResult->addItem(tr("Start With"),static_cast<int>(FtResultMode::StartWith));
    addField(root, tr("ToResult"), m_toResult);

    m_dependentGroup = new QWidget(this);
    auto* depLay = new QVBoxLayout(m_dependentGroup);
    depLay->setContentsMargins(0, 0, 0, 0);
    depLay->setSpacing(10);

    m_targetStack = new QStackedWidget(m_dependentGroup);

    auto* singlePage = new QWidget(m_targetStack);
    auto* singleLay = new QVBoxLayout(singlePage);
    singleLay->setContentsMargins(0, 0, 0, 0);
    m_targetSingle = new QTextEdit(singlePage);
    m_targetSingle->setPlaceholderText(tr("Hex bytes separated by spaces, e.g. 22 66"));
    m_targetSingle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_targetSingle->setMinimumHeight(40);
    m_targetSingle->setLineWrapMode(QTextEdit::WidgetWidth);
    singleLay->addWidget(m_targetSingle);
    m_targetStack->addWidget(singlePage);

    auto* rangePage = new QWidget(m_targetStack);
    auto* rangeLay = new QHBoxLayout(rangePage);
    rangeLay->setContentsMargins(0, 0, 0, 0);
    m_targetMin = new QLineEdit(rangePage);
    m_targetMax = new QLineEdit(rangePage);
    m_targetMin->setPlaceholderText(tr("Min hex"));
    m_targetMax->setPlaceholderText(tr("Max hex"));
    rangeLay->addWidget(new QLabel(tr("Min"), rangePage));
    rangeLay->addWidget(m_targetMin, 1);
    rangeLay->addWidget(new QLabel(tr("Max"), rangePage));
    rangeLay->addWidget(m_targetMax, 1);
    m_targetStack->addWidget(rangePage);

    addField(depLay, tr("Target"), m_targetStack);

    m_passMessage = new QTextEdit(m_dependentGroup);
    m_failMessage = new QTextEdit(m_dependentGroup);
    m_passMessage->setPlaceholderText(tr("Message shown on pass"));
    m_failMessage->setPlaceholderText(tr("Message shown on fail"));
    m_passMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_failMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_passMessage->setMinimumHeight(40);
    m_failMessage->setMinimumHeight(40);
    m_passMessage->setLineWrapMode(QTextEdit::WidgetWidth);
    m_failMessage->setLineWrapMode(QTextEdit::WidgetWidth);
    addField(depLay, tr("Pass Message"), m_passMessage);
    addField(depLay, tr("Fail Message"), m_failMessage);

    m_failTo = new QComboBox(m_dependentGroup);
    m_failTo->addItem(tr("Stop"),          static_cast<int>(FtFailAction::Stop));
    m_failTo->addItem(tr("Continue This"), static_cast<int>(FtFailAction::ContinueThis));
    m_failTo->addItem(tr("Continue All"),  static_cast<int>(FtFailAction::ContinueAll));
    addField(depLay, tr("FailTo"), m_failTo);

    root->addWidget(m_dependentGroup);
    root->addStretch();

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(m_toResult, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FT_AdvanceDialog::onModeChanged);
    connect(buttons, &QDialogButtonBox::accepted, this, &FT_AdvanceDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setMinimumWidth(300);
    onModeChanged(m_toResult->currentIndex());
    adjustSize();
}

void FT_AdvanceDialog::onModeChanged(int index)
{
    const auto mode = static_cast<FtResultMode>(
        m_toResult->itemData(index).toInt());
    m_targetStack->setCurrentIndex(mode == FtResultMode::Range ? 1 : 0);
    m_dependentGroup->setVisible(mode != FtResultMode::DontCare);
    layout()->invalidate();
}

QString FT_AdvanceDialog::normalizeHex(const QString& raw)
{
    const QStringList parts = raw.trimmed().split(
        QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    QStringList up;
    up.reserve(parts.size());
    for (const QString& p : parts)
        up << p.toUpper();
    return up.join(QLatin1Char(' '));
}

void FT_AdvanceDialog::setConfig(const FT_AdvanceConfig& cfg)
{
    const int modeIdx = m_toResult->findData(static_cast<int>(cfg.toResult));
    m_toResult->setCurrentIndex(modeIdx >= 0 ? modeIdx : 0);

    if (cfg.toResult == FtResultMode::Range) {
        m_targetMin->setText(cfg.targetMin);
        m_targetMax->setText(cfg.targetMax);
    } else {
        m_targetSingle->setPlainText(cfg.target);
    }

    m_passMessage->setPlainText(cfg.passMessage);
    m_failMessage->setPlainText(cfg.failMessage);
    const int failIdx = m_failTo->findData(static_cast<int>(cfg.failTo));
    m_failTo->setCurrentIndex(failIdx >= 0 ? failIdx : 0);

    onModeChanged(m_toResult->currentIndex());
}

FT_AdvanceConfig FT_AdvanceDialog::getConfig() const
{
    FT_AdvanceConfig cfg;
    cfg.toResult = static_cast<FtResultMode>(
        m_toResult->currentData().toInt());

    if (cfg.toResult == FtResultMode::Range) {
        cfg.targetMin = normalizeHex(m_targetMin->text());
        cfg.targetMax = normalizeHex(m_targetMax->text());
        cfg.target.clear();
    } else {
        cfg.target = normalizeHex(m_targetSingle->toPlainText());
        cfg.targetMin.clear();
        cfg.targetMax.clear();
    }

    cfg.passMessage = m_passMessage->toPlainText().trimmed();
    cfg.failMessage = m_failMessage->toPlainText().trimmed();
    cfg.failTo = static_cast<FtFailAction>(m_failTo->currentData().toInt());
    return cfg;
}

void FT_AdvanceDialog::onAccepted()
{
    const auto mode = static_cast<FtResultMode>(m_toResult->currentData().toInt());
    if (mode == FtResultMode::Range) {
        const QString minRaw = m_targetMin->text();
        const QString maxRaw = m_targetMax->text();
        if (minRaw.trimmed().isEmpty() || maxRaw.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Range requires both Min and Max hex values."));
            return;
        }
        const QStringList minToks = ftSplitWs(minRaw);
        const QStringList maxToks = ftSplitWs(maxRaw);
        if (!ftAllHexByteTokens(minToks) || !ftAllHexByteTokens(maxToks)) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Min and Max must be hex bytes separated by spaces (e.g. 22 66)."));
            return;
        }
        const QByteArray minBytes = ftHexBytesToArray(
            normalizeHex(minRaw).split(QLatin1Char(' '), Qt::SkipEmptyParts));
        const QByteArray maxBytes = ftHexBytesToArray(
            normalizeHex(maxRaw).split(QLatin1Char(' '), Qt::SkipEmptyParts));
        if (minBytes.size() != maxBytes.size()) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Min and Max must have the same number of bytes."));
            return;
        }
    }
    accept();
}

FT_TboxFunction::FT_TboxFunction(QWidget* parent)
    : FT_Function(parent)
{
    setObjectName(QStringLiteral("ftTboxFunction"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(kFunctionDefaultHeight);
    setStyleSheet(
        "QWidget#ftTboxFunction {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 3px;"
        "}");

    m_combo = new FHintComboBox(tr("TBox Command"), this);
    m_combo->setFixedWidth(160);
    m_combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_combo->addItems({
        QStringLiteral("AT+CGSN"),
        QStringLiteral("AT+CFUN?"),
        QStringLiteral("AT+COPS?"),
        QStringLiteral("AT+CSQ"),
        QStringLiteral("AT$QCRMCALL?"),
        QStringLiteral("TBox_Reset"),
        QStringLiteral("TBox_Version")
    });
    m_originalComboCount = m_combo->count();

    m_payload = new FHintTextEdit(tr("Payload"), this);
    m_payload->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_payload->setPlaceholderText(tr("Hex bytes separated by spaces, e.g. 22 66"));

    m_advance = new QLabel(this);
    m_advance->setFixedWidth(80);
    m_advance->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_advance->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QPalette advancePal = m_advance->palette();
    advancePal.setColor(QPalette::WindowText, QColor(0x1B, 0x7A, 0x3A));
    m_advance->setPalette(advancePal);

    auto* layout = functionLayout();
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(6);
    layout->addWidget(m_combo,   0);
    layout->addWidget(m_payload, 1);
    layout->addWidget(m_advance, 0);

    connect(m_combo, &FHintComboBox::currentTextChanged,
            this, &FT_Function::contentChanged);
    connect(m_payload, &FHintTextEdit::textChanged,
            this, &FT_Function::contentChanged);

    connect(m_payload, &FHintTextEdit::heightChanged, this, [this](int) {
        updateFunctionHeight();
    });

    installCommandMenuFilters();

    updateAdvanceLabel();
    updateFunctionHeight();
}

FT_FunctionData FT_TboxFunction::toConfig() const
{
    FT_TboxConfig cfg;
    cfg.command = m_combo->currentText();
    cfg.payload = m_payload->toPlainText();
    cfg.advance = m_advanceCfg;
    return cfg;
}

void FT_TboxFunction::applyConfig(const FT_FunctionData& data)
{
    if (!std::holds_alternative<FT_TboxConfig>(data))
        return;

    while (m_combo->count() > m_originalComboCount)
        m_combo->removeItem(m_combo->count() - 1);

    const FT_TboxConfig& t = std::get<FT_TboxConfig>(data);
    if (int idx = m_combo->findText(t.command); idx >= 0) {
        m_combo->setCurrentIndex(idx);
    } else if (!t.command.isEmpty()) {
        m_combo->insertItem(0, t.command);
        m_combo->setCurrentIndex(0);
    }
    m_payload->setPlainText(t.payload);

    m_advanceCfg = t.advance;
    updateAdvanceLabel();
}

QString FT_TboxFunction::resultModeText(FtResultMode mode)
{
    switch (mode) {
    case FtResultMode::Equal:     return tr("Equal");
    case FtResultMode::Range:     return tr("Range");
    case FtResultMode::StartWith: return tr("Start With");
    case FtResultMode::DontCare:  default: return tr("DontCare");
    }
}

void FT_TboxFunction::updateAdvanceLabel()
{
    m_advance->setText(resultModeText(m_advanceCfg.toResult));
}

void FT_TboxFunction::openResponseAdvance()
{
    FT_AdvanceDialog dlg;
    dlg.setConfig(m_advanceCfg);
    if (dlg.exec() == QDialog::Accepted) {
        m_advanceCfg = dlg.getConfig();
        updateAdvanceLabel();
        emit contentChanged();
    }
}

FT_IicWriteFunction::FT_IicWriteFunction(QWidget* parent)
    : FT_Function(parent)
{
    setObjectName(QStringLiteral("ftIicWriteFunction"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(kFunctionDefaultHeight);
    setStyleSheet(
        "QWidget#ftIicWriteFunction {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 3px;"
        "}");

    m_port = new FHintSpinBox(tr("Port"), this);
    m_port->setFixedWidth(60);
    m_port->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_port->setRange(0, 255);

    m_reg = new FHintTextEdit(tr("Reg"), this);
    m_reg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_reg->setPlaceholderText(tr("Hex bytes, e.g. 22 66"));

    m_payload = new FHintTextEdit(tr("Payload"), this);
    m_payload->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_payload->setPlaceholderText(tr("Hex bytes separated by spaces"));

    m_advance = new QLabel(this);
    m_advance->setFixedWidth(80);
    m_advance->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_advance->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QPalette pal = m_advance->palette();
    pal.setColor(QPalette::WindowText, QColor(0x1B, 0x7A, 0x3A));
    m_advance->setPalette(pal);

    auto* layout = functionLayout();
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(6);
    layout->addWidget(m_port,    0);
    layout->addWidget(m_reg,     1);
    layout->addWidget(m_payload, 1);
    layout->addWidget(m_advance, 0);

    connect(m_port,    QOverload<int>::of(&FHintSpinBox::valueChanged),
            this, &FT_Function::contentChanged);
    connect(m_reg,     &FHintTextEdit::textChanged,
            this, &FT_Function::contentChanged);
    connect(m_payload, &FHintTextEdit::textChanged,
            this, &FT_Function::contentChanged);

    connect(m_reg,     &FHintTextEdit::heightChanged, this, [this](int) { updateFunctionHeight(); });
    connect(m_payload, &FHintTextEdit::heightChanged, this, [this](int) { updateFunctionHeight(); });

    installCommandMenuFilters();

    updateAdvanceLabel();
    updateFunctionHeight();
}

FT_FunctionData FT_IicWriteFunction::toConfig() const
{
    FT_IicWriteConfig cfg;
    cfg.port    = m_port->value();
    cfg.reg     = m_reg->toPlainText();
    cfg.payload = m_payload->toPlainText();
    cfg.advance = m_advanceCfg;
    return cfg;
}

void FT_IicWriteFunction::applyConfig(const FT_FunctionData& data)
{
    if (!std::holds_alternative<FT_IicWriteConfig>(data))
        return;

    const FT_IicWriteConfig& cfg = std::get<FT_IicWriteConfig>(data);
    m_port->setValue(cfg.port);
    m_reg->setPlainText(cfg.reg);
    m_payload->setPlainText(cfg.payload);

    m_advanceCfg = cfg.advance;
    updateAdvanceLabel();
}

QString FT_IicWriteFunction::resultModeText(FtResultMode mode)
{
    switch (mode) {
    case FtResultMode::Equal:     return tr("Equal");
    case FtResultMode::Range:     return tr("Range");
    case FtResultMode::StartWith: return tr("Start With");
    case FtResultMode::DontCare:  default: return tr("DontCare");
    }
}

void FT_IicWriteFunction::updateAdvanceLabel()
{
    m_advance->setText(resultModeText(m_advanceCfg.toResult));
}

void FT_IicWriteFunction::openResponseAdvance()
{
    FT_AdvanceDialog dlg;
    dlg.setConfig(m_advanceCfg);
    if (dlg.exec() == QDialog::Accepted) {
        m_advanceCfg = dlg.getConfig();
        updateAdvanceLabel();
        emit contentChanged();
    }
}

FT_IicWriteReadFunction::FT_IicWriteReadFunction(QWidget* parent)
    : FT_Function(parent)
{
    setObjectName(QStringLiteral("ftIicWriteReadFunction"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(kFunctionDefaultHeight);
    setStyleSheet(
        "QWidget#ftIicWriteReadFunction {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 3px;"
        "}");

    m_port = new FHintSpinBox(tr("Port"), this);
    m_port->setFixedWidth(60);
    m_port->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_port->setRange(0, 255);

    m_reg = new FHintTextEdit(tr("Reg"), this);
    m_reg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_reg->setPlaceholderText(tr("Hex bytes, e.g. 22 66"));

    m_payload = new FHintTextEdit(tr("Payload"), this);
    m_payload->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_payload->setPlaceholderText(tr("Hex bytes separated by spaces"));

    m_readLength = new FHintSpinBox(tr("Read Len"), this);
    m_readLength->setFixedWidth(70);
    m_readLength->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_readLength->setRange(1, 256);
    m_readLength->setValue(1);

    m_advance = new QLabel(this);
    m_advance->setFixedWidth(80);
    m_advance->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_advance->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QPalette pal = m_advance->palette();
    pal.setColor(QPalette::WindowText, QColor(0x1B, 0x7A, 0x3A));
    m_advance->setPalette(pal);

    auto* layout = functionLayout();
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(6);
    layout->addWidget(m_port,       0);
    layout->addWidget(m_reg,        1);
    layout->addWidget(m_payload,    1);
    layout->addWidget(m_readLength, 0);
    layout->addWidget(m_advance,    0);

    connect(m_port,       QOverload<int>::of(&FHintSpinBox::valueChanged),
            this, &FT_Function::contentChanged);
    connect(m_reg,        &FHintTextEdit::textChanged,
            this, &FT_Function::contentChanged);
    connect(m_payload,    &FHintTextEdit::textChanged,
            this, &FT_Function::contentChanged);
    connect(m_readLength, QOverload<int>::of(&FHintSpinBox::valueChanged),
            this, &FT_Function::contentChanged);

    connect(m_reg,        &FHintTextEdit::heightChanged, this, [this](int) { updateFunctionHeight(); });
    connect(m_payload,    &FHintTextEdit::heightChanged, this, [this](int) { updateFunctionHeight(); });

    installCommandMenuFilters();

    updateAdvanceLabel();
    updateFunctionHeight();
}

FT_FunctionData FT_IicWriteReadFunction::toConfig() const
{
    FT_IicWriteReadConfig cfg;
    cfg.port       = m_port->value();
    cfg.reg        = m_reg->toPlainText();
    cfg.payload    = m_payload->toPlainText();
    cfg.readLength = m_readLength->value();
    cfg.advance    = m_advanceCfg;
    return cfg;
}

void FT_IicWriteReadFunction::applyConfig(const FT_FunctionData& data)
{
    if (!std::holds_alternative<FT_IicWriteReadConfig>(data))
        return;

    const FT_IicWriteReadConfig& cfg = std::get<FT_IicWriteReadConfig>(data);
    m_port->setValue(cfg.port);
    m_reg->setPlainText(cfg.reg);
    m_payload->setPlainText(cfg.payload);
    m_readLength->setValue(cfg.readLength);

    m_advanceCfg = cfg.advance;
    updateAdvanceLabel();
}

QString FT_IicWriteReadFunction::resultModeText(FtResultMode mode)
{
    switch (mode) {
    case FtResultMode::Equal:     return tr("Equal");
    case FtResultMode::Range:     return tr("Range");
    case FtResultMode::StartWith: return tr("Start With");
    case FtResultMode::DontCare:  default: return tr("DontCare");
    }
}

void FT_IicWriteReadFunction::updateAdvanceLabel()
{
    m_advance->setText(resultModeText(m_advanceCfg.toResult));
}

void FT_IicWriteReadFunction::openResponseAdvance()
{
    FT_AdvanceDialog dlg;
    dlg.setConfig(m_advanceCfg);
    if (dlg.exec() == QDialog::Accepted) {
        m_advanceCfg = dlg.getConfig();
        updateAdvanceLabel();
        emit contentChanged();
    }
}

#include "FT_Function.moc"