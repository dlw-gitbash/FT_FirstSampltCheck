#include "FT_FunctionWidgetItem.h"
#include "FT_FunctionWidget.h"
#include "FT_TBoxFunctionWidget.h"
#include "FT_TextEdit.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QContextMenuEvent>
#include <QDebug>
#include <utility>

namespace {
constexpr int kTitleHeight  = 72;
constexpr int kTitleWidth   = 240;
constexpr int kCheckWidth   = 20;
constexpr int kItemVMargin  = 8;
constexpr int kItemHMargin  = 8;
constexpr int kColSpacing   = 12;
constexpr int kSpacing      = 8;

constexpr int kTBoxMinWidth = 740;
constexpr int kItemMinWidth = kItemHMargin * 2 + kCheckWidth + kColSpacing + kTitleWidth + kColSpacing + kTBoxMinWidth;
constexpr int kLeftColWidth = kCheckWidth + 10 + kTitleWidth;
}

FT_FunctionWidgetItem::FT_FunctionWidgetItem(QWidget *parent)
    : QWidget(parent)
{
    m_enableCheck = new QCheckBox(this);
    m_enableCheck->setFixedWidth(kCheckWidth);
    m_enableCheck->setChecked(true);
    m_enableCheck->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_enableCheck->setToolTip(tr("是否启用本调度"));

    m_titleEdit = new FT_TextEdit(this);
    m_titleEdit->setHint(tr("标题"));
    m_titleEdit->setFixedSize(kTitleWidth, kTitleHeight);
    m_titleEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_titleEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_titleEdit->setAlignment(Qt::AlignCenter);

    auto *leftPanel = new QHBoxLayout;
    leftPanel->setContentsMargins(0, 0, 0, 0);
    leftPanel->setSpacing(10);
    leftPanel->setAlignment(Qt::AlignTop);
    leftPanel->addWidget(m_enableCheck, 0, Qt::AlignTop);
    leftPanel->addWidget(m_titleEdit, 0, Qt::AlignTop);
    leftPanel->addStretch(0);

    m_funcContainer = new QWidget(this);
    m_funcContainer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_funcLayout = new QVBoxLayout(m_funcContainer);
    m_funcLayout->setContentsMargins(0, 0, 0, 0);
    m_funcLayout->setSpacing(kSpacing);
    m_funcLayout->setAlignment(Qt::AlignTop);
    m_funcContainer->setObjectName(QStringLiteral("FuncContainer"));
    m_funcContainer->setMinimumWidth(kTBoxMinWidth);
    m_funcContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_funcContainer->setStyleSheet(QStringLiteral(
        "#FuncContainer { border: 1px dashed rgba(220,0,0,0.25); background: rgba(255,255,255,1); }"));

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(kItemHMargin, kItemVMargin, kItemHMargin, kItemVMargin);
    root->setSpacing(kColSpacing);
    root->addLayout(leftPanel, 0);
    root->addWidget(m_funcContainer, 1);
    root->setAlignment(Qt::AlignTop);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumWidth(kItemMinWidth);
    setMinimumHeight(kItemVMargin * 2 + kTitleHeight);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(255, 255, 255));
    setPalette(pal);

    connect(m_enableCheck, &QCheckBox::toggled, this,
            [this](bool on) { emit enabledChanged(on); });
    connect(m_titleEdit, &FT_TextEdit::textChanged, this,
            [this] { emit titleChanged(m_titleEdit->toPlainText()); });
    connect(m_funcContainer, &QWidget::customContextMenuRequested,
            this, &FT_FunctionWidgetItem::showFunclistMenu);

    QTimer::singleShot(0, this, [this] { addFunction(new FT_TBoxFunctionWidget); });
}

bool FT_FunctionWidgetItem::scheduleEnabled() const { return m_enableCheck->isChecked(); }
void FT_FunctionWidgetItem::setScheduleEnabled(bool on) { m_enableCheck->setChecked(on); }

QString FT_FunctionWidgetItem::title() const
{
    return m_titleEdit->toPlainText().trimmed();
}
void FT_FunctionWidgetItem::setTitle(const QString &t)
{
    if (t.isEmpty()) return;
    if (m_titleEdit->toPlainText().trimmed() == t) return;
    m_titleEdit->setPlainText(t);
}

void FT_FunctionWidgetItem::hookFunctionSize(FT_FunctionWidget *fw)
{
    connect(fw, &FT_FunctionWidget::addFunctionRequested, this, [this] {
        addFunction(new FT_TBoxFunctionWidget);
    });
    connect(fw, &FT_FunctionWidget::sizeHintChanged, this, [this] {
        if (m_funcLayout) m_funcLayout->invalidate();
        m_funcContainer->updateGeometry();
        updateGeometry();
        emit sizeHintChanged();
    });
}

void FT_FunctionWidgetItem::addFunction(FT_FunctionWidget *fw)
{
    if (!fw) return;

    const QSize fwHint = fw->sizeHint();
    const QSize fwMinHint = fw->minimumSizeHint();
    const int wantW = qMax(kTBoxMinWidth, qMax(fwHint.width(), fwMinHint.width()));
    const int wantH = qMax(72, qMax(fwHint.height(), fwMinHint.height()));
    fw->setMinimumSize(wantW, wantH);

    fw->setParent(m_funcContainer);
    m_functions.append(fw);
    m_funcLayout->addWidget(fw, 0, Qt::AlignTop);

    fw->setStyleSheet(QStringLiteral(
        ".FT_FunctionWidget { border: 2px dashed rgba(0,170,0,0.45); background: rgba(255,255,255,1); }"));

    hookFunctionSize(fw);

    m_funcLayout->invalidate();
    m_funcContainer->updateGeometry();
    m_funcContainer->update();
    updateGeometry();
    update();

    emit functionListChanged();
    emit sizeHintChanged();

    QTimer::singleShot(0, this, [this] {
        if (!m_funcContainer) return;
        m_funcLayout->invalidate();
        m_funcContainer->updateGeometry();
        updateGeometry();
        emit sizeHintChanged();
    });
}

void FT_FunctionWidgetItem::clearFunctions()
{
    while (!m_functions.isEmpty()) {
        FT_FunctionWidget *fw = m_functions.takeFirst();
        if (m_funcLayout) m_funcLayout->removeWidget(fw);
        delete fw;
    }
    if (m_funcContainer) {
        m_funcContainer->adjustSize();
        m_funcContainer->updateGeometry();
    }
    updateGeometry();
    emit functionListChanged();
    emit sizeHintChanged();
}

QList<FT_FunctionWidget *> FT_FunctionWidgetItem::functions() const { return m_functions; }

FT_FunctionWidget *FT_FunctionWidgetItem::functionAt(int row) const
{
    if (row < 0 || row >= m_functions.size()) return nullptr;
    return m_functions.at(row);
}

int FT_FunctionWidgetItem::functionCount() const { return m_functions.size(); }

FT_FunctionWidget *FT_FunctionWidgetItem::createFunction(const QString &type)
{
    if (type.compare(QLatin1String("TBox"), Qt::CaseInsensitive) == 0)
        return new FT_TBoxFunctionWidget;
    return nullptr;
}

void FT_FunctionWidgetItem::contextMenuEvent(QContextMenuEvent *e)
{
    const QPoint local = m_funcContainer->mapFromGlobal(e->globalPos());
    if (!m_funcContainer->rect().contains(local))
        emit contextMenuRequested(e->globalPos());
    e->accept();
}

void FT_FunctionWidgetItem::showFunclistMenu(const QPoint &pos)
{
    const QPoint globalPos = m_funcContainer->mapToGlobal(pos);

    FT_FunctionWidget *target = nullptr;

    for (FT_FunctionWidget *fw : std::as_const(m_functions)) {
        const QRect g = fw->geometry();
        if (g.contains(pos)) {
            target = fw;
            break;
        }
    }

    constexpr int kTol = 4;
    if (!target && !m_functions.isEmpty()) {
        FT_FunctionWidget *nearest = nullptr;
        int nearestDist = 1000000;
        for (FT_FunctionWidget *fw : std::as_const(m_functions)) {
            const QRect g = fw->geometry();
            int dx = 0, dy = 0;
            if      (pos.x() < g.left())   dx = g.left()   - pos.x();
            else if (pos.x() > g.right())  dx = pos.x()    - g.right();
            if      (pos.y() < g.top())    dy = g.top()    - pos.y();
            else if (pos.y() > g.bottom()) dy = pos.y()    - g.bottom();
            const int dist = dx + dy;
            if (dist < nearestDist) { nearestDist = dist; nearest = fw; }
        }
        if (nearest && nearestDist <= kTol) {
            target = nearest;
        }
    }

    QMenu menu(this);
    QAction *addAct  = menu.addAction(tr("Add TBox Function"));
    QAction *delAct  = nullptr;
    if (target) delAct = menu.addAction(tr("Delete This Function"));

    QAction *chosen = menu.exec(globalPos);

    if (chosen == addAct) {
        addFunction(new FT_TBoxFunctionWidget);
    } else if (delAct && chosen == delAct && target) {
        m_functions.removeOne(target);
        if (m_funcLayout) m_funcLayout->removeWidget(target);
        delete target;
        if (m_funcLayout) m_funcLayout->invalidate();
        if (m_funcContainer) {
            m_funcContainer->updateGeometry();
            m_funcContainer->update();
        }
        updateGeometry();
        update();
        emit functionListChanged();
        emit sizeHintChanged();
        QTimer::singleShot(0, this, [this] {
            if (!m_funcContainer) return;
            if (m_funcLayout) m_funcLayout->invalidate();
            m_funcContainer->updateGeometry();
            updateGeometry();
            emit sizeHintChanged();
        });
    }
}

QString FT_FunctionWidgetItem::toConfig() const
{
    QJsonObject obj;
    obj["enabled"] = scheduleEnabled();
    obj["title"]   = title();

    QJsonArray arr;
    for (const FT_FunctionWidget *fw : std::as_const(m_functions)) {
        const QJsonObject fwj = QJsonDocument::fromJson(
            fw->toConfig().toUtf8()).object();
        if (!fwj.isEmpty()) arr.append(fwj);
    }
    obj["functions"] = arr;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

void FT_FunctionWidgetItem::fromConfig(const QString &cfg)
{
    const QJsonObject obj = QJsonDocument::fromJson(cfg.toUtf8()).object();
    setScheduleEnabled(obj["enabled"].toBool(true));
    setTitle(obj["title"].toString());

    clearFunctions();
    const QJsonArray arr = obj["functions"].toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject fwj = v.toObject();
        const QString type = fwj["type"].toString();
        FT_FunctionWidget *fw = createFunction(type);
        if (fw) {
            fw->fromConfig(QString::fromUtf8(
                QJsonDocument(fwj).toJson(QJsonDocument::Indented)));
            addFunction(fw);
        }
    }
}