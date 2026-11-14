// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

namespace tg::core {

/// The text styles of a drawing, owned by Document (docs/adr/0044, point 3;
/// docs/adr/0046). Empty for now: the package of the text styles adds the
/// styles and their operations in this file.
class TextStyleTable {
public:
    friend bool operator==(const TextStyleTable&, const TextStyleTable&) = default;
};

}  // namespace tg::core
