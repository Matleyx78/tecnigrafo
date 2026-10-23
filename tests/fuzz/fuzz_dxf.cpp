// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/io.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    // libFuzzer owns these bytes; the reader must accept arbitrary binary data.
    const std::string_view bytes{reinterpret_cast<const char*>(data), size};
    (void)tg::io::readDxf(bytes);
    return 0;
}
