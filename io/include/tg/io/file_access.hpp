// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/result.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace tg::io {

/// How io and the file commands reach files (docs/adr/0021, point 4).
/// Names are UTF-8.
class FileAccess {
public:
    virtual ~FileAccess() = default;

    /// Every byte of the file `name`.
    [[nodiscard]] virtual Result<std::string> read(std::string_view name) = 0;

    /// Writes atomically: a temporary file in the same folder, renamed when
    /// complete. On failure the old file, if any, is untouched.
    [[nodiscard]] virtual std::optional<Error> write(std::string_view name,
                                                     std::string_view bytes) = 0;

    [[nodiscard]] virtual bool exists(std::string_view name) const = 0;
};

/// The real file system, without restrictions (the application).
///
/// A name is a path, absolute or relative to the current folder. Errors are
/// of kind Error::Kind::File, with line 0.
class DiskFiles final : public FileAccess {
public:
    [[nodiscard]] Result<std::string> read(std::string_view name) override;
    [[nodiscard]] std::optional<Error> write(std::string_view name,
                                             std::string_view bytes) override;
    [[nodiscard]] bool exists(std::string_view name) const override;
};

}  // namespace tg::io
