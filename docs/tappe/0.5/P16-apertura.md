# [0.5-P16] `cmd`: `NEW`, `OPEN`, modifiche non salvate

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #175
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.5-R16]` (#174), agente gemini
- **Dipende da:** P15, R16
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0021](../../adr/0021-comandi-dei-file.md) punti 1–3,
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punto 3
  (messaggio d'apertura); `cmd/include/tg/cmd/commands/file_commands.hpp` e
  `cmd/src/commands/file_commands.cpp` (P15), `command.hpp`,
  `command_runner.cpp`, `edit/include/tg/edit/editor.hpp`
  (`replaceDocument`, `isModified`)

## File

- `cmd/include/tg/cmd/command.hpp` (`CommandContext` riceve
  `bool documentReplaced{false};`)
- `cmd/src/command_runner.cpp`
- `cmd/include/tg/cmd/commands/file_commands.hpp`
- `cmd/src/commands/file_commands.cpp`
- `cmd/src/builtin_commands.cpp`
- `tests/cmd/file_commands_test.cpp`
- `tests/verifica/in-attesa.txt` (togliere le righe di R16)

## Cosa fare

### Modifiche non salvate

Una parte comune a `NEW` e `OPEN`, che si esegue per prima solo se
`editor.isModified()`:

`Save changes to <nome>? [Yes/No/Cancel] <Yes>:`, con le parole chiave
`Yes` (`Y`), `No` (`N`), `Cancel` (`C`).

- `Yes` (anche Invio vuoto): salva come `QSAVE` di P15, con le sue domande se
  il disegno non ha un file; se il salvataggio fallisce o si annulla, il
  comando finisce e il disegno resta.
- `No`: si prosegue senza salvare.
- `Cancel` o `Esc`: il comando finisce senza cambiare niente.

### `NEW`

Dopo la domanda (se serve): `editor.replaceDocument(core::Document{}, "")`:
disegno vuoto `Drawing1.cadx`, unità 4.

### `OPEN`

1. La domanda sulle modifiche, se serve.
2. `Enter name of drawing to open:` (testo, `FilePrompt::Open`, nessun valore
   predefinito; Invio vuoto ripete la domanda).
3. Estensione `.cadx` → `io::readCadx`, `.dxf` → `io::readDxf`, senza
   distinguere maiuscole; altra o nessuna →
   `Cannot open <nome>: only .cadx and .dxf files are supported.`
4. Errore di lettura o di `FileAccess`: `Cannot open <nome>: <messaggio>`
   (per esempio `Cannot open plan.dxf: line 812: …`); il disegno corrente
   resta com'era.
5. Riuscita: `editor.replaceDocument(documento, nome)`; poi, se ci sono
   entità non gestite, `<n> entities not supported: HATCH 2, INSERT 1`
   (tipi in ordine alfabetico; `1 entity not supported: …` al singolare);
   poi ogni avviso della lettura su una riga sua.

### Runner

Quando un comando ha messo `documentReplaced`, alla sua fine il runner
svuota la selezione e il rettangolo di selezione, dimentica l'ultimo punto
(`lastPoint`) e rimette `documentReplaced` a falso. Annulla e ripeti non
attraversano l'apertura (le pile sono vuote, P5).

## Regole della tappa che valgono qui

- Domande e messaggi in inglese, con le parole di AutoCAD.
- I file comuni con P15 (`file_commands`, `builtin_commands.cpp`) sono ormai
  tuoi: P15 è unito.

## Test richiesti

Con un `FileAccess` di prova in memoria:

- `NEW` e `OPEN` senza modifiche: nessuna domanda di salvataggio.
- Con modifiche: `Yes` con e senza file corrente, `No`, `Cancel`, `Esc`;
  salvataggio fallito.
- `OPEN` di un `.dxf` e di un `.cadx`; estensione sbagliata; file mancante;
  file danneggiato; `.cadx` di una versione più recente (il messaggio di
  P14).
- Messaggio delle entità non gestite, singolare e plurale; avvisi.
- Dopo l'apertura: selezione vuota, pile vuote, `isModified()` falso, nome
  corrente quello aperto; `QSAVE` salva nel file aperto.

## Fatto quando

- I casi di `[0.5-R16]` passano, al livello 1 e al livello 2, e le loro righe
  sono tolte da `tests/verifica/in-attesa.txt`.
- Un file danneggiato produce un messaggio, mai un crash, e il disegno
  aperto prima resta.
- La CI è verde.
