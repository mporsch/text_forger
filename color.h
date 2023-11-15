#pragma once

#include "literal.h"

#include <iostream>
#include <vector>

class Color {
    enum ColorCode {
        Black        =  40,
        Red          =  41,
        Green        =  42,
        Yellow       =  43,
        Blue         =  44,
        Magenta      =  45,
        Cyan         =  46,
        LightGray    =  47,
        DarkGray     = 100,
        LightRed     = 101,
        LightGreen   = 102,
        LightYellow  = 103,
        LightBlue    = 104,
        LightMagenta = 105,
        LightCyan    = 106
    };

public:
    Color()
        : m_colorCode(Black) {
    }

    static Color Reset() {
        auto c = Color();
        c.m_colorCode = static_cast<ColorCode>(0);
        return c;
    }

    static Color FromId(size_t id) {
        static std::vector<ColorCode> const colorLut = GetColorLut();

        Color ret;
        ret.m_colorCode = colorLut[id % colorLut.size()];
        return ret;
    }

    friend OSTREAM &operator<<(OSTREAM &os, Color const &color);

private:
    static std::vector<ColorCode> GetColorLut() {
        std::vector<ColorCode> ret;

        for(int i = Red; i <= LightGray; ++i) {
            ret.push_back(static_cast<ColorCode>(i));
        }
        for(int i = DarkGray; i <= LightCyan; ++i) {
            ret.push_back(static_cast<ColorCode>(i));
        }

        return ret;
    }

private:
    ColorCode m_colorCode;
};

OSTREAM &operator<<(OSTREAM &os, [[maybe_unused]] Color const &color) {
#ifdef USE_COLOR
    os << LITERAL("\x1B[") << color.m_colorCode << LITERAL("m");
#endif // USE_COLOR
    return os;
}
