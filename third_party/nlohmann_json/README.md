<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->

# nlohmann/json

Libreria JSON del modulo `mcp`, ammessa dall'[ADR 0027](../../docs/adr/0027-server-mcp-di-sviluppo.md)
(punto 2) e copiata come zlib ([ADR 0019](../../docs/adr/0019-archivi-zip.md)).
`README.md` e `.clang-format-ignore` sono i soli file nostri della cartella;
`json.hpp` e `LICENSE` sono di nlohmann/json e **non sono modificati**.

| | |
|---|---|
| Progetto | JSON for Modern C++, di Niels Lohmann, <https://github.com/nlohmann/json> |
| Versione | 3.12.0 (11 aprile 2025) |
| File originale | `json.hpp`, intestazione unica, dalla pagina del rilascio <https://github.com/nlohmann/json/releases/tag/v3.12.0> |
| SHA-256 di `json.hpp` | `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63` |
| Licenza | MIT, testo in [`LICENSE`](LICENSE) (`LICENSE.MIT` dell'archivio `json.tar.xz` dello stesso rilascio) |
| SHA-256 di `LICENSE` | `46a65cffd1ea955132d95a8dd921640714a8d6b537d2e4e482d31145ae95b603` |

Il `json.hpp` scaricato da solo coincide byte per byte con
`single_include/nlohmann/json.hpp` degli archivi `include.zip` e
`json.tar.xz` dello stesso rilascio.

## Come si compila

[`../CMakeLists.txt`](../CMakeLists.txt) ne fa il bersaglio d'interfaccia
`tg_nlohmann_json`, collegato **solo** da `tg_mcp`. La cartella è inclusa come
`SYSTEM`: non riceve gli avvisi del progetto né `clang-tidy`. Si include come
`#include "nlohmann_json/json.hpp"`, così il nome non si confonde con il
lettore JSON interno di `io` (`io/src/json.hpp`).

`.clang-format-ignore` tiene `json.hpp` fuori dal controllo del formato, come
per SARibbon.

## Aggiornare

Un aggiornamento è un pacchetto a sé: si sostituisce `json.hpp` con quello
della nuova versione, senza modificarlo, e si aggiornano versione e SHA-256
in questa pagina (ADR 0027, "Conseguenze").
