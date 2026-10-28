#include <TB_CM_Theme.h>

#include <QFontDatabase>

namespace {

QFont monospaceFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(9);
    font.setStyleHint(QFont::Monospace);
    return font;
}

} // namespace

TB_CM_Theme TB_CM_Theme::dark()
{
    TB_CM_Theme theme;
    theme.background = QColor(0x1e, 0x1f, 0x22);
    theme.alternateBackground = QColor(0x22, 0x23, 0x27);
    theme.text = QColor(0xd6, 0xd8, 0xdc);
    theme.textMuted = QColor(0x79, 0x80, 0x89);
    theme.category = QColor(0x62, 0xa8, 0xe8);
    theme.separator = QColor(0x2e, 0x30, 0x34);
    theme.selection = QColor(0x2c, 0x4b, 0x74);
    theme.hover = QColor(0x2a, 0x2c, 0x31);
    theme.matchBackground = QColor(0x8a, 0x6d, 0x00, 0xcc);
    theme.matchText = QColor(0xff, 0xf4, 0xcf);
    theme.scrollBar = QColor(0x45, 0x49, 0x4f);
    theme.scrollBarHover = QColor(0x5c, 0x62, 0x6a);
    theme.inputBackground = QColor(0x17, 0x18, 0x1b);
    theme.checkedBackground = QColor(0x33, 0x36, 0x3c);
    theme.levelColors = {
        QColor(0x6e, 0x76, 0x81),
        QColor(0x58, 0xa6, 0xff),
        QColor(0x3f, 0xb9, 0x50),
        QColor(0xd2, 0x99, 0x22),
        QColor(0xf8, 0x51, 0x49),
        QColor(0xdb, 0x61, 0xa2),
    };
    theme.font = monospaceFont();
    return theme;
}
