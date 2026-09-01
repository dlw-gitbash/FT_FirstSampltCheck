#include "FT_TBoxFunctionWidget.h"
#include "FT_AdvanceDialog.h"
#include "FT_ComBox.h"
#include "FT_LineEdit.h"
#include "FT_TextEdit.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTextDocument>
#include <QShowEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIntValidator>

namespace {
constexpr int kMinPayloadHeight = 56;
constexpr int kLayoutVMargin    = 16;
constexpr int kCommandWidth     = 140;
constexpr int kDelayWidth       = 120;
}

static int calcPayloadHeight(FT_TextEdit *te)
{
    if (!te) return kMinPayloadHeight;
    QTextDocument *doc = te->document();
    if (!doc) return kMinPayloadHeight;

    int vpW = te->viewport()->width();
    if (vpW <= 0) {
        const int approx = te->width() - 16;
        vpW = approx > 0 ? approx : 200;
    }
    doc->setTextWidth(vpW);
    const int docH = static_cast<int>(doc->size().height());

    int h = docH + te->frameWidth() * 2 + 8;
    return qMax(kMinPayloadHeight, h);
}

FT_TBoxFunctionWidget::FT_TBoxFunctionWidget(QWidget *parent)
    : FT_FunctionWidget(QStringLiteral("TBox"), parent)
{
    m_commandEdit = new FT_ComBox(this);
    m_commandEdit->setHint(tr("Command"));
    m_commandEdit->setFixedWidth(kCommandWidth);
    m_commandEdit->setEditable(true);
    m_commandEdit->addItems({QStringLiteral("READ"),
                             QStringLiteral("WRITE"),
                             QStringLiteral("INIT")});

    m_payloadEdit = new FT_TextEdit(this);
    m_payloadEdit->setHint(tr("Payload"));
    m_payloadEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_payloadEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_payloadEdit->setMinimumHeight(kMinPayloadHeight);

    m_advanceLabel = new QLabel(tr("Advance"), this);
    m_advanceLabel->setFixedWidth(104);
    m_advanceLabel->setAlignment(Qt::AlignCenter);
    m_advanceLabel->setStyleSheet(QStringLiteral(
        "color: #666666; border: 1px solid #e5e5e5;"
        "border-radius: 4px; background: #fafafa;"));
    m_advanceLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_advanceLabel->setMinimumHeight(kMinPayloadHeight);

    m_delayEdit = new FT_LineEdit(this);
    m_delayEdit->setHint(tr("Delay"));
    m_delayEdit->setFixedWidth(kDelayWidth);
    m_delayEdit->setText(QStringLiteral("0"));
    m_delayEdit->setValidator(new QIntValidator(0, 999999, this));

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(10);
    root->addWidget(m_commandEdit);
    root->addWidget(m_payloadEdit, 1);
    root->addWidget(m_advanceLabel);
    root->addWidget(m_delayEdit);

    constexpr int kMinTBoxWidth = kCommandWidth + 10 + 320 + 10 + 104 + 10 + kDelayWidth + 16;
    setMinimumWidth(qMax(640, kMinTBoxWidth));
    setMinimumHeight(kMinPayloadHeight + kLayoutVMargin);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_payloadTextHeight = kMinPayloadHeight;
    m_payloadEdit->setFixedHeight(kMinPayloadHeight);

    connect(m_payloadEdit, &FT_TextEdit::textChanged,
            this, &FT_TBoxFunctionWidget::adjustPayloadHeight);

    updateAdvanceSummary();
}

void FT_TBoxFunctionWidget::showEvent(QShowEvent *e)
{
    FT_FunctionWidget::showEvent(e);
    adjustPayloadHeight();
}

void FT_TBoxFunctionWidget::adjustPayloadHeight()
{
    const int newH = calcPayloadHeight(m_payloadEdit);
    if (newH != m_payloadTextHeight) {
        m_payloadTextHeight = newH;
        m_payloadEdit->setFixedHeight(newH);
        updateGeometry();
        emit sizeHintChanged();
    }
}

QSize FT_TBoxFunctionWidget::sizeHint() const
{
    const int wantH = m_payloadTextHeight + kLayoutVMargin;
    return QSize(qMax(minimumWidth(), 640),
                 qMax(kMinPayloadHeight + kLayoutVMargin, wantH));
}
QSize FT_TBoxFunctionWidget::minimumSizeHint() const { return sizeHint(); }

QString FT_TBoxFunctionWidget::command() const
{
    return m_commandEdit->currentText().trimmed();
}

void FT_TBoxFunctionWidget::setCommand(const QString &cmd)
{
    m_commandEdit->setCurrentText(cmd);
}

QString FT_TBoxFunctionWidget::payload() const
{
    return m_payloadEdit->toPlainText();
}

void FT_TBoxFunctionWidget::setPayload(const QString &p)
{
    m_payloadEdit->setText(p);
}

int FT_TBoxFunctionWidget::delay() const
{
    return m_delayEdit->text().toInt();
}

void FT_TBoxFunctionWidget::setDelay(int ms)
{
    m_delayEdit->setText(QString::number(ms));
}

FT_AdvanceDialog::CompareMode FT_TBoxFunctionWidget::compareMode() const { return m_mode; }
QString FT_TBoxFunctionWidget::compareParam() const                   { return m_param; }
QString FT_TBoxFunctionWidget::passMessage() const                    { return m_passMsg; }
QString FT_TBoxFunctionWidget::failMessage() const                    { return m_failMsg; }
FT_AdvanceDialog::FailAction FT_TBoxFunctionWidget::failAction() const { return m_failAction; }

void FT_TBoxFunctionWidget::responseAdvance()
{
    FT_AdvanceDialog dlg(this);
    dlg.setCompareMode(m_mode);
    dlg.setCompareParam(m_param);
    dlg.setPassMessage(m_passMsg);
    dlg.setFailMessage(m_failMsg);
    dlg.setFailAction(m_failAction);

    if (dlg.exec() == QDialog::Accepted) {
        m_mode       = dlg.compareMode();
        m_param      = dlg.compareParam();
        m_passMsg    = dlg.passMessage();
        m_failMsg    = dlg.failMessage();
        m_failAction = dlg.failAction();
        updateAdvanceSummary();
    }
}

void FT_TBoxFunctionWidget::updateAdvanceSummary()
{
    m_advanceLabel->setToolTip(
        tr("处理:%1  比较参数:%2\nPass:%3\nFail:%4\n失败动作:%5\n（右键 → Response Advance 编辑）")
            .arg(FT_AdvanceDialog::compareModeToString(m_mode))
            .arg(m_param.isEmpty() ? tr("(无)") : m_param)
            .arg(m_passMsg.isEmpty() ? tr("(无)") : m_passMsg)
            .arg(m_failMsg.isEmpty() ? tr("(无)") : m_failMsg)
            .arg(FT_AdvanceDialog::failActionToString(m_failAction)));
}

QString FT_TBoxFunctionWidget::toConfig() const
{
    QJsonObject obj;
    obj["type"]    = functionType();
    obj["command"] = command();
    obj["payload"] = payload();
    obj["delay"]   = delay();

    QJsonObject adv;
    adv["mode"]   = FT_AdvanceDialog::compareModeToString(m_mode);
    adv["param"]  = m_param;
    adv["pass"]   = m_passMsg;
    adv["fail"]   = m_failMsg;
    adv["action"] = FT_AdvanceDialog::failActionToString(m_failAction);
    obj["advance"] = adv;

    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

void FT_TBoxFunctionWidget::fromConfig(const QString &cfg)
{
    const QJsonObject obj = QJsonDocument::fromJson(cfg.toUtf8()).object();

    setCommand(obj["command"].toString());
    setPayload(obj["payload"].toString());
    setDelay(obj["delay"].toInt(0));

    const QJsonObject adv = obj["advance"].toObject();
    FT_AdvanceDialog::CompareMode m;
    if (FT_AdvanceDialog::compareModeFromString(adv["mode"].toString(), &m))
        m_mode = m;
    m_param   = adv["param"].toString();
    m_passMsg = adv["pass"].toString();
    m_failMsg = adv["fail"].toString();
    FT_AdvanceDialog::FailAction a;
    if (FT_AdvanceDialog::failActionFromString(adv["action"].toString(), &a))
        m_failAction = a;

    updateAdvanceSummary();
}