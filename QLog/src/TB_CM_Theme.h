#pragma once

#include <TB_CM_Types.h>

#include <QColor>
#include <QFont>

#include <array>

struct TB_CM_Theme {
    QColor background;
    QColor alternateBackground;
    QColor text;
    QColor textMuted;
    QColor category;
    QColor separator;
    QColor selection;
    QColor hover;
    QColor matchBackground;
    QColor matchText;
    QColor scrollBar;
    QColor scrollBarHover;
    QColor inputBackground;
    QColor checkedBackground;
    std::array<QColor, TB_CM_LevelCount> levelColors;
    QFont font;

    static TB_CM_Theme dark();
};
