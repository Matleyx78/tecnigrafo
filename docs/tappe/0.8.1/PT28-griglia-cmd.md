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

Il responsabile registra in AutoCAD LT (seduta prima dell'apertura): le
cronologie di `GRID`, `SNAP` e `UCSICON` con ogni opzione, i messaggi di `F7` e
`F9`. Le richieste qui sotto si correggono con le parole tradotte di quelle
cronologie. Lo snap alla griglia con orto, polare e tracciamento non si prova
in AutoCAD: vale la regola di REGOLE-NUMERICHE.md
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
   (`<10.0000,10.0000>`), i limiti di AutoCAD (`GRIDMAJOR` da 1 a 100, passi
   > 0; `GRIDUNIT` accetta 0,0).
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
  toccata; con l'orto e con il polare.

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
- **Fonte:** le cronologie «da registrare» di `GRID`, `SNAP`, `UCSICON` e la
  prova con orto e polare (sopra); i DXF `autocad-griglia-*` per i valori delle
  variabili.
- **Risposte condivise:** nessuna: con lo snap spento, che è il valore di un
  disegno nuovo e di tutti i DXF dei casi di oggi (controllato su
  `dxf-test-file`), `grid snap:` non si scrive e i punti non cambiano. Da
  controllare nel pacchetto di riferimenti: le sorgenti di `libdxfrw` usate da
  sei script (`ac1015-lwpolyline`, `ac1015-3dface`).
