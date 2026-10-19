// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/line.hpp"

namespace tg::core {

double Line::length() const {
    return distance(start, end);
}

BBox Line::boundingBox() const {
    return {start, end};
}

}  // namespace tg::core
