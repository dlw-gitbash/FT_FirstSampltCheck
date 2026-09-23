#include <QApplication>
#include "FT_Edit.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("FT_Edit"));

    FT_Edit editor;
    editor.show();

    return app.exec();
}