# [0.5-P15] `cmd`: `QSAVE`, `SAVE`, `SAVEAS`, `INSUNITS`

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.5-R15]` (#…), agente gemini
- **Dipende da:** P5, P11, P14, R15
- **Modulo:** `cmd` (più i collegamenti in `script`, `app` e nella verifica)
- **Da leggere:** [ADR 0021](../../adr/0021-comandi-dei-file.md) (tutto),
  [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punto 2 (versione scritta),
  [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md) (domande di
  testo); `cmd/include/tg/cmd/command.hpp`, `command_runner.hpp`,
  `cmd/src/commands/text_command.cpp` (una domanda di testo già fatta),
  `edit/include/tg/edit/editor.hpp` (P5), `io/include/tg/io/io.hpp` e
  `file_access.hpp`

## File

- `cmd/include/tg/cmd/command.hpp` (`CommandContext` riceve
  `io::FileAccess* files{nullptr};`, `Prompt` riceve `FilePrompt file`)
- `cmd/include/tg/cmd/command_runner.hpp`
- `cmd/src/command_runner.cpp`
- `cmd/include/tg/cmd/commands/file_commands.hpp` (nuovo)
- `cmd/src/commands/file_commands.cpp` (nuovo)
- `cmd/src/builtin_commands.cpp`
- `cmd/CMakeLists.txt` (`tg_cmd` collega `Tecnigrafo::io`; il commento sugli
  strati)
- `script/src/level1.cpp` (una riga: `runner.setFileAccess(options.files)`)
- `app/src/script_session.cpp` (in `--script`, un `script::CaseFiles` con la
  cartella d'uscita e `--sources`)
- `app/src/main.cpp` (opzione `--sources`, valida solo con `--script`)
- `tests/verifica/confronta_immagini.cpp` (passa `--sources` al programma)
- `tests/verifica/CMakeLists.txt` (passa `--sources` a `confronta_immagini`)
- `tests/cmd/file_commands_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R15)

## Cosa fare

### Runner

```cpp
/// Where file commands read and write (docs/adr/0021, point 4). Not owned;
/// nullptr (the default) makes them answer "File access is not available.".
void setFileAccess(io::FileAccess* files);
```

### Domande dei nomi di file

```cpp
/// What a text prompt asks for, so that the application can show a file
/// dialog instead (docs/adr/0021, point 2). Commands set it; the runner
/// does not use it.
enum class FilePrompt : std::uint8_t { None, Open, Save };
```

`Prompt` riceve `FilePrompt file{FilePrompt::None};`; le domande del nome di
`QSAVE`, `SAVE` e `SAVEAS` lo mettono a `Save` (P16 mette `Open` a quella di
`OPEN`).

### Comandi

In `file_commands.hpp/.cpp`, una classe per comando, registrate in
`builtin_commands.cpp` in ordine alfabetico: `INSUNITS`, `QSAVE`, `SAVE`,
`SAVEAS`. Le domande sono di testo (`PromptKind::Text`): la riga scritta è il
nome intero, spazi compresi; le virgolette intorno si tolgono; Invio vuoto
accetta il valore tra `< >`.

| Comando | Comportamento |
|---|---|
| `QSAVE` | se `editor.hasFile()`, salva subito nel file corrente; altrimenti chiede `Save drawing as <Drawing1.cadx>:` e si comporta come `SAVEAS` |
| `SAVE` | chiede `Save drawing as <nome corrente>:`; salva con quel nome; il nome corrente e `isModified()` cambiano solo se il nome è quello corrente |
| `SAVEAS` | chiede `Save drawing as <nome corrente>:`; salva; il nome dato diventa quello corrente (`markSaved`) |
| `INSUNITS` | chiede `Enter new value for INSUNITS <4>:` (il valore attuale); un intero da 0 a 24 si applica in una transazione `INSUNITS`; altro: `Requires an integer between 0 and 24.` e la domanda resta |

- **Formato** dall'estensione, senza distinguere maiuscole: `.cadx` →
  `io::writeCadx`; `.dxf` → `io::writeDxf`; nessuna estensione → si aggiunge
  `.cadx`; altra → `Cannot save plan.dwg: only .cadx and .dxf files are
  supported.` e il comando finisce.
- **Versione del DXF**: quella letta quando si salva nel file corrente un
  DXF aperto (`QSAVE`, o `SAVE` con il nome corrente); `R2018` negli altri
  casi (ADR 0017, punto 2).
- **File esistente** (non per il file corrente con `QSAVE`):
  `A drawing with this name already exists. Do you want to replace it? [Yes/No] <No>:`
  con le parole chiave `Yes` (`Y`) e `No` (`N`); `No` torna alla domanda del
  nome.
- **Errori** (di `io` o di `FileAccess`): `Cannot save <nome>: <messaggio>`
  sulla cronologia; il comando finisce e il disegno resta com'era.
- Il salvataggio riuscito non scrive messaggi: lo stato si vede nel titolo
  della finestra (P17) e nel campo `drawing:` del rapporto.
- `Esc` a ogni domanda annulla il comando senza salvare.

### Collegamenti

- Livello 1: `setFileAccess` in `script/src/level1.cpp`, con il `FileAccess`
  di `RunOptions` (P8).
- Livello 2: ogni caso con uno script gira anche con `tecnigrafo --script`
  (`verifica2/…`), quindi anche i casi dei file. In `script_session.cpp` il
  programma usa un `script::CaseFiles` (P8) con la cartella di `--output` e
  quella di `--sources`; `confronta_immagini` e `tests/verifica/CMakeLists.txt`
  passano `--sources`. Fuori da `--script` l'interfaccia non ha ancora un
  `FileAccess`: lo darà P17 con le finestre di dialogo.

## Regole della tappa che valgono qui

- Domande e messaggi in inglese, scritti per intero, con le parole di AutoCAD
  (ADR 0005, ADR 0011 punto 3).
- `cmd` non usa Qt; non apre file da solo: solo attraverso `FileAccess`.
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `cmd/src/commands/file_commands.cpp` li tocca anche P16, dopo: solo le tue
  righe ora.

## Test richiesti

Con un `FileAccess` di prova in memoria:

- `QSAVE` di un disegno nuovo (chiede il nome), poi di nuovo (non chiede).
- `SAVE` con un altro nome: il nome corrente non cambia e il disegno resta
  modificato; con il nome corrente: diventa salvato.
- `SAVEAS` in `.dxf` e in `.cadx`; estensione mancante e sbagliata.
- DXF aperto (letto con `io`) e salvato nel suo file: stessa versione; con
  `SAVEAS`: R2018.
- File esistente: `No` torna al nome, `Yes` sostituisce; `Esc` a ogni domanda.
- Errore di scrittura: messaggio, disegno invariato.
- `INSUNITS`: valori 0, 24, 25, `-1`, `abc`, Invio vuoto; annulla e ripeti.
- Senza `FileAccess`: il messaggio.

## Fatto quando

- I casi di `[0.5-R15]` passano, al livello 1 e al livello 2, e le loro righe
  sono tolte da `tests/verifica/in-attesa.txt`.
- I DXF scritti dai casi passano `ezdxf audit` (passo dell'oracolo).
- La CI è verde.
