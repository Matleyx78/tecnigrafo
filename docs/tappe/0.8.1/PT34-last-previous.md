# [0.8.1-PT34] `cmd`: *Last*, *Previous*, `SELECT`, i prefissi `#` e `*`, l'evidenziazione dei comandi

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT34]` (#…), agente codex
- **Dipende da:** PT5, RT34
- **Modulo:** `cmd` (più il filtro dei tasti del campo dinamico in `app`)
- **Da leggere:** [ADR 0008](../../adr/0008-selezione-e-snap.md);
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md);
  [ADR 0042](../../adr/0042-immissione-dinamica.md);
  `cmd/src/command_runner.cpp` (`typedSelection`), `cmd/src/selection_set.cpp`,
  `cmd/src/coordinate_input.cpp`, `cmd/src/dynamic_input.cpp`; la cronologia
  `autocad-selezione.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## Perché insieme

Tre voci piccole che passano tutte da `command_runner.cpp`, il file che
cambiano anche PT28 e PT31: insieme fanno una sola tappa della catena invece di
tre. `Command::highlighted()` serve a PT26, che così non tocca il runner.

## File

- `cmd/src/command_runner.cpp`, `cmd/include/tg/cmd/command_runner.hpp`
- `cmd/include/tg/cmd/command.hpp` (`virtual std::span<const core::EntityId>
  highlighted() const`, vuoto per difetto)
- `cmd/src/selection_set.cpp`, `cmd/include/tg/cmd/selection_set.hpp`
- `cmd/src/coordinate_input.cpp`, `cmd/src/dynamic_input.cpp`
- `cmd/src/commands/select_command.cpp` (nuovo: `SELECT`)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `app/src/dynamic_input_box.cpp` (il campo dinamico lascia passare `#` e `*`
  come primo carattere; verbale della revisione PT5, [bloccante] 10)
- `tests/app/dynamic_input_test.cpp`
- `tests/cmd/command_runner_test.cpp`, `tests/cmd/coordinate_input_test.cpp`,
  `tests/cmd/dynamic_input_test.cpp`, `tests/cmd/select_command_test.cpp`
  (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. ***Last*** (`L`) a una richiesta di oggetti: l'entità visibile (layer acceso
   e scongelato) con l'identificativo più alto, se c'è; sui layer bloccati vale
   la regola di oggi (`1 was on a locked layer.`). `ERASE` *Last* ripetuto
   prende ogni volta l'ultima rimasta.
2. ***Previous*** (`P`): il **gruppo di selezione precedente** del documento,
   cioè gli oggetti dell'ultima richiesta di oggetti finita (di qualunque
   comando, anche `SELECT`), senza quelli cancellati; dopo un `U` che annulla il
   comando, il gruppo resta quello del comando annullato (cronologia). Senza
   gruppo: `No previous selection set.`. Il gruppo è del documento e non entra
   nell'annullamento.
   Una richiesta finita **senza oggetti** (Invio subito, o annullata) non
   cambia il gruppo; gli oggetti **preselezionati** con cui parte un comando
   che chiede oggetti diventano il gruppo, come se fossero stati scelti nella
   richiesta (revisione PT5, [decisione] 6). Un disegno nuovo o aperto non ha
   gruppo, anche in un runner che riusa la sessione e anche con gli stessi id
   (il gruppo è del documento: [decisione] 6, regola esistente).
3. **`SELECT`**: una richiesta di oggetti; alla fine il gruppo diventa il
   precedente e il comando non cambia il disegno.
4. **`#x,y` e `*x,y`**: coordinate assolute, anche scritte nella riga di
   comando; con l'immissione dinamica, che rende relativi i numeri scritti
   nel campo, `#` e `*` li rendono assoluti, come AutoCAD. Il filtro dei tasti
   del campo (`DynamicInputBox::eventFilter`) oggi prende solo cifre, punto,
   segno e virgola: deve prendere anche `#` e `*` come primo carattere del
   campo, così il testo arriva a `cmd` come scritto ([bloccante] 10).
5. ***All*** dà gli oggetti **dal più recente** (id decrescente), come AutoCAD,
   così `-BLOCK` con `ALL` ordina gli `ATTDEF` all'inverso della creazione
   (ADR 0056, punto 1; [bloccante] 13): `defineBlock()` tiene l'ordine che
   riceve.
6. `Command::highlighted()` e `CommandRunner::highlighted()`, che dà quello del
   comando in corso o niente.

## Regole della tappa che valgono qui

- File comuni: `cmd/include/tg/cmd/command.hpp`, `cmd/src/builtin_commands.cpp`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`. `command_runner.cpp` lo cambiano
  dopo PT28 e PT31.

## Test richiesti

- La cronologia di `autocad-selezione`; *Last* con l'ultima entità su un layer
  spento; *Previous* dopo `ERASE`; `#10,10` dopo un punto, con l'immissione
  dinamica accesa e spenta.
- Revisione PT5: `SELECT` A, Invio; `SELECT`, Invio a vuoto; `MOVE` `P` dà A
  (`1 found`); `SELECT` A, poi B preselezionata e `MOVE`; `MOVE` `P` dà B
  ([decisione] 6); `NEW` nello stesso runner e `P`: `No previous selection
  set.`.
- `#10,10` e `*10,10` **digitati nel campo dinamico** con il focus nella vista
  e un punto base già posto (test Qt e uno script di livello 2), oltre alla
  riga di comando ([bloccante] 10).
- `-BLOCK` con `ALL` dopo una `LINE` e poi un `CIRCLE`: nella definizione
  prima il `CIRCLE`, poi la `LINE` ([bloccante] 13; con gli `ATTDEF`, che
  arrivano con PT14, vale lo stesso ordine).

## Fatto quando

- I casi di RT34 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/selezione/last-previous/script.tgs
LINE
0,0
30,0

CIRCLE
50,0
5
LINE
0,20
30,20

ERASE
L

SELECT
W
-5,-10
60,10

MOVE
P

0,0
0,-10
!report
U
SELECT
P

ERASE
W
40,-20
60,0

SELECT
P
```

Righe da controllare: in `messages:` `1 found` e `2 found` come la cronologia
(tradotti), `No previous selection set.` alla fine; `entities:` dopo
`[report 1]`.

- **Strumenti:** righe normali, `!report`, su `main`.
- **Dipendenze:** `ERASE`, `MOVE`, `LINE`, `CIRCLE` su `main`; `SELECT` è di
  questo pacchetto (fino ad allora `Unknown command`: è il confine del caso,
  ADR 0037, punto 4.2).
- **Fonte:** `autocad-selezione.txt`, compreso il gruppo di annullamento
  lasciato aperto da un `SELECT` *Previous* andato a vuoto (per questo è in
  fondo).
- **Risposte condivise:** nessuna: `L` e `P` a una richiesta di oggetti oggi
  sono un punto non valido, e nessun caso li scrive lì (controllato:
  `L`/`P` nei casi di oggi sono opzioni di altri comandi). L'ordine di *All*
  lo usano tre casi (`erase/selezione-scritta-all`,
  `erase/selezione-scritta-bloccati`, `explode/non-scomponibili`), che non
  dipendono dall'ordine: cancellano, o contano le entità non scomponibili
  (controllato).
