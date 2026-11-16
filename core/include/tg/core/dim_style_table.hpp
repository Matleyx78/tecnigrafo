// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

namespace tg::core {

/// The dimension styles of a drawing, owned by Document (docs/adr/0044,
/// point 3; docs/adr/0047). Empty for now: the package of the dimension
/// styles adds the styles and their operations in this file.
class DimStyleTable {
public:
    friend bool operator==(const DimStyleTable&, const DimStyleTable&) = default;
};

}  // namespace tg::core
