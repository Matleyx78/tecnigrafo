# [0.8.1-PT6] `core` e `tools`: le quattro varianti del font e le loro misure

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT6]` (#…), agente codex
- **Dipende da:** PT2, PT4, RT6
- **Modulo:** `core` (più `tools/fontmetrics` e le domande `text-width` e
  `font-metrics` di `script`)
- **Da leggere:** [ADR 0013](../../adr/0013-testo-e-font.md), punto 3;
  [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md), punto 2;
  `tools/fontmetrics/main.cpp`, `tools/CMakeLists.txt`,
  `core/src/text_metrics.cpp`, `script/src/text_questions.cpp`

## Materiale

I tre file `LiberationSans-Bold.ttf`, `LiberationSans-Italic.ttf` e
`LiberationSans-BoldItalic.ttf` dell'archivio 2.1.5 sono già in
`resources/fonts`, con i loro SHA-256 nel README (pull request del piano).

## File

- `tools/fontmetrics/main.cpp`, `tools/CMakeLists.txt`
- `core/include/tg/core/text_metrics.hpp`, `core/src/text_metrics.cpp`
- `script/src/text_questions.cpp`, `script/src/question_table.cpp` (file
  comune: le righe di `text-width` e `font-metrics`)
- `tests/core/text_metrics_test.cpp`, `tests/script/questions_test.cpp`
- `resources/fonts/README.md` (la frase «until package 0.8.1-PT6 they are not
  read» diventa la descrizione delle quattro tabelle), `docs/verifica/domande.md`
  (toglie l'annuncio)

## Cosa fare

1. `tools/fontmetrics` legge i quattro file e scrive una tabella per variante
   nello stesso `font_metrics.inc` (o in quattro file generati), con
   ascendente, discendente, avanzamenti e carattere mancante, ciascuna in unità
   dell'altezza delle maiuscole **del suo file**.
2. In `core`:

   ```cpp
   enum class FontFace : std::uint8_t { Regular, Bold, Italic, BoldItalic };
   [[nodiscard]] const FontMetrics& fontMetrics(FontFace face = FontFace::Regular);
   [[nodiscard]] double advanceWidth(char32_t codePoint, FontFace face = FontFace::Regular);
   [[nodiscard]] double textWidth(std::u32string_view text, FontFace face = FontFace::Regular);
   ```

   Il Regular dà esattamente i valori di oggi; `fontCharacter` sostituisce
   `⌀` con `Ø` per variante, se manca in quella variante.
3. `text-width "<t>" face=…` e `font-metrics face=…` (annunciati da PT2) con
   le quattro varianti; senza `face=` il Regular.

## Regole della tappa che valgono qui

- Le misure sono deterministiche sui tre sistemi: niente calcoli in virgola
  mobile nel generatore oltre alla divisione per l'altezza delle maiuscole,
  come oggi.

## Test richiesti

- Per ogni variante, gli avanzamenti di `A`, `a`, spazio e del carattere
  mancante; il Regular invariato.

## Fatto quando

- I casi di RT6 passano; la CI è verde sui tre sistemi.

## Prova a secco dei riferimenti

```text
# casi/font/varianti/domande.tgq
? font-metrics face=bold -> …
? text-width "Normale grassetto" face=bold -> …
? text-width "corsivo" face=italic -> …
? text-width "grassetto corsivo" face=bold-italic -> …
? text-width "Normale grassetto" -> …
```

- **Strumenti:** l'argomento `face=` dell'onda 0 (PT2). I valori si calcolano
  in modo indipendente con fontTools sui file di `resources/fonts`, come RT13
  della 0.8.
- **Dipendenze:** nessun comando; i TTF sono già nel repository.
- **Risposte condivise:** nessuna; senza `face=` le risposte di oggi non
  cambiano.
