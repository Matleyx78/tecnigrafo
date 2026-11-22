# [0.8.1-PT28] `cmd` e `script`: `GRID`, `SNAP`, `UCSICON` e lo snap alla griglia

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT28]` (#…), agente codex, fascia forte
  (nodi, pareggi, orto e polare)
- **Dipende da:** PT5, PT27, PT34, RT28
- **Modulo:** `cmd` (più il campo `grid snap:` di `script`)
- **Da leggere:** [ADR 0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  punti 3, 4 e 7;
  [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 3, punto 6, e
  sezione 4, «Orto e polare», punto 3; `cmd/src/command_runner.cpp`
  (`aidedPoint`), `cmd/src/commands/aid_commands.cpp` (le variabili come
  comandi); la scheda [PT27](PT27-stato-del-disegno.md)

## Prima del pacchetto

Le cronologie di `GRID`, `SNAP`, `UCSICON`, `F7` e `F9` sono registrate
(seduta in AutoCAD 2021 del 2026-11-22, [dxf-di-prova.md](dxf-di-prova.md)): le
richieste e i messaggi del punto 1 sono le loro parole tradotte. Lo snap alla
griglia con orto, polare e tracciamento non si prova in AutoCAD: vale la regola
di REGOLE-NUMERICHE.md
([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano
della 0.8.1», punto 3).

## File

- `cmd/src/commands/grid_commands.cpp`,
  `cmd/include/tg/cmd/commands/grid_commands.hpp` (nuovi: `GRID`, `SNAP`,
  `UCSICON`, `GRIDMODE`, `GRIDUNIT`, `SNAPMODE`, `SNAPUNIT`, `SNAPBASE`,
  `GRIDDISPLAY`, `GRIDMAJOR`)
- `cmd/src/command_runner.cpp` (`aidedPoint`: il nodo)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `script/src/report.cpp` (`grid snap:`)
- `tests/cmd/grid_commands_test.cpp` (nuovo), `tests/cmd/command_runner_test.cpp`,
  `tests/script/report_test.cpp`, `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. I comandi dell'ADR 0055, punto 3, che cambiano `GridSettings` del disegno
   corrente senza annullamento e lo segnano modificato; le variabili con `Enter
   new value for <NOME> <valore>:`, i punti come `x,y` con quattro decimali
   (`<10.0000,10.0000>`). Come AutoCAD:
   - **`GRID`**: un passo scritto, o Invio, accende la griglia; `2x` vuol dire
     due volte il passo dello snap; `0` mette `GRIDUNIT` a 0,0, come *Snap*; un
     passo negativo dà `Value must be positive.` e la domanda si ripete.
     *Aspect*: `Specify the horizontal spacing(X) <…>:` e `Specify the vertical
     spacing(Y) <…>:`; con i due passi diversi la proposta della domanda diventa
     `<A>`. *Major*: `Enter number of grid divisions per major grid line <5>:`,
     fuori da 1–100 `Requires an integer between 1 and 100.` e la domanda si
     ripete. *aDaptive*: `Turn adaptive behavior on [Yes/No] <Yes>:` (la
     proposta è lo stato attuale); *Yes* chiede `Allow subdivision below grid
     spacing [Yes/No] <No>:` (bit 4); *No* spegne il bit 2 e, regola nostra
     dove la seduta non arriva, anche il 4 (la suddivisione vale solo con
     l'adattiva, ADR 0055, punto 5).
   - **`SNAP`**: un passo scritto, o Invio, accende lo snap; `0` o un passo
     negativo danno `Value must be positive and nonzero.` e la domanda si
     ripete. *Aspect*: `Specify horizontal spacing <…>:` e `Specify vertical
     spacing <…>:` (senza `(X)` e `(Y)`), accende lo snap, poi la proposta è
     `<A>`.
   - **`UCSICON`**: la proposta è lo stato attuale (`<ON>` o `<OFF>`) e Invio lo
     lascia; *OFF* dà 2, *ON* 3, *Noorigin* 1, *ORigin* 3.
   - **Variabili**: `GRIDMAJOR` fuori da 1–100 come *Major*; `GRIDUNIT`
     accetta 0,0 e rifiuta un passo negativo, `SNAPUNIT` rifiuta 0 e i negativi,
     tutte e due con `Cannot set <NOME> to that value.` e `*Invalid*`, e il
     comando finisce (AutoCAD accetta un `GRIDUNIT` negativo senza messaggi:
     [DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Esiti della seconda
     seduta in AutoCAD della 0.8.1», punto 7).
2. **Snap alla griglia** in `aidedPoint`, per i punti indicati (clic e
   puntatore della preview), non per quelli scritti: prima lo snap a oggetto
   (se trova un punto vince), poi il nodo (REGOLE-NUMERICHE.md, sezione 3,
   punto 6), poi tracciamento, polare e orto sul nodo (sezione 4, «Orto e
   polare», punto 3, e «Tracciamento degli snap»); l'incrocio di due guide
   resta il suo punto. Con `gridUnit` 0,0 il passo dello snap.
3. Il campo `grid snap: unit=x,y base=x,y`, dopo `ortho:`, solo con lo snap
   acceso (annunciato da PT2).
4. `F7` e `F9` sono dell'interfaccia (PT32): qui nessun tasto.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`. `command_runner.cpp` lo cambiano anche PT34 (prima) e
  PT31 (dopo).
- Pareggi e riduzioni: [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md),
  sezione 3, punto 6; sezione 4, «Orto e polare», punto 3, e «Tracciamento
  degli snap».

## Test richiesti

- Il nodo per punti negativi, a metà fra due nodi, con `SNAPBASE` 3,4 e passi
  diversi in X e Y; uno snap a oggetto che vince; una coordinata scritta non
  toccata; con l'orto e con il polare; ogni richiesta e ogni messaggio del
  punto 1, con i valori rifiutati.

## Fatto quando

- I casi di RT28 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/griglia/snap-alla-griglia/script.tgs
!osnap none
SNAPUNIT
2.5,5
SNAPBASE
3,4
SNAPMODE
1
LINE
!click 4.1,6.4
!click 9.9,13.6
-1.2,0.3

!report
GRIDUNIT

GRID
OFF
GRIDMODE

```

Righe da controllare: la `LINE` dal nodo al nodo e la seconda alla coordinata
scritta intatta; `grid snap: unit=2.5,5 base=3,4` nel `[report 1]`; le eco
`Enter new value for GRIDUNIT <10.0000,10.0000>:` e `Enter new value for
GRIDMODE <0>:`.

- **Strumenti:** righe normali, `!click`, `!osnap none`, `!report` su `main`;
  il campo `grid snap:` dell'onda 0 (PT2).
- **Dipendenze:** PT27 (lo stato).
- **Fonte:** le cronologie di `GRID`, `SNAP`, `UCSICON`, `F7` e `F9` della
  seduta del 2026-11-22 (sopra); i DXF `autocad-griglia-*` per i valori delle
  variabili; con orto,
  polare e tracciamento la regola di REGOLE-NUMERICHE.md (decisione D3).
- **Risposte condivise:** nessuna: con lo snap spento, che è il valore di un
  disegno nuovo e di tutti i DXF dei casi di oggi (controllato su
  `dxf-test-file`), `grid snap:` non si scrive e i punti non cambiano. Da
  controllare nel pacchetto di riferimenti: le sorgenti di `libdxfrw` usate da
  sei script (`ac1015-lwpolyline`, `ac1015-3dface`).
