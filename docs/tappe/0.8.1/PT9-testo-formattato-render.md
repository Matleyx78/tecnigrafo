# [0.8.1-PT9] `render` e `app`: il testo formattato nel disegno

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nelle immagini; test con il
  pittore di prova di `render` e un test Qt del pittore, prova manuale
- **Dipende da:** PT4, PT8
- **Modulo:** `render` (più il pittore Qt di `app` e le risorse dei font)
- **Da leggere:** [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md),
  punti 2 e 4; `render/include/tg/render/painter.hpp`,
  `render/src/generated_drawing.cpp`, `app/src/qt_painter.cpp`,
  `app/CMakeLists.txt` (le risorse); la scheda [PT8](PT8-impaginazione-formati.md)

## File

- `render/include/tg/render/painter.hpp` (il testo con la variante)
- `render/src/generated_drawing.cpp`
- `app/src/qt_painter.hpp`, `app/src/qt_painter.cpp`
- `app/CMakeLists.txt` (file comune: le tre risorse nuove)
- `tests/render/generated_drawing_test.cpp`, `tests/app/qt_painter_test.cpp`

## Cosa fare

1. `TextRun` (e quindi `Painter::drawText`) riceve la `FontFace` e la
   spaziatura (`tracking`) del `FormattedText` di PT8; un `Text` dà Regular e 1.
   Il pittore di prova le registra (verbale della revisione delle interfacce
   PT4, #927, [bloccante] 1).
2. `QtPainter` carica i quattro file di `resources/fonts` da `:/tg/fonts/` e
   disegna ogni pezzo con la sua variante, il suo colore (già nelle proprietà
   effettive), il suo obliquo e la sua spaziatura: con `tracking` ≠ 1 ogni
   carattere si mette all'ascissa data dagli avanzamenti della variante
   moltiplicati per `tracking`, senza allargare il glifo (che allarga solo il
   fattore di larghezza). Le righe di decorazione e delle frazioni sono già
   `Line`.
3. Sotto l'altezza minima in pixel il pezzo si disegna come il suo riquadro,
   come oggi, ma con `formattedTextBox` di PT8 (variante e spaziatura), non con
   il riquadro Regular del suo `Text`.
4. Nessun cambiamento alle misure: il disegno usa le posizioni di `core`.

## Regole della tappa che valgono qui

- File comuni: `app/CMakeLists.txt`.

## Test richiesti

- Pittore di prova: un `MTEXT` con un pezzo grassetto e uno corsivo registra
  le due varianti; `{\T2;AB}` registra la spaziatura 2 e `{\W2;AB}` il
  fattore 2 con spaziatura 1; a zoom basso un pezzo Bold dà il riquadro di
  `formattedTextBox`; `QtPainter` trova le quattro famiglie.

## Fatto quando

- I test passano; la CI è verde; nella prova manuale `autocad-mtext-formati.dxf`
  si vede con grassetto, corsivo, colori, decorazioni e frazioni.
