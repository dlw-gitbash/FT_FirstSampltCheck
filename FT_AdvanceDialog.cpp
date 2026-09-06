#include "FT_AdvanceDialog.h"

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
    setFixedSize(300, 400);
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

FT_AdvanceConfig FT_AdvanceDialog::config() const
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
        if (!ftAllHexByteTokens(minRaw, false) || !ftAllHexByteTokens(maxRaw, false)) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Min and Max must be hex bytes separated by spaces (e.g. 22 66)."));
            return;
        }
        const QByteArray minBytes = ftHexBytesToArray(normalizeHex(minRaw));
        const QByteArray maxBytes = ftHexBytesToArray(normalizeHex(maxRaw));
        if (minBytes.size() != maxBytes.size()) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Min and Max must have the same number of bytes."));
            return;
        }
        if (minBytes > maxBytes) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Min must not be greater than Max."));
            return;
        }
    } else if (mode != FtResultMode::DontCare) {
        const QString targetRaw = m_targetSingle->toPlainText();
        if (targetRaw.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Please enter the target hex value."));
            return;
        }
        if (!ftAllHexByteTokens(targetRaw, false)) {
            QMessageBox::warning(this, tr("Response Advance"),
                                 tr("Target must be hex bytes separated by spaces (e.g. 22 66)."));
            return;
        }
    }
    accept();
}