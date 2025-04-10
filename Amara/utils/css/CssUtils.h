#ifndef UITLS_H
#define UITLS_H
#include <string>
#include <algorithm>
#include <regex>
#include <cctype>

#include <masharifcore/Masharif.h>
#include "BorderInfo.h"
#include "Style.h"
using namespace masharif;

struct MarginPaddingValues {
    CSSValue top;
    CSSValue right;
    CSSValue bottom;
    CSSValue left;
};

struct FlexShorthand {
    float grow = 0.0f;
    float shrink = 1.0f;
    std::optional<CSSValue> basis;
};

std::vector<CSSValue> parseCSSValues(const std::string &input);

MarginPaddingValues parseMarginOrPadding(const std::string &input);

bool caseInsensitiveCompare(const std::string &s1, const std::string &s2);

inline std::string trim(const std::string &str);

inline std::vector<std::string> split(const std::string &s);

// Helper function to convert to lowercase
inline std::string toLower(const std::string &str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

CSSValue parseCSSValue(const std::string &input);

BorderShorthand parseBorderShorthand(const std::string &input);

BorderStyle parseBorderStyle(const std::string &input);

BorderWidths parseBorderWidth(const std::string &input);

BorderStyles parseBorderStyleShorthand(const std::string &input);

std::array<Color, 4> parseBorderColor(const std::string &input);

BorderRadius parseBorderRadius(const std::string &input);

ParsedBorderEdge parseBorderEdge(const std::string &input);

Visibility parseVisibility(const std::string &input);

JustifyContent parseJustifyContent(const std::string &input);

AlignItems parseAlignItems(const std::string &input);

FlexShorthand parseFlexShorthand(const std::string &input);

AlignContent parseAlignContent(const std::string &input);

FlexDirection parseFlexDirection(const std::string &input);

FlexWrap parseFlexWrap(const std::string &input);

Gap parseGap(const std::string &input);

DisplayType parseDisplay(const std::string& input);
#endif //UITLS_H
