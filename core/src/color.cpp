// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/color.hpp"

#include <array>
#include <charconv>
#include <limits>

namespace tg::core {
namespace {
// ADR 0024, point 3: base colours, 24 hues at 15 degree intervals,
// five brightness levels with full/half saturation, then six greys.
constexpr std::array<Rgb, 255> kAci{{
    Rgb{255, 0, 0},      // 1
    Rgb{255, 255, 0},    // 2
    Rgb{0, 255, 0},      // 3
    Rgb{0, 255, 255},    // 4
    Rgb{0, 0, 255},      // 5
    Rgb{255, 0, 255},    // 6
    Rgb{255, 255, 255},  // 7
    Rgb{128, 128, 128},  // 8
    Rgb{192, 192, 192},  // 9
    Rgb{255, 0, 0},      // 10
    Rgb{255, 127, 127},  // 11
    Rgb{165, 0, 0},      // 12
    Rgb{165, 82, 82},    // 13
    Rgb{127, 0, 0},      // 14
    Rgb{127, 63, 63},    // 15
    Rgb{76, 0, 0},       // 16
    Rgb{76, 38, 38},     // 17
    Rgb{38, 0, 0},       // 18
    Rgb{38, 19, 19},     // 19
    Rgb{255, 63, 0},     // 20
    Rgb{255, 159, 127},  // 21
    Rgb{165, 41, 0},     // 22
    Rgb{165, 103, 82},   // 23
    Rgb{127, 31, 0},     // 24
    Rgb{127, 79, 63},    // 25
    Rgb{76, 19, 0},      // 26
    Rgb{76, 47, 38},     // 27
    Rgb{38, 9, 0},       // 28
    Rgb{38, 23, 19},     // 29
    Rgb{255, 127, 0},    // 30
    Rgb{255, 191, 127},  // 31
    Rgb{165, 82, 0},     // 32
    Rgb{165, 123, 82},   // 33
    Rgb{127, 63, 0},     // 34
    Rgb{127, 95, 63},    // 35
    Rgb{76, 38, 0},      // 36
    Rgb{76, 57, 38},     // 37
    Rgb{38, 19, 0},      // 38
    Rgb{38, 28, 19},     // 39
    Rgb{255, 191, 0},    // 40
    Rgb{255, 223, 127},  // 41
    Rgb{165, 123, 0},    // 42
    Rgb{165, 144, 82},   // 43
    Rgb{127, 95, 0},     // 44
    Rgb{127, 111, 63},   // 45
    Rgb{76, 57, 0},      // 46
    Rgb{76, 66, 38},     // 47
    Rgb{38, 28, 0},      // 48
    Rgb{38, 33, 19},     // 49
    Rgb{255, 255, 0},    // 50
    Rgb{255, 255, 127},  // 51
    Rgb{165, 165, 0},    // 52
    Rgb{165, 165, 82},   // 53
    Rgb{127, 127, 0},    // 54
    Rgb{127, 127, 63},   // 55
    Rgb{76, 76, 0},      // 56
    Rgb{76, 76, 38},     // 57
    Rgb{38, 38, 0},      // 58
    Rgb{38, 38, 19},     // 59
    Rgb{191, 255, 0},    // 60
    Rgb{223, 255, 127},  // 61
    Rgb{123, 165, 0},    // 62
    Rgb{144, 165, 82},   // 63
    Rgb{95, 127, 0},     // 64
    Rgb{111, 127, 63},   // 65
    Rgb{57, 76, 0},      // 66
    Rgb{66, 76, 38},     // 67
    Rgb{28, 38, 0},      // 68
    Rgb{33, 38, 19},     // 69
    Rgb{127, 255, 0},    // 70
    Rgb{191, 255, 127},  // 71
    Rgb{82, 165, 0},     // 72
    Rgb{123, 165, 82},   // 73
    Rgb{63, 127, 0},     // 74
    Rgb{95, 127, 63},    // 75
    Rgb{38, 76, 0},      // 76
    Rgb{57, 76, 38},     // 77
    Rgb{19, 38, 0},      // 78
    Rgb{28, 38, 19},     // 79
    Rgb{63, 255, 0},     // 80
    Rgb{159, 255, 127},  // 81
    Rgb{41, 165, 0},     // 82
    Rgb{103, 165, 82},   // 83
    Rgb{31, 127, 0},     // 84
    Rgb{79, 127, 63},    // 85
    Rgb{19, 76, 0},      // 86
    Rgb{47, 76, 38},     // 87
    Rgb{9, 38, 0},       // 88
    Rgb{23, 38, 19},     // 89
    Rgb{0, 255, 0},      // 90
    Rgb{127, 255, 127},  // 91
    Rgb{0, 165, 0},      // 92
    Rgb{82, 165, 82},    // 93
    Rgb{0, 127, 0},      // 94
    Rgb{63, 127, 63},    // 95
    Rgb{0, 76, 0},       // 96
    Rgb{38, 76, 38},     // 97
    Rgb{0, 38, 0},       // 98
    Rgb{19, 38, 19},     // 99
    Rgb{0, 255, 63},     // 100
    Rgb{127, 255, 159},  // 101
    Rgb{0, 165, 41},     // 102
    Rgb{82, 165, 103},   // 103
    Rgb{0, 127, 31},     // 104
    Rgb{63, 127, 79},    // 105
    Rgb{0, 76, 19},      // 106
    Rgb{38, 76, 47},     // 107
    Rgb{0, 38, 9},       // 108
    Rgb{19, 38, 23},     // 109
    Rgb{0, 255, 127},    // 110
    Rgb{127, 255, 191},  // 111
    Rgb{0, 165, 82},     // 112
    Rgb{82, 165, 123},   // 113
    Rgb{0, 127, 63},     // 114
    Rgb{63, 127, 95},    // 115
    Rgb{0, 76, 38},      // 116
    Rgb{38, 76, 57},     // 117
    Rgb{0, 38, 19},      // 118
    Rgb{19, 38, 28},     // 119
    Rgb{0, 255, 191},    // 120
    Rgb{127, 255, 223},  // 121
    Rgb{0, 165, 123},    // 122
    Rgb{82, 165, 144},   // 123
    Rgb{0, 127, 95},     // 124
    Rgb{63, 127, 111},   // 125
    Rgb{0, 76, 57},      // 126
    Rgb{38, 76, 66},     // 127
    Rgb{0, 38, 28},      // 128
    Rgb{19, 38, 33},     // 129
    Rgb{0, 255, 255},    // 130
    Rgb{127, 255, 255},  // 131
    Rgb{0, 165, 165},    // 132
    Rgb{82, 165, 165},   // 133
    Rgb{0, 127, 127},    // 134
    Rgb{63, 127, 127},   // 135
    Rgb{0, 76, 76},      // 136
    Rgb{38, 76, 76},     // 137
    Rgb{0, 38, 38},      // 138
    Rgb{19, 38, 38},     // 139
    Rgb{0, 191, 255},    // 140
    Rgb{127, 223, 255},  // 141
    Rgb{0, 123, 165},    // 142
    Rgb{82, 144, 165},   // 143
    Rgb{0, 95, 127},     // 144
    Rgb{63, 111, 127},   // 145
    Rgb{0, 57, 76},      // 146
    Rgb{38, 66, 76},     // 147
    Rgb{0, 28, 38},      // 148
    Rgb{19, 33, 38},     // 149
    Rgb{0, 127, 255},    // 150
    Rgb{127, 191, 255},  // 151
    Rgb{0, 82, 165},     // 152
    Rgb{82, 123, 165},   // 153
    Rgb{0, 63, 127},     // 154
    Rgb{63, 95, 127},    // 155
    Rgb{0, 38, 76},      // 156
    Rgb{38, 57, 76},     // 157
    Rgb{0, 19, 38},      // 158
    Rgb{19, 28, 38},     // 159
    Rgb{0, 63, 255},     // 160
    Rgb{127, 159, 255},  // 161
    Rgb{0, 41, 165},     // 162
    Rgb{82, 103, 165},   // 163
    Rgb{0, 31, 127},     // 164
    Rgb{63, 79, 127},    // 165
    Rgb{0, 19, 76},      // 166
    Rgb{38, 47, 76},     // 167
    Rgb{0, 9, 38},       // 168
    Rgb{19, 23, 38},     // 169
    Rgb{0, 0, 255},      // 170
    Rgb{127, 127, 255},  // 171
    Rgb{0, 0, 165},      // 172
    Rgb{82, 82, 165},    // 173
    Rgb{0, 0, 127},      // 174
    Rgb{63, 63, 127},    // 175
    Rgb{0, 0, 76},       // 176
    Rgb{38, 38, 76},     // 177
    Rgb{0, 0, 38},       // 178
    Rgb{19, 19, 38},     // 179
    Rgb{63, 0, 255},     // 180
    Rgb{159, 127, 255},  // 181
    Rgb{41, 0, 165},     // 182
    Rgb{103, 82, 165},   // 183
    Rgb{31, 0, 127},     // 184
    Rgb{79, 63, 127},    // 185
    Rgb{19, 0, 76},      // 186
    Rgb{47, 38, 76},     // 187
    Rgb{9, 0, 38},       // 188
    Rgb{23, 19, 38},     // 189
    Rgb{127, 0, 255},    // 190
    Rgb{191, 127, 255},  // 191
    Rgb{82, 0, 165},     // 192
    Rgb{123, 82, 165},   // 193
    Rgb{63, 0, 127},     // 194
    Rgb{95, 63, 127},    // 195
    Rgb{38, 0, 76},      // 196
    Rgb{57, 38, 76},     // 197
    Rgb{19, 0, 38},      // 198
    Rgb{28, 19, 38},     // 199
    Rgb{191, 0, 255},    // 200
    Rgb{223, 127, 255},  // 201
    Rgb{123, 0, 165},    // 202
    Rgb{144, 82, 165},   // 203
    Rgb{95, 0, 127},     // 204
    Rgb{111, 63, 127},   // 205
    Rgb{57, 0, 76},      // 206
    Rgb{66, 38, 76},     // 207
    Rgb{28, 0, 38},      // 208
    Rgb{33, 19, 38},     // 209
    Rgb{255, 0, 255},    // 210
    Rgb{255, 127, 255},  // 211
    Rgb{165, 0, 165},    // 212
    Rgb{165, 82, 165},   // 213
    Rgb{127, 0, 127},    // 214
    Rgb{127, 63, 127},   // 215
    Rgb{76, 0, 76},      // 216
    Rgb{76, 38, 76},     // 217
    Rgb{38, 0, 38},      // 218
    Rgb{38, 19, 38},     // 219
    Rgb{255, 0, 191},    // 220
    Rgb{255, 127, 223},  // 221
    Rgb{165, 0, 123},    // 222
    Rgb{165, 82, 144},   // 223
    Rgb{127, 0, 95},     // 224
    Rgb{127, 63, 111},   // 225
    Rgb{76, 0, 57},      // 226
    Rgb{76, 38, 66},     // 227
    Rgb{38, 0, 28},      // 228
    Rgb{38, 19, 33},     // 229
    Rgb{255, 0, 127},    // 230
    Rgb{255, 127, 191},  // 231
    Rgb{165, 0, 82},     // 232
    Rgb{165, 82, 123},   // 233
    Rgb{127, 0, 63},     // 234
    Rgb{127, 63, 95},    // 235
    Rgb{76, 0, 38},      // 236
    Rgb{76, 38, 57},     // 237
    Rgb{38, 0, 19},      // 238
    Rgb{38, 19, 28},     // 239
    Rgb{255, 0, 63},     // 240
    Rgb{255, 127, 159},  // 241
    Rgb{165, 0, 41},     // 242
    Rgb{165, 82, 103},   // 243
    Rgb{127, 0, 31},     // 244
    Rgb{127, 63, 79},    // 245
    Rgb{76, 0, 19},      // 246
    Rgb{76, 38, 47},     // 247
    Rgb{38, 0, 9},       // 248
    Rgb{38, 19, 23},     // 249
    Rgb{51, 51, 51},     // 250
    Rgb{80, 80, 80},     // 251
    Rgb{105, 105, 105},  // 252
    Rgb{130, 130, 130},  // 253
    Rgb{190, 190, 190},  // 254
    Rgb{255, 255, 255},  // 255
}};

std::string_view trim(std::string_view text) {
    constexpr std::string_view kWhitespace = " \t\r\n\f\v";
    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(kWhitespace) - first + 1);
}

std::optional<int> parseInteger(std::string_view text) {
    int value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}
}  // namespace

Color Color::fromIndex(int index) {
    assert(index >= 1 && index <= 255);
    Color color(Kind::Index);
    color.m_index = index;
    return color;
}

Rgb aciToRgb(int index) {
    assert(index >= 1 && index <= 255);
    return kAci.at(static_cast<std::size_t>(index - 1));
}

int nearestAci(Rgb rgb) {
    int best = 1;
    int bestDistance = std::numeric_limits<int>::max();
    for (int index = 1; index <= 255; ++index) {
        if (index == 7) {
            continue;
        }
        const auto candidate = aciToRgb(index);
        const int red = static_cast<int>(rgb.r) - candidate.r;
        const int green = static_cast<int>(rgb.g) - candidate.g;
        const int blue = static_cast<int>(rgb.b) - candidate.b;
        const int distance = red * red + green * green + blue * blue;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = index;
        }
    }
    return best;
}

std::string formatColor(const Color& color) {
    switch (color.kind()) {
        case Color::Kind::ByLayer:
            return "bylayer";
        case Color::Kind::ByBlock:
            return "byblock";
        case Color::Kind::Index:
            return std::to_string(color.index());
        case Color::Kind::Rgb: {
            const auto rgb = color.rgb();
            return std::to_string(rgb.r) + "," + std::to_string(rgb.g) + "," +
                   std::to_string(rgb.b);
        }
    }
    return {};
}

std::optional<Color> parseColor(std::string_view text) {
    text = trim(text);
    if (text.empty()) {
        return std::nullopt;
    }
    std::string lower(text);
    for (auto& ch : lower) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    if (lower == "bylayer" || lower == "256") {
        return Color::byLayer();
    }
    if (lower == "byblock" || lower == "0") {
        return Color::byBlock();
    }
    constexpr std::array<std::string_view, 7> kNames{"red",  "yellow",  "green", "cyan",
                                                     "blue", "magenta", "white"};
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (lower == kNames.at(i)) {
            return Color::fromIndex(static_cast<int>(i) + 1);
        }
    }
    if (text.find(',') == std::string_view::npos) {
        const auto index = parseInteger(text);
        if (index && *index >= 1 && *index <= 255) {
            return Color::fromIndex(*index);
        }
        return std::nullopt;
    }
    std::array<std::uint8_t, 3> channels{};
    for (std::size_t i = 0; i < channels.size(); ++i) {
        const auto comma = text.find(',');
        if ((i < 2) != (comma != std::string_view::npos)) {
            return std::nullopt;
        }
        const auto value = parseInteger(text.substr(0, comma));
        if (!value || *value < 0 || *value > 255) {
            return std::nullopt;
        }
        channels.at(i) = static_cast<std::uint8_t>(*value);
        if (comma != std::string_view::npos) {
            text.remove_prefix(comma + 1);
        }
    }
    return Color::fromRgb({channels.at(0), channels.at(1), channels.at(2)});
}
}  // namespace tg::core
