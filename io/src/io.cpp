// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/io.hpp"

#include "tg/io/dxf_groups.hpp"

#include <string>
#include <string_view>

namespace tg::io {
namespace {

Error notImplemented(std::string_view what, std::string_view package) {
    return Error{Error::Kind::NotImplemented, 0,
                 std::string{what} + " is not implemented yet: it arrives with package " +
                     std::string{package} + " of milestone 0.5"};
}

}  // namespace

Result<ReadResult> readDxf(std::string_view bytes) {
    // The groups are already read, so that the fuzzing target exercises the
    // group reader before the drawing is built (P10).
    const Result<GroupFile> groups = readGroups(bytes);
    if (!groups.ok()) {
        return groups.error();
    }
    return notImplemented("building a drawing from DXF groups", "P10");
}

Result<std::string> writeDxf(const core::Document& /*document*/, DxfVersion /*version*/) {
    return notImplemented("writing DXF files", "P10");
}

Result<ReadResult> readCadx(std::string_view /*bytes*/) {
    return notImplemented("reading .cadx files", "P14");
}

Result<std::string> writeCadx(const core::Document& /*document*/) {
    return notImplemented("writing .cadx files", "P14");
}

}  // namespace tg::io
