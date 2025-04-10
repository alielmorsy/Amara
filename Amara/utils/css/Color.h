#ifndef COLOR_H
#define COLOR_H
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

class Color {
public:
    uint8_t r, g, b, a;

    Color() : r(0), g(0), b(0), a(255) {
    }

    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {
    }

    explicit Color(std::string_view hex) {
        setFromHex(hex);
    }

    explicit Color(uint32_t rgba) {
        r = (rgba >> 24) & 0xFF;
        g = (rgba >> 16) & 0xFF;
        b = (rgba >> 8) & 0xFF;
        a = rgba & 0xFF;
    }

    // Convert to hexadecimal string
    [[nodiscard]] std::string toHex(bool uppercase = false, bool force_alpha = false) const {
        std::ostringstream ss;
        ss << "#";
        if (uppercase) ss << std::uppercase;
        ss << std::hex << std::setfill('0');

        ss << std::setw(2) << static_cast<int>(r)
                << std::setw(2) << static_cast<int>(g)
                << std::setw(2) << static_cast<int>(b);

        if (force_alpha || a != 255) {
            ss << std::setw(2) << static_cast<int>(a);
        }

        return ss.str();
    }

    static uint8_t hex_value(char c) {
        static constexpr uint8_t table[256] = {
            // clang-format off
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
            0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        return table[static_cast<uint8_t>(c)];
    }

    // Set color from hexadecimal string

    void setFromHex(std::string_view hex) {
        if (hex.empty() || hex[0] != '#') {
            throw std::invalid_argument("Hex string must start with '#'");
        }

        const size_t length = hex.length();
        const size_t num_digits = length - 1;

        const bool is_shorthand = (num_digits == 3 || num_digits == 4);
        const bool is_full = (num_digits == 6 || num_digits == 8);

        if (!is_shorthand && !is_full) {
            throw std::invalid_argument("Invalid length. Use #RGB, #RGBA, #RRGGBB, or #RRGGBBAA.");
        }

        // Validate characters
        for (size_t i = 1; i < length; ++i) {
            const char c = hex[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                throw std::invalid_argument("Contains non-hex characters");
            }
        }

        // Parse components
        if (is_shorthand) {
            // Shorthand: #RGB or #RGBA
            r = hex_value(hex[1]) << 4 | hex_value(hex[1]);
            g = hex_value(hex[2]) << 4 | hex_value(hex[2]);
            b = hex_value(hex[3]) << 4 | hex_value(hex[3]);
            a = (num_digits == 4) ? (hex_value(hex[4]) << 4 | hex_value(hex[4])) : 255;
        } else {
            // Full: #RRGGBB or #RRGGBBAA
            r = hex_value(hex[1]) << 4 | hex_value(hex[2]);
            g = hex_value(hex[3]) << 4 | hex_value(hex[4]);
            b = hex_value(hex[5]) << 4 | hex_value(hex[6]);
            a = (num_digits == 8) ? (hex_value(hex[7]) << 4 | hex_value(hex[8])) : 255;
        }
    }

    // Equality operators
    bool operator==(const Color &other) const {
        return other.r == r && other.g == g && other.b == b && other.a == a;
    };

    bool operator!=(const Color &other) const {
        return !operator==(other);
    };

    // Utility methods
    [[nodiscard]] uint32_t toRGBA() const {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    [[nodiscard]] Color withAlpha(uint8_t alpha) const {
        return {r, g, b, alpha};
    }

    static bool isValid(const std::string& color) {
        static std::regex hexPattern("^#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})$");
        return std::regex_match(color, hexPattern);
    }
};

#endif //COLOR_H
