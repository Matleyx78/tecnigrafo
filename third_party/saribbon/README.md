<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->

# SARibbon

Ribbon widgets for `app`, approved by ADR 0009 and package 0.6-P3.
The upstream files are copied without changes, including their line endings.

| Item | Value |
|---|---|
| Project | <https://github.com/czyt1988/SARibbon> |
| Release | v2.9.5 (2026-09-18), latest published release when imported |
| Archive | <https://api.github.com/repos/czyt1988/SARibbon/tarball/v2.9.5> |
| Archive SHA-256 | `c08d5e6ba83bcc1992908374bb3bc3c5219edbe3ffc8fa0271ef0013c112f45c` |
| License | MIT; unmodified text in [LICENSE](LICENSE) |
| License source | <https://github.com/czyt1988/SARibbon/blob/v2.9.5/LICENSE> |

## Copied files

- `src/SARibbon.h` and `src/SARibbon.cpp`, the release's amalgamated files.
  The implementation already embeds its resources; no external resource
  files, examples, build scripts or frameless-window libraries are needed.
- The root `LICENSE`.

`README.md` and `.clang-format-ignore` are Tecnigrafo integration files.
The latter keeps the upstream sources out of our formatting policy without
changing formatting checks for first-party code.

## Build

`../CMakeLists.txt` builds `tg_saribbon` as a static library only when
`TG_BUILD_APP` is enabled, using Qt 6.8 or newer (ADR 0028) and Qt's AUTOMOC.
Its public headers are SYSTEM headers. Like zlib, this target does not receive
Tecnigrafo's warning-as-error policy or clang-tidy; the global sanitizer flags
still apply. MSVC reads the original UTF-8 sources with `/utf-8`.
Only `tg_app_ui` links this dependency; application sources do not use it yet.
No download takes place during CMake configuration.

Updates are separate packages: replace the upstream files without editing them,
record the release and archive checksum, and rerun the offscreen smoke test on
Linux, Windows and macOS.
