#include "FT_Project.h"
#include "FT_FunctionList.h"
#include "FT_Function.h"
#include "FT_FunctionItem.h"
#include "FT_Data.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QListWidgetItem>
#include <string>

FT_Project::FT_Project(FT_FunctionList* list)
    : m_funcList(list)
{
}

void FT_Project::markDirty()
{
    if (m_suppressDirty)
        return;
    m_dirty = true;
}

void FT_Project::markClean()
{
    m_dirty = false;
}

FT_FunctionDocument FT_Project::collectDocument() const
{
    FT_FunctionDocument doc;
    for (int i = 0; i < m_funcList->count(); ++i) {
        QListWidgetItem* item = m_funcList->item(i);
        auto* funcItem = qobject_cast<FT_FunctionItem*>(m_funcList->itemWidget(item));
        if (!funcItem)
            continue;
        doc.push_back(funcItem->toConfig());
    }
    return doc;
}

bool FT_Project::validatePayloadHex(const FT_FunctionDocument& doc)
{
    for (int i = 0; i < doc.size(); ++i) {
        const FT_FunctionItemConfig& cfg = doc[i];
        for (const QVector<FT_FunctionData>& line : cfg.lines) {
            for (const FT_FunctionData& data : line) {
                if (!std::holds_alternative<FT_TboxConfig>(data))
                    continue;
                const FT_TboxConfig& tbox = std::get<FT_TboxConfig>(data);
                const QStringList tokens = ftSplitWs(tbox.payload);
                if (!tokens.isEmpty() && !ftAllHexByteTokens(tokens)) {
                    m_lastError = QObject::tr("Step \"%1\": payload must be hex bytes "
                                              "separated by spaces (e.g. 22 66).")
                                      .arg(cfg.title.isEmpty() ? QObject::tr("(untitled)") : cfg.title);
                    return false;
                }
            }
        }
    }
    return true;
}

bool FT_Project::exportToFile(const QString& path)
{
    FT_FunctionDocument doc = collectDocument();

    if (!validatePayloadHex(doc))
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = QObject::tr("Cannot write file:\n%1\n%2").arg(path, file.errorString());
        return false;
    }

    const std::string text = ftDocumentToJson(doc).dump(2);
    const QByteArray bytes = QByteArray::fromStdString(text);
    const qint64 n = file.write(bytes);
    if (n != bytes.size() || !file.flush()) {
        const QString err = file.errorString();
        file.close();
        m_lastError = QObject::tr("Cannot write file:\n%1\n%2").arg(path, err);
        return false;
    }
    file.close();

    markClean();
    return true;
}

bool FT_Project::importFromFile(const QString& path, FT_FunctionDocument& outDoc)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QObject::tr("Cannot read file:\n%1\n%2").arg(path, file.errorString());
        return false;
    }

    const QByteArray raw = file.readAll();
    file.close();

    FtJson root;
    try {
        root = FtJson::parse(std::string(raw.constData(), static_cast<size_t>(raw.size())));
    } catch (const std::exception& e) {
        m_lastError = QObject::tr("The file is not a valid config JSON:\n%1")
                          .arg(QString::fromUtf8(e.what()));
        return false;
    }

    try {
        outDoc = ftDocumentFromJson(root);
    } catch (const std::exception& e) {
        m_lastError = QObject::tr("Malformed config structure:\n%1")
                          .arg(QString::fromUtf8(e.what()));
        return false;
    }

    markClean();
    return true;
}

bool FT_Project::confirmDiscardIfDirty(const QString& actionTitle)
{
    if (!m_dirty)
        return true;

    const auto ret = QMessageBox::question(
        nullptr, actionTitle,
        QObject::tr("There are unsaved changes. Discard them?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return ret == QMessageBox::Yes;
}