// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <string>

namespace tg::edit {

/// The file of an open document (docs/adr/0041, point 3). The editor
/// tracks whether the document is modified using its history.
class DocumentFile {
public:
    DocumentFile() = default;  ///< "Drawing1.cadx", no file

    /// The name, UTF-8: "Drawing1.cadx" until the first save or load.
    [[nodiscard]] const std::string& name() const { return m_name; }

    /// True once the document has been saved or loaded under a name.
    [[nodiscard]] bool hasFile() const { return m_hasFile; }

    /// After a save or a load: the document is now the file `name`.
    void setSaved(std::string name);

    /// A new document without a file, named `untitled`.
    void reset(std::string untitled);

private:
    std::string m_name{"Drawing1.cadx"};
    bool m_hasFile{false};
};

}  // namespace tg::edit
