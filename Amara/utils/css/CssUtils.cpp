#include "CssUtils.h"

std::vector<CSSValue> parseCSSValues(const std::string &input) {
    std::vector<CSSValue> values;
    std::vector<std::string> tokens = split(trim(input));

    for (const auto &token: tokens) {
        CSSValue value = parseCSSValue(token);
        if (!value.isUndefined()) {
            values.push_back(value);
        }
    }

    return values;
}

MarginPaddingValues parseMarginOrPadding(const std::string &input) {
    MarginPaddingValues result;
    std::string trimmed = trim(input);
    if (trimmed.empty()) return result;

    std::vector<std::string> tokens;
    std::istringstream stream(trimmed);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }

    switch (tokens.size()) {
        case 1:
            result.top = result.right = result.bottom = result.left = parseCSSValue(tokens[0]);
            break;
        case 2:
            result.top = result.bottom = parseCSSValue(tokens[0]);
            result.right = result.left = parseCSSValue(tokens[1]);
            break;
        case 3:
            result.top = parseCSSValue(tokens[0]);
            result.right = result.left = parseCSSValue(tokens[1]);
            result.bottom = parseCSSValue(tokens[2]);
            break;
        case 4:
            result.top = parseCSSValue(tokens[0]);
            result.right = parseCSSValue(tokens[1]);
            result.bottom = parseCSSValue(tokens[2]);
            result.left = parseCSSValue(tokens[3]);
            break;
        default:
            // Invalid input, return all AUTO
            break;
    }

    return result;
}

bool caseInsensitiveCompare(const std::string &s1, const std::string &s2) {
    if (s1.size() != s2.size()) return false;
    const auto len = s1.size();
    const auto *p1 = s1.data();
    const auto *p2 = s2.data();

    for (size_t i = 0; i < len; ++i) {
        const uint8_t c1 = p1[i];
        const uint8_t c2 = p2[i];

        // Quick check for exact match (common case)
        if (c1 == c2) continue;

        // Only convert to lowercase if needed
        const int lc1 = std::tolower(c1);
        const int lc2 = std::tolower(c2);

        if (lc1 != lc2) {
            return false;
        }
    }

    return true;
}

std::string trim(const std::string &str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) ++start;

    auto end = str.end();
    do {
        --end;
    } while (end >= start && std::isspace(*end));

    return {start, end + 1};
}

CSSValue parseCSSValue(const std::string &input) {
    std::string trimmed = trim(input);
    if (trimmed.empty()) return CSSValue();

    // Check for 'auto' (case-insensitive)
    if (caseInsensitiveCompare(trimmed, "auto") == 0) {
        return CSSValue();
    }

    // Regular expression to match CSS values with units
    static const std::regex pattern(
        R"(^([+-]?\d*\.?\d+)(px|%)$)",
        std::regex_constants::icase
    );

    std::smatch matches;
    if (!std::regex_match(trimmed, matches, pattern)) {
        return CSSValue(); // Invalid format
    }

    // Parse value and unit
    float value = std::stof(matches[1].str());
    std::string unitStr = toLower(matches[2].str());

    if (unitStr == "%") {
        return CSSValue(value, CSSUnit::PERCENT);
    } else if (unitStr == "px") {
        return CSSValue(value, CSSUnit::PX);
    }

    return CSSValue(); // Unknown unit
}

std::vector<std::string> split(const std::string &s) {
    std::vector<std::string> tokens;
    std::istringstream stream(s);
    std::string token;
    while (stream >> token) {
        tokens.emplace_back(token);
    }
    return tokens;
}

BorderShorthand parseBorderShorthand(const std::string &input) {
    BorderShorthand result;
    std::vector<std::string> tokens = split(trim(input));

    for (const auto &token: tokens) {
        // Try color first (using your existing Color class)
        if (Color::isValid(token)) {
            result.color = Color(token);
            continue;
        }

        // Try style
        auto parsedStyle = parseBorderStyle(token);
        if (parsedStyle != BorderStyle::NONE) {
            result.style = parsedStyle;
            continue;
        }

        // Try width
        auto width = parseCSSValue(token);
        if (!width.isUndefined()) {
            result.width = width;
        }
    }

    return result;
}

BorderStyle parseBorderStyle(const std::string &input) {
    std::string lower = toLower(trim(input));
    if (lower == "none") return BorderStyle::NONE;
    if (lower == "hidden") return BorderStyle::HIDDEN;
    if (lower == "dotted") return BorderStyle::DOTTED;
    if (lower == "dashed") return BorderStyle::DASHED;
    if (lower == "solid") return BorderStyle::SOLID;
    if (lower == "double") return BorderStyle::DOUBLE;
    if (lower == "groove") return BorderStyle::GROOVE;
    if (lower == "ridge") return BorderStyle::RIDGE;
    if (lower == "inset") return BorderStyle::INSET;
    if (lower == "outset") return BorderStyle::OUTSET;
    return BorderStyle::NONE;
}

BorderWidths parseBorderWidth(const std::string &input) {
    BorderWidths result;
    auto values = parseMarginOrPadding(input); // Reuse existing parser
    result.top = values.top;
    result.right = values.right;
    result.bottom = values.bottom;
    result.left = values.left;
    return result;
}


ParsedBorderEdge parseBorderEdge(const std::string &input) {
    ParsedBorderEdge result;
    std::vector<std::string> tokens = split(trim(input));

    for (const auto &token: tokens) {
        // First check for color (most specific)
        if (Color::isValid(token)) {
            result.color = Color(token);
            continue;
        }

        // Then check for border style
        auto parsedStyle = parseBorderStyle(token);
        if (parsedStyle != BorderStyle::NONE) {
            result.style = parsedStyle;
            continue;
        }

        // Finally check for width
        auto parsedWidth = parseCSSValue(token);
        if (!parsedWidth.isUndefined()) {
            result.width = parsedWidth;
        }
    }

    return result;
}


std::array<Color, 4> parseBorderColor(const std::string &input) {
    std::array<Color, 4> colors;
    std::vector<std::string> tokens = split(trim(input));

    switch (tokens.size()) {
        case 1:
            colors.fill(Color(tokens[0]));
            break;
        case 2:
            colors[0] = colors[2] = Color(tokens[0]);
            colors[1] = colors[3] = Color(tokens[1]);
            break;
        case 3:
            colors[0] = Color(tokens[0]);
            colors[1] = colors[3] = Color(tokens[1]);
            colors[2] = Color(tokens[2]);
            break;
        case 4:
            for (int i = 0; i < 4; ++i)
                colors[i] = Color(tokens[i]);
            break;
        default:
            colors.fill(Color()); // Default to transparent
    }
    return colors;
}


BorderRadius parseBorderRadius(const std::string &input) {
    BorderRadius radius;
    std::vector<CSSValue> values = parseCSSValues(input);

    switch (values.size()) {
        case 1:
            radius = BorderRadius(values[0]);
            break;
        case 2:
            radius.topLeft = values[0];
            radius.bottomRight = values[0];
            radius.topRight = radius.bottomLeft = values[1];
            break;
        case 3:
            radius.topLeft = values[0];
            radius.topRight = radius.bottomRight = values[1];
            radius.bottomLeft = values[2];
            break;
        case 4:
            radius.topLeft = values[0];
            radius.topRight = values[1];
            radius.bottomRight = values[2];
            radius.bottomLeft = values[3];
            break;
        default:
            // Invalid input, return default
            break;
    }
    return radius;
}

Visibility parseVisibility(const std::string &input) {
    auto handled = trim(input);
    if (caseInsensitiveCompare(handled, "visible") == 0) return Visibility::Visible;
    if (caseInsensitiveCompare(handled, "hidden") == 0) return Visibility::Hidden;
    if (caseInsensitiveCompare(handled, "collapse") == 0) return Visibility::Collapse;
    if (caseInsensitiveCompare(handled, "unset") == 0) return Visibility::Unset;
    if (caseInsensitiveCompare(handled, "inherit") == 0) return Visibility::Inherit;
    return Visibility::Visible;
}


FlexShorthand parseFlexShorthand(const std::string &input) {
    FlexShorthand result;
    auto tokens = split(trim(input));

    switch (tokens.size()) {
        case 1: {
            auto value = parseCSSValue(tokens[0]);
            if (!value.isUndefined()) {
                result.basis = value;
            } else {
                result.grow = std::stof(tokens[0]);
            }
            break;
        }
        case 2: {
            result.grow = std::stof(tokens[0]);
            auto value = parseCSSValue(tokens[1]);
            if (!value.isUndefined()) {
                result.basis = value;
            } else {
                result.shrink = std::stof(tokens[1]);
            }
            break;
        }
        case 3: {
            result.grow = std::stof(tokens[0]);
            result.shrink = std::stof(tokens[1]);
            result.basis = parseCSSValue(tokens[2]);
            break;
        }
        default:
            // Invalid input, use defaults
            break;
    }
    return result;
}

JustifyContent parseJustifyContent(const std::string &input) {
    const std::string lower = toLower(trim(input));
    if (lower == "flex-start") return JustifyContent::FlexStart;
    if (lower == "flex-end") return JustifyContent::FlexEnd;
    if (lower == "center") return JustifyContent::FlexCenter;
    if (lower == "space-between") return JustifyContent::SpaceBetween;
    if (lower == "space-around") return JustifyContent::SpaceAround;
    if (lower == "space-evenly") return JustifyContent::SpaceEvenly;
    return JustifyContent::FlexStart; // Default
}

// Parse align-items
AlignItems parseAlignItems(const std::string &input) {
    std::string lower = toLower(trim(input));
    if (lower == "flex-start") return AlignItems::FlexStart;
    if (lower == "flex-end") return AlignItems::FlexEnd;
    if (lower == "center") return AlignItems::FlexCenter;
    if (lower == "stretch") return AlignItems::Stretch;
    if (lower == "baseline") return AlignItems::Baseline;
    if (lower == "auto") return AlignItems::AUTO_ALIGN;
    return AlignItems::Stretch; // Default
}

// Parse align-content
AlignContent parseAlignContent(const std::string &input) {
    std::string lower = toLower(trim(input));
    if (lower == "flex-start") return AlignContent::FlexStart;
    if (lower == "flex-end") return AlignContent::FlexEnd;
    if (lower == "center") return AlignContent::FlexCenter;
    if (lower == "stretch") return AlignContent::Stretch;
    if (lower == "space-between") return AlignContent::SpaceBetween;
    if (lower == "space-around") return AlignContent::SpaceAround;
    return AlignContent::Stretch; // Default
}

// Parse flex-direction
FlexDirection parseFlexDirection(const std::string &input) {
    std::string lower = toLower(trim(input));
    if (lower == "row") return FlexDirection::Row;
    if (lower == "column") return FlexDirection::Column;
    if (lower == "row-reverse") return FlexDirection::RowReverse;
    if (lower == "column-reverse") return FlexDirection::ColumnReverse;
    return FlexDirection::Row; // Default
}

// Parse flex-wrap
FlexWrap parseFlexWrap(const std::string &input) {
    std::string lower = toLower(trim(input));
    if (lower == "nowrap") return FlexWrap::NoWrap;
    if (lower == "wrap") return FlexWrap::Wrap;
    if (lower == "wrap-reverse") return FlexWrap::WrapReverse;
    return FlexWrap::NoWrap; // Default
}

Gap parseGap(const std::string &input) {
    Gap gap;
    std::vector<CSSValue> values = parseCSSValues(input);

    switch (values.size()) {
        case 1:
            gap.row = gap.column = values[0];
            break;
        case 2:
            gap.row = values[0];
            gap.column = values[1];
            break;
        default:
            // Invalid input, use defaults
            break;
    }
    return gap;
}

DisplayType parseDisplay(const std::string &input) {
    std::string lower = toLower(trim(input));
    if (lower == "block") return DisplayType::Block;
    if (lower == "inline-block") return DisplayType::InlineBlock;
    if (lower == "flex") return DisplayType::Flex;
    if (lower == "inline-flex") return DisplayType::InlineFlex;
    if (lower == "none") return DisplayType::None;
    return DisplayType::Block; // Default value
}
