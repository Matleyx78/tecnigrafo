# ADR 0002: Linguaggio, librerie e strumenti

- **Stato:** Accettato
- **Sostituito in parte da:** ADR 0028 (versione minima di Qt)
- **Data:** 2026-10-15

## Contesto

Il software deve funzionare su Windows, Linux e macOS, con prestazioni adatte a
disegni grandi, e in futuro usare Open CASCADE (C++) come kernel 3D.

## Decisione

| Ambito | Scelta | Versione minima |
|---|---|---|
| Linguaggio | C++20 | GCC 12, Clang 16, MSVC 2022 |
| Build | CMake + Ninja | CMake 3.24 |
| Interfaccia | Qt Widgets | Qt 6.4 |
| Test | GoogleTest | 1.14 (scaricata: 1.15.2) |
| Formattazione | clang-format | 18 |
| Analisi statica | clang-tidy | 18 |
| CI | GitHub Actions | Windows, Linux, macOS |

Kernel 3D previsto: Open CASCADE Technology 8.x (LGPL 2.1 con eccezione,
compatibile con la GPLv3), da introdurre con un ADR dedicato.

## Alternative considerate

- **Rust:** sicuro e moderno, ma nessun kernel CAD maturo e binding Qt immaturi.
- **Python come linguaggio principale:** troppo lento per il cuore del programma;
  potrà essere aggiunto in futuro come linguaggio di scripting.
- **Qt Quick/QML:** adatto alle app touch, meno a un'interfaccia CAD densa di
  barre, pannelli e finestre di proprietà.

## Conseguenze

- Ogni cambio di versione minima o nuova dipendenza richiede un nuovo ADR.
- Le versioni di clang-format e clang-tidy sono fissate perché versioni diverse
  formattano in modo diverso.
