// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
#include "tg/core/hatch.hpp"

#include <gtest/gtest.h>

#include <string_view>

namespace tg::io {
namespace {

TEST(DxfHatch, TheCodecIsRegistered) {
    const EntityCodec* codec = findCodec(std::string_view{"HATCH"});
    ASSERT_NE(codec, nullptr);
    EXPECT_EQ(codec, findCodec(core::Entity{core::Hatch{}}));
}

}  // namespace
}  // namespace tg::io
