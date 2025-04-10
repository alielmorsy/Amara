

#ifndef BORDERINFO_H
#define BORDERINFO_H
#include <optional>

#include "masharifcore/Masharif.h"
using namespace masharif;

enum class BorderStyle {
    NONE,
    HIDDEN,
    DOTTED,
    DASHED,
    SOLID,
    DOUBLE,
    GROOVE,
    RIDGE,
    INSET,
    OUTSET
};

struct BorderWidths {
    CSSValue top;
    CSSValue right;
    CSSValue bottom;
    CSSValue left;
};

struct BorderStyles {
    BorderStyle top = BorderStyle::NONE;
    BorderStyle right = BorderStyle::NONE;
    BorderStyle bottom = BorderStyle::NONE;
    BorderStyle left = BorderStyle::NONE;
};

struct BorderEdge {
    CSSValue width;
    BorderStyle style = BorderStyle::NONE;
    Color color;

    bool operator==(const BorderEdge& other) const {
        return (width == other.width) &&
               (style == other.style) &&
               (color == other.color);
    }
};

struct BorderRadius {
    CSSValue topLeft;
    CSSValue topRight;
    CSSValue bottomRight;
    CSSValue bottomLeft;

    BorderRadius() = default;
    explicit BorderRadius(const CSSValue& all)
        : topLeft(all), topRight(all), bottomRight(all), bottomLeft(all) {}
};

struct Border {
    BorderEdge top;
    BorderEdge right;
    BorderEdge bottom;
    BorderEdge left;
    BorderRadius radius;

    // Convenience methods
    void setAllEdges(const BorderEdge& edge) {
        top = right = bottom = left = edge;
    }

    [[nodiscard]] bool hasUniformWidth() const {
        return (top.width == right.width &&
                top.width == bottom.width &&
                top.width == left.width);
    }
};
struct BorderShorthand {
    std::optional<CSSValue> width;
    std::optional<BorderStyle> style;
    std::optional<Color> color;
};

struct ParsedBorderEdge {
    std::optional<CSSValue> width;
    std::optional<BorderStyle> style;
    std::optional<Color> color;
};
#endif //BORDERINFO_H
