# [0.8.1-PT11] `cmd`: `TEXTEDIT` e la richiesta dell'editor

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT11]` (#…), agente claude
- **Dipende da:** PT3, PT4, RT11
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md),
  punti 5 e 6;
  [ADR 0056](../../adr/0056-comandi-degli-attributi.md), punto 4;
  [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md) (testo sostitutivo);
  `cmd/src/commands/mtext_command.cpp`, `dimension_commands.cpp`,
  `dimension_angular_commands.cpp`, `cmd/include/tg/cmd/command_protocol.hpp`

## File

- `cmd/include/tg/cmd/command_protocol.hpp` (`Prompt::mtextEditing`)
- `cmd/include/tg/cmd/commands/textedit_command.hpp`,
  `cmd/src/commands/textedit_command.cpp` (nuovi)
- `cmd/src/commands/mtext_command.cpp` (la richiesta del contenuto porta il
  `MText` da modificare)
- `cmd/src/commands/dimension_commands.cpp`,
  `cmd/src/commands/dimension_angular_commands.cpp` (*Mtext*)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/textedit_command_test.cpp` (nuovo), `tests/cmd/mtext_command_test.cpp`,
  `tests/cmd/dimension_commands_test.cpp`, `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `Prompt` ha `std::optional<core::MText> mtextEditing`: quando c'è, il
   programma apre l'editor nella finestra (PT12) su quel testo invece della riga
   di comando. Lo mettono: la richiesta delle righe di `MTEXT` (con i due angoli
   e le opzioni già date), `TEXTEDIT` su un `MTEXT` o su una quota, *Mtext* delle
   quote. Negli script e al livello 1 non cambia niente: la richiesta resta di
   testo.
2. **`TEXTEDIT`** (alias `ED`, anche `DDEDIT`), con le richieste dell'ADR 0054,
   punto 6: `Select an annotation object or [Undo/Mode]:`; *Mode* `Enter a text
   edit mode option [Single/Multiple] <Multiple>:`; *Undo* annulla l'ultima
   modifica del comando (`Nothing to undo.` se non ce n'è); in *Multiple* la
   scelta si ripete fino a Invio. Accetta un oggetto già scelto (pickfirst),
   come il doppio clic lo darà (PT12). Su `TEXT`, `MTEXT`, quota, `ATTDEF`,
   `INSERT` con attributi le richieste del punto 6; su un altro oggetto `Object
   is not an annotation.` (o il messaggio della cronologia). Ogni oggetto
   modificato è un passo `Textedit`.
3. ***Mtext*** delle quote (l'opzione la porta PT3): la sua richiesta `Enter
   dimension text <misura>:` ha `mtextEditing`, con il testo sostitutivo di
   ora; la richiesta di testo non cambia.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.
- La forma da riga di comando di `TEXTEDIT` è di Tecnigrafo, dentro `TEXTEDIT`,
  come l'ADR 0054, punto 6 ([DECISIONI.md](../../DECISIONI.md), 2026-11-22,
  «Decisioni D1–D10 del piano della 0.8.1», punto 2).

## Test richiesti

- `TEXTEDIT` su ogni tipo, *Single* e *Multiple*, *Undo*, Invio che tiene il
  valore, un oggetto non annotativo, un oggetto su layer bloccato.

## Fatto quando

- I casi di RT11 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/textedit/testi/script.tgs
TEXT
0,0
2.5
0
Primo
MTEXT
0,-10
60,-20
riga uno\Priga due

DIMLINEAR
0,-40
30,-40
0,-45
TEXTEDIT
!click 0.5,0.5
Corretto
!click 1,-10.5
riga corretta
!click 15,-45
<> mm

!report
```

Righe da controllare: in `entities:` il `TEXT` con `value="Corretto"`, il
`MTEXT` con il valore nuovo, la quota con il testo sostitutivo; in `undo:` tre
passi `Textedit`.

- **Strumenti:** `!click`, righe normali, `!report`, su `main`.
- **Dipendenze:** `TEXT`, `MTEXT`, `DIMLINEAR` su `main`. Per `ATTDEF` e
  `INSERT` con attributi, un secondo caso apre con `OPEN` un DXF del caso
  scritto a mano (un `ATTDEF` nel disegno e un blocco con due `ATTDEF`
  *Preset*, che `-INSERT` non chiede nemmeno con `ATTREQ` 1, PT17) e lo
  inserisce con `-INSERT`: tutto su `main`, senza `-ATTDEF` (PT14).
- **Risposte condivise:** nessuna: le richieste delle quote cambiano con PT3
  (che aggiorna i 21 casi), e `mtextEditing` non si vede negli script.
