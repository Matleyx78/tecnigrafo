# [0.8.1-PT13] `app`: barra dei formati dell'editor

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: test Qt e prova manuale
- **Dipende da:** PT12
- **Modulo:** `app`
- **Da leggere:** [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md),
  punti 1 e 5; [ADR 0040](../../adr/0040-icone-a-ruoli.md) e
  [ICONE.md](../../ICONE.md); la scheda [PT12](PT12-editor.md)

## File

- `app/src/text_format_bar.hpp`, `app/src/text_format_bar.cpp` (nuovi)
- `app/src/text_editor_overlay.cpp` (solo: mostra e nasconde la barra)
- `resources/icons/` (le icone della barra, a coppie 16/32)
- `app/CMakeLists.txt` (file comune)
- `tests/app/text_format_bar_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. Sopra l'editor, una barra con: grassetto, corsivo, sottolineato,
   sopralineato, barrato; altezza, colore (indice o vero), fattore di larghezza,
   obliquo, spaziatura; impila (con la scelta che contiene `/`, `#` o `^`) e
   disimpila. Ogni comando chiama `applyFormat` di PT12 sulla scelta, o sul
   punto del cursore per ciò che si scrive dopo.
2. Lo stato dei pulsanti segue il formato sotto il cursore.
3. Scorciatoie `Ctrl+B`, `Ctrl+I`, `Ctrl+U` nell'editor.

## Regole della tappa che valgono qui

- Icone a ruoli, contate a coppie 16/32 ([DECISIONI.md](../../DECISIONI.md),
  2026-11-07).

## Test richiesti

- Ogni pulsante produce il codice dell'ADR 0054, punto 1, sul testo scelto; lo
  stato dei pulsanti dentro un grassetto.

## Fatto quando

- I test passano; la CI è verde.
