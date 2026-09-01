#include "FT_AdvanceDialog.h"

#include <QComboBox>
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>

FT_AdvanceDialog::FT_AdvanceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Advance - 高级配置"));
    setMinimumWidth(460);

    auto *root = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_modeCombo = new QComboBox(this);
    using FT_Core::CompareMode;
    m_modeCombo->addItem(FT_Core::compareModeToString(CompareMode::DontCare));
    m_modeCombo->addItem(FT_Core::compareModeToString(CompareMode::Equal));
    m_modeCombo->addItem(FT_Core::compareModeToString(CompareMode::contains));
    m_modeCombo->addItem(FT_Core::compareModeToString(CompareMode::uncontains));
    m_modeCombo->addItem(FT_Core::compareModeToString(CompareMode::startswith));
    m_modeCombo->addItem(FT_Core::compareModeToString(CompareMode::endswith));
    form->addRow(tr("处理方式:"), m_modeCombo);

    m_paramEdit = new QTextEdit(this);
    m_paramEdit->setPlaceholderText(tr("hex 格式、空格分隔，如：AA 0B 1F 3C"));
    m_paramEdit->setFixedHeight(56);
    m_paramEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    form->addRow(tr("比较参数:"), m_paramEdit);

    m_passEdit = new QTextEdit(this);
    m_passEdit->setPlaceholderText(tr("处理结果为 pass 时打印的内容..."));
    m_passEdit->setFixedHeight(56);
    m_passEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    form->addRow(tr("Pass Message:"), m_passEdit);

    m_failEdit = new QTextEdit(this);
    m_failEdit->setPlaceholderText(tr("处理结果为 NG 时打印的内容..."));
    m_failEdit->setFixedHeight(56);
    m_failEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    form->addRow(tr("Fail Message:"), m_failEdit);

    m_failCombo = new QComboBox(this);
    using FT_Core::FailAction;
    m_failCombo->addItem(FT_Core::failActionToString(FailAction::Stop));
    m_failCombo->addItem(FT_Core::failActionToString(FailAction::RestartThis));
    m_failCombo->addItem(FT_Core::failActionToString(FailAction::RestartAll));
    form->addRow(tr("失败处理:"), m_failCombo);

    root->addLayout(form);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &FT_AdvanceDialog::onOkClicked);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setCompareMode(CompareMode::DontCare);
    setFailAction(FailAction::Stop);
}

FT_AdvanceDialog::CompareMode FT_AdvanceDialog::compareMode() const
{
    using FT_Core::CompareMode;
    static const CompareMode kOrder[] = {
        CompareMode::DontCare,   CompareMode::Equal,    CompareMode::contains,
        CompareMode::uncontains, CompareMode::startswith, CompareMode::endswith
    };
    const int idx = m_modeCombo->currentIndex();
    if (idx >= 0 && idx < static_cast<int>(sizeof(kOrder) / sizeof(kOrder[0])))
        return kOrder[idx];
    return CompareMode::DontCare;
}

void FT_AdvanceDialog::setCompareMode(CompareMode m)
{
    using FT_Core::CompareMode;
    static const CompareMode kOrder[] = {
        CompareMode::DontCare,   CompareMode::Equal,    CompareMode::contains,
        CompareMode::uncontains, CompareMode::startswith, CompareMode::endswith
    };
    for (int i = 0; i < static_cast<int>(sizeof(kOrder) / sizeof(kOrder[0])); ++i) {
        if (kOrder[i] == m) { m_modeCombo->setCurrentIndex(i); return; }
    }
    m_modeCombo->setCurrentIndex(0);
}

QString FT_AdvanceDialog::compareParam() const
{
    return m_paramEdit->toPlainText().trimmed();
}

void FT_AdvanceDialog::setCompareParam(const QString &hexText)
{
    m_paramEdit->setPlainText(hexText);
}

QString FT_AdvanceDialog::passMessage() const
{
    return m_passEdit->toPlainText();
}

void FT_AdvanceDialog::setPassMessage(const QString &s)
{
    m_passEdit->setPlainText(s);
}

QString FT_AdvanceDialog::failMessage() const
{
    return m_failEdit->toPlainText();
}

void FT_AdvanceDialog::setFailMessage(const QString &s)
{
    m_failEdit->setPlainText(s);
}

FT_AdvanceDialog::FailAction FT_AdvanceDialog::failAction() const
{
    using FT_Core::FailAction;
    static const FailAction kOrder[] = {
        FailAction::Stop, FailAction::RestartThis, FailAction::RestartAll
    };
    const int idx = m_failCombo->currentIndex();
    if (idx >= 0 && idx < static_cast<int>(sizeof(kOrder) / sizeof(kOrder[0])))
        return kOrder[idx];
    return FailAction::Stop;
}

void FT_AdvanceDialog::setFailAction(FailAction a)
{
    using FT_Core::FailAction;
    static const FailAction kOrder[] = {
        FailAction::Stop, FailAction::RestartThis, FailAction::RestartAll
    };
    for (int i = 0; i < static_cast<int>(sizeof(kOrder) / sizeof(kOrder[0])); ++i) {
        if (kOrder[i] == a) { m_failCombo->setCurrentIndex(i); return; }
    }
    m_failCombo->setCurrentIndex(0);
}

void FT_AdvanceDialog::onOkClicked()
{
    const QString param = compareParam();
    if (compareMode() != CompareMode::DontCare && !param.isEmpty() && !isValidHexParam(param)) {
        QMessageBox::warning(this, tr("参数错误"),
                             tr("比较参数需为 hex 格式并以空格分隔，例如：AA 0B 1F 3C"));
        return;
    }
    accept();
}

QString FT_AdvanceDialog::toConfig() const
{
    QJsonObject obj;
    obj["mode"]   = compareModeToString(compareMode());
    obj["param"]  = compareParam();
    obj["pass"]   = passMessage();
    obj["fail"]   = failMessage();
    obj["action"] = failActionToString(failAction());
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

void FT_AdvanceDialog::fromConfig(const QString &cfg)
{
    const QJsonObject obj = QJsonDocument::fromJson(cfg.toUtf8()).object();
    CompareMode m;
    if (compareModeFromString(obj["mode"].toString(), &m))
        setCompareMode(m);
    setCompareParam(obj["param"].toString());
    setPassMessage(obj["pass"].toString());
    setFailMessage(obj["fail"].toString());
    FailAction a;
    if (failActionFromString(obj["action"].toString(), &a))
        setFailAction(a);
}