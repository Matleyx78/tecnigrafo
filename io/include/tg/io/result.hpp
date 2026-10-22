// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace tg::io {

/// Why reading or writing a file failed (docs/adr/0017, point 6).
struct Error {
    enum class Kind : std::uint8_t {
        Malformed,       ///< the file breaks the format
        Unsupported,     ///< a valid file Tecnigrafo cannot read (binary DXF, old version)
        TooLarge,        ///< a safety limit was exceeded (docs/adr/0017, point 7)
        File,            ///< the file system refused the operation
        NotImplemented,  ///< the function arrives with a later package
    };

    Kind kind{Kind::Malformed};
    std::size_t line{0};  ///< line of the file, 1-based; 0 if not about a line
    std::string message;  ///< English, says what was expected and what was found

    friend bool operator==(const Error&, const Error&) = default;
};

/// A value or an Error (no exception ever leaves the module, docs/adr/0017).
///
/// Both constructors are implicit, so that a function returning Result<T>
/// can simply `return value;` or `return Error{…};`.
template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value) : m_value(std::move(value)) {}

    Result(Error error) : m_error(std::move(error)) {}

    [[nodiscard]] bool ok() const { return m_value.has_value(); }

    /// The value. Precondition: ok().
    [[nodiscard]] T& value() & {
        assert(ok());
        return *m_value;
    }

    /// The value. Precondition: ok().
    [[nodiscard]] const T& value() const& {
        assert(ok());
        return *m_value;
    }

    /// The value, moved out. Precondition: ok().
    [[nodiscard]] T&& value() && {
        assert(ok());
        return *std::move(m_value);
    }

    /// The error. Precondition: !ok().
    [[nodiscard]] const Error& error() const {
        assert(!ok());
        return m_error;
    }

private:
    // An optional and a plain Error rather than a variant: the static
    // analyzer follows them, and an Error is cheap.
    std::optional<T> m_value;
    Error m_error;
};

}  // namespace tg::io
