// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <string>
#include <string_view>
#include <vector>

/// Comparison of reports and answers with expected values
/// (docs/adr/0011, point 6; docs/verifica/casi.md).
namespace tg::script {

/// How far an obtained result may be from the expected one. Changed only
/// by "!tolerance" in the files written by whoever writes the references.
struct Tolerances {
    /// Two numbers are equal if they differ by at most this, in drawing
    /// units, or by at most `relative` times the larger of the two.
    double absolute{1e-9};
    double relative{0.0};
    /// Images (level 2): a pixel differs if one channel differs by more
    /// than `channel` out of 255, and a case fails if more than `pixels`
    /// percent of the pixels differ.
    int channel{8};
    double pixels{0.1};

    friend bool operator==(const Tolerances&, const Tolerances&) = default;
};

/// One difference between an expected and an obtained text.
struct Difference {
    int line;              ///< 1-based, in the expected file
    std::string expected;  ///< kNoLine if the expected text is shorter
    std::string actual;    ///< kNoLine if the obtained text is shorter

    friend bool operator==(const Difference&, const Difference&) = default;
};

/// Stands for a line missing on one side of a Difference.
inline constexpr std::string_view kNoLine = "(no line)";

/// True if the two numbers are equal within the tolerances.
[[nodiscard]] bool numbersMatch(double expected, double actual, const Tolerances& tolerances);

/// True if the two lines are the same except for numbers equal within the
/// tolerances. A number is a run of `[+-] digits [. digits] [(e|E) [+-]
/// digits]` not preceded by a letter, a digit, '_' or '.': in "x2" or
/// "R10" the digits are text. The text between numbers must be identical.
[[nodiscard]] bool linesMatch(std::string_view expected, std::string_view actual,
                              const Tolerances& tolerances);

/// Compares two reports line by line; numbers are equal within the
/// tolerances, everything else must match exactly. Line endings (LF or
/// CRLF), a final line ending and a UTF-8 byte order mark do not count.
/// Returns every difference, in line order; empty if the reports match.
[[nodiscard]] std::vector<Difference> compareReports(std::string_view expected,
                                                     std::string_view actual,
                                                     const Tolerances& tolerances);

/// Compares a report obtained up to the frontier with the expected one
/// (docs/adr/0037, point 6). The sections of `actual` before its last one
/// ([report 1] ... [report k]) are compared line by line with the expected
/// sections of the same titles; the lines of the "messages:" list of the
/// last section of `actual` must be the first lines of the "messages:" list
/// of the expected section after [report k]. Nothing else is compared. The
/// header line is compared. Returns the differences, with the lines of the
/// expected file; an expected section missing is one difference.
[[nodiscard]] std::vector<Difference> compareReportPrefix(std::string_view expected,
                                                          std::string_view actual,
                                                          const Tolerances& tolerances);

/// The lines of `text` without their endings (LF or CRLF), without a UTF-8
/// byte order mark at the start. A final line ending does not start an
/// empty line: "a\n" is one line, "a\n\n" two, the second empty.
[[nodiscard]] std::vector<std::string_view> splitLines(std::string_view text);

}  // namespace tg::script
