#include <QApplication>
#include "FT_Edit.h"
#include "FT_FunctionWidgetItem.h"
#include "FT_TBoxFunctionWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FT_FirstSampltCheck"));

    FT_Edit editor;
    editor.setWindowTitle(QStringLiteral("FT First Sample Check - 配置编辑"));
    editor.resize(960, 620);
    editor.setMinimumSize(640, 360);

    editor.addSchedule();

    editor.show();
    return app.exec();
}