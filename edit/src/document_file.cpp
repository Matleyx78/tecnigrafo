// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/document_file.hpp"

#include <utility>

namespace tg::edit {

void DocumentFile::setSaved(std::string name) {
    m_name = std::move(name);
    m_hasFile = true;
}

void DocumentFile::reset(std::string untitled) {
    m_name = std::move(untitled);
    m_hasFile = false;
}

}  // namespace tg::edit
