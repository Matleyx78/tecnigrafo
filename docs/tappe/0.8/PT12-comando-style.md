# [0.8-PT12] `edit` e `cmd`: `-STYLE`, `TEXTSTYLE` e `TEXT` con lo stile corrente

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT12]` (#…), agente claude
- **Dipende da:** PT7, PT11, RT12
- **Modulo:** `cmd` (più le operazioni sulla tabella in `edit`)
- **Da leggere:** [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  punto 1; [ADR 0026](../../adr/0026-descrizione-dei-comandi.md);
  `cmd/src/commands/layer_commands.cpp` (come è fatto `-LAYER`),
  `cmd/src/commands/text_command.cpp`; la scheda [PT7](PT7-tabelle-edit.md);
  la cronologia di `-STYLE` copiata dal responsabile in AutoCAD LT
  ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `edit/include/tg/edit/text_styles.hpp`, `edit/src/text_styles.cpp` (nuovi)
- `cmd/include/tg/cmd/commands/style_commands.hpp`,
  `cmd/src/commands/style_commands.cpp` (nuovi)
- `cmd/src/commands/text_command.cpp`
- `cmd/src/builtin_commands.cpp`, `edit/CMakeLists.txt`, `cmd/CMakeLists.txt`
  (file comuni)
- `tests/cmd/style_commands_test.cpp` (nuovo), `tests/cmd/text_command_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `edit/text_styles.*`: `putTextStyle` e `setCurrentTextStyle`, annullabili,
   sopra `DocumentTables::replaceTextStyles` (PT7).
2. **`-STYLE`** (alias `STYLE`), con le richieste di AutoCAD LT copiate dalla
   cronologia del responsabile; quelle attese:
   - `Enter name of text style or [?] <Standard>:` — `?` elenca gli stili;
   - `Specify full font name or font filename (TTF or SHX) <arial.ttf>:`
   - `Specify height of text or [Annotative] <0.0000>:` — *Annotative* non c'è:
     la richiesta mostra solo `Specify height of text <0.0000>:`;
   - `Specify width factor <1.0000>:`, `Specify obliquing angle <0>:`,
     `Display text backwards? [Yes/No] <No>:`,
     `Display text upside-down? [Yes/No] <No>:`;
   - alla fine `"<nome>" is now the current text style.`

   Se le richieste di AutoCAD LT sono diverse, valgono quelle di AutoCAD, e la
   pull request lo dice. I valori predefiniti di ogni richiesta sono quelli
   dello stile se esiste già. Un passo di annullamento: `Style`.
3. **`TEXTSTYLE`**: `Enter new value for TEXTSTYLE <"Standard">:`; un nome che
   non c'è dà `Cannot find text style "<nome>".`
4. **`TEXT`** usa lo stile corrente: lo scrive nel testo; se lo stile ha
   altezza fissa non chiede l'altezza; fattore di larghezza e obliquo dello
   stile vanno nel testo nuovo.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `edit/CMakeLists.txt`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `style_commands_test.cpp`: creare uno stile, cambiarne uno, `?`, `U`;
  `TEXTSTYLE` con un nome che non c'è.
- `text_command_test.cpp`: con uno stile ad altezza fissa `TEXT` salta
  l'altezza; fattore di larghezza e obliquo copiati.

## Fatto quando

- I casi di RT12 passano; la CI è verde.
