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
  `dimension_angular_commands.cpp`, `cmd/include/tg/cmd/command.hpp`,
  `cmd/include/tg/cmd/input.hpp`;
  [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punto 5 (protocollo dei
  comandi senza il disegno)

## File

- `cmd/include/tg/cmd/text_editing.hpp` (nuovo: `TextEditRequest`)
- `cmd/include/tg/cmd/command.hpp` (file comune: `Command::textEditing()`)
- `cmd/include/tg/cmd/command_runner.hpp` (file comune: l'accessore in linea
  `CommandRunner::textEditing()`)
- `cmd/src/command_runner.cpp` (solo la consegna a `TEXTEDIT` di un unico
  oggetto già scelto come `ObjectInput` all'avvio; eccezione decisa dal
  responsabile nella issue #939)
- `cmd/include/tg/cmd/input.hpp` (solo il commento di `TextInput`: può essere
  vuoto quando viene dall'editor)
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

1. **Richiesta dell'editor**, nel disegno 2D e non nel protocollo:
   `command_protocol.hpp` non conosce il disegno (ADR 0041, punto 5), quindi
   `Prompt` non cambia (verbale della revisione delle interfacce PT4, #927,
   [bloccante] 5). In `text_editing.hpp`:

   ```cpp
   // What the editor in the window opens on (docs/adr/0054, point 5), at the
   // Text prompt of the command that gives it.
   struct TextEditRequest {
       // The entity edited; nullopt while it does not exist yet: a new MTEXT,
       // the Mtext option of a dimension being created.
       std::optional<core::EntityId> target;
       std::variant<core::Text, core::MText> shown;  // what the editor draws, where it stands
       std::string value;  // the value to edit, codes kept
       // For the text of a dimension: what its first <> stands for, already
       // formatted (dimensionText() of "<>", DIMPOST included); otherwise
       // nullopt.
       std::optional<std::string> measurement;
   };
   ```

   e `Command` ha `[[nodiscard]] virtual std::optional<TextEditRequest>
   textEditing() const { return std::nullopt; }`; `CommandRunner::textEditing()`
   dà quello del comando in corso (vuoto senza comando, o mentre chiede una
   selezione). Quando c'è, il programma apre l'editor (PT12) invece della riga
   di comando. Lo danno:
   - la richiesta delle righe di `MTEXT` (con i due angoli e le opzioni già
     date): `target` vuoto, `shown` il `MText` che nascerà, `value` vuoto;
   - `TEXTEDIT` su un `TEXT`: `shown` il `Text` (posizione, rotazione, stile:
     l'editor a una riga sta sul testo), `value` il suo valore; su un `MTEXT`:
     `shown` il `MText`, `value` il suo valore;
   - `TEXTEDIT` su una quota e *Mtext* delle quote: `shown` il `MTEXT` della
     grafica della quota, quello che si vede (con la misura al posto di `<>`),
     e `value` il testo sostitutivo con `<>` (`<>` se è vuoto), separati: con
     `<> mm` la grafica mostra `30 mm` ma l'editor riceve `<> mm`, così la
     misura resta dinamica. `measurement` è il testo che sostituisce `<>`,
     già formattato (`dimensionText` di `"<>"`, con `DIMPOST`), anche quando
     il valore di partenza non ha `<>`: così l'editor lo mostra appena si
     scrive `<>` (verbale del secondo giro della #993, [bloccante] 1).
     Nell'editor la misura si vede, ma è un'unità sola legata alla prima
     occorrenza di `<>` (PT10). `target` è l'identificativo della quota con
     `TEXTEDIT`, vuoto con *Mtext* durante la creazione, quando la quota non
     esiste ancora (verbale della rilettura della #993, [bloccante] 2).

   Negli script e al livello 1 non cambia niente: la richiesta resta di testo.
2. **`TEXTEDIT`** (alias `ED`, anche `DDEDIT`), con le richieste dell'ADR 0054,
   punto 6: `Current settings: Edit mode = Multiple`, `Select an annotation
   object or [Undo/Mode]:` (in *Single* senza *Undo*: `Select an annotation
   object or [Mode]:`); *Mode* `Enter a text edit mode option [Single/Multiple]
   <Multiple>:`, e la modalità resta per i `TEXTEDIT` successivi della
   sessione; *Undo* annulla l'ultima modifica del comando e scrive `The command
   has been completely undone.`, anche quando non c'è niente da annullare; in
   *Multiple* la scelta si ripete fino a Invio. Accetta un oggetto già scelto
   (pickfirst), come il doppio clic lo darà (PT12). Su `TEXT`, `MTEXT`, quota,
   `ATTDEF`, `INSERT` con attributi le richieste del punto 6; un altro oggetto
   non dà messaggi e la domanda si ripete, come AutoCAD. Ogni oggetto
   modificato è un passo `Textedit`.
   **Valore vuoto confermato dall'editor**, come AutoCAD (prova in AutoCAD del
   responsabile, 2026-11-22, che corregge la decisione sulla #927, [decisione]
   4): un `TEXT` o un `MTEXT` esistente svuotato e confermato **torna com'era**,
   senza un passo `Textedit`; una quota svuotata **torna alla misura** (testo
   sostitutivo `""`). L'editor consegna il valore con
   `CommandRunner::submit(TextInput{…})`, anche vuoto, mentre una riga vuota
   scritta resta `ConfirmInput` (Invio), che tiene il valore come dice l'ADR
   0054, punto 6. Alla richiesta del testo un `TextInput` vuoto su un `TEXT` o
   un `MTEXT` esistente non cambia niente e non fa un passo, come
   `ConfirmInput`; su una quota dà il testo sostitutivo vuoto. Nel `MTEXT`
   nuovo un valore vuoto non crea niente, come oggi. Svuotato vuol dire senza
   niente di visibile, anche se restano codici nascosti: è l'editor (PT12) che
   lo riconosce con il modello di PT10 e consegna `ConfirmInput` per `TEXT` e
   `MTEXT`, `TextInput{""}` per una quota; il comando riceve solo questi due
   ingressi (decisione del responsabile sulla rilettura della #994).
  Per un `ATTDEF`, un'etichetta con spazi o vuota, compreso `TextInput{""}`
  dall'editor, scrive `Invalid tag name.` e ripete la richiesta dell'etichetta,
  senza creare un passo; decisione del responsabile nella issue #939.
3. ***Mtext*** delle quote (l'opzione la porta PT3): alla sua richiesta `Enter
   dimension text <misura>:` il comando dà `textEditing()`, con il testo
   sostitutivo di ora e la grafica della quota in costruzione; la richiesta di
   testo non cambia.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `cmd/src/command_runner.cpp` (solo la consegna a `TEXTEDIT` di un unico
  oggetto pickfirst come `ObjectInput`, eccezione decisa nella issue #939),
  `tests/CMakeLists.txt`, `cmd/include/tg/cmd/command.hpp` (solo
  `textEditing()`; `highlighted()` lo aggiunge PT34),
  `cmd/include/tg/cmd/command_runner.hpp` (solo l'accessore).
- La forma da riga di comando di `TEXTEDIT` è di Tecnigrafo, dentro `TEXTEDIT`,
  come l'ADR 0054, punto 6 ([DECISIONI.md](../../DECISIONI.md), 2026-11-22,
  «Decisioni D1–D10 del piano della 0.8.1», punto 2).

## Test richiesti

- `TEXTEDIT` su ogni tipo, *Single* e *Multiple*, *Undo*, Invio che tiene il
  valore, un oggetto non annotativo, un oggetto su layer bloccato.
- `textEditing()` (#927, [bloccante] 5): `MTEXT` nuovo e *Mtext* di una quota in
  costruzione (`target` vuoto), `TEXTEDIT` su una quota esistente (`target` la
  quota), `TEXTEDIT` su `TEXT` (il `Text` e il suo valore), su `MTEXT`, e su una
  quota lineare lunga 30 con testo `<> mm`: `shown` mostra `30 mm`, `value` è
  `<> mm`, `measurement` è `30`; *Mtext* in creazione con il testo `FISSO` già
  dato: `value` è `FISSO` e `measurement` c'è lo stesso; `measurement` vuoto su
  `TEXT` e `MTEXT`; *Mtext* delle quote; nessuna richiesta dell'editor su
  `ATTDEF` e `INSERT`, né mentre si sceglie l'oggetto.
- Valore vuoto (prova in AutoCAD del 2026-11-22): `TextInput{""}` su un
  `TEXT` e su un `MTEXT` con il valore `PROVA` li lascia com'erano, senza passo
  `Textedit`; su una quota con `<> mm` dà il testo sostitutivo vuoto (la
  misura), in un passo; `ConfirmInput` alla stessa richiesta tiene il valore;
  un `MTEXT` con valore `""` resta nel disegno.

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
- **Fonte:** la seduta del 2026-11-22 ([dxf-di-prova.md](dxf-di-prova.md)):
  richieste, *Mode*, *Undo* (con e senza una modifica da annullare) e il clic su
  una linea, che non dà messaggi.
- **Risposte condivise:** nessuna: le richieste delle quote cambiano con PT3
  (che aggiorna i 21 casi), e `textEditing()` non si vede negli script.
