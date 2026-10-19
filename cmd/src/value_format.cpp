// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/value_format.hpp"

#include "tg/core/math.hpp"

#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace tg::cmd {
namespace {

// AutoCAD's default length precision (docs/adr/0014).
constexpr int kDecimals = 4;

std::string fixed(double value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(kDecimals) << value;
    std::string text = stream.str();
    if (text == "-0.0000") {
        text.erase(0, 1);
    }
    return text;
}

}  // namespace

std::string formatLength(double value) {
    return fixed(value);
}

std::string formatAngle(double radians) {
    std::string text = fixed(core::radiansToDegrees(radians));
    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') {
            text.pop_back();
        }
        if (text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

}  // namespace tg::cmd
