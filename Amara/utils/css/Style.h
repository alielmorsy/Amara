#ifndef AMARA_STYLE_H
#define AMARA_STYLE_H


#include <masharifcore/Masharif.h>


#include "BorderInfo.h"
#include "Color.h"


//using css value from the layout engine directly to avoid recreation.
using namespace masharif;

enum class Visibility {
    Visible, // CSS: visibility: visible;
    Hidden, // CSS: visibility: hidden;
    Collapse, // CSS: visibility: collapse;
    Unset, // CSS: visibility: unset;
    Inherit // CSS: visibility: inherit;
};

enum class DisplayType {
    Block = 0,
    InlineBlock,
    Flex,
    InlineFlex,
    None
};

struct WidgetStyle {
    CSSValue width;
    CSSValue height;
    CSSValue minWidth;
    CSSValue maxWidth;
    CSSValue minHeight;
    CSSValue maxHeight;
    Color backgroundColor;
    MarginEdge margin;
    PaddingEdge padding;
    Border border;
    PositionType position = PositionType::Static;
    DisplayType display = DisplayType::Block;

    CSSFlex flex;

    CSSValue opacity{1.0, CSSUnit::PX};
    CSSValue boxShadow;
    CSSValue textShadow;
    Visibility visibility = Visibility::Visible;
    CSSValue overflowX;
    CSSValue overflowY;

    CSSValue fontSize;
    CSSValue fontWeight;
    std::string fontFamily;
    Color textColor;
    CSSValue lineHeight;
    CSSValue textAlign;
};


#endif //STYLE_H
