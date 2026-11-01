# [0.7-P20] `cmd`: `FILLET`, `CIRCLE` *Ttr*

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R20]` (#…), agente gemini
- **Dipende da:** P18, P19, P25, R20
- **Corretta:** 2026-11-01, con le decisioni del responsabile sui buchi B6, B14
  e B15 della #464 ([DECISIONI.md](../../DECISIONI.md))
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 13, 14, 16, 21, 22 e 23; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 8; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md) punto 6;
  `core/include/tg/core/fillet.hpp` (P19), `core/include/tg/core/tolerance.hpp`
  (P14); `cmd/src/commands/circle_command.cpp`;
  [dxf-di-prova.md](dxf-di-prova.md) ("Che cosa resta da controllare", punti 2
  e 4)

## File

- `cmd/include/tg/cmd/commands/fillet_command.hpp`,
  `cmd/src/commands/fillet_command.cpp` (nuovi)
- `cmd/src/commands/circle_command.cpp`,
  `cmd/include/tg/cmd/commands/circle_command.hpp`
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults::filletRadius`,
  `trimMode`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/fillet_command_test.cpp` (nuovo), `tests/cmd/circle_command_test.cpp`,
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R20.txt` (da cancellare)

## Cosa fare

**`FILLET` (`F`).** Prima il messaggio
`Current settings: Mode = TRIM, Radius = <r>`, poi:

```
Select first object or [Undo/Polyline/Radius/Trim/Multiple]:
Select second object or shift-select to apply corner or [Radius]:
Specify fillet radius <{default}>:
Enter Trim mode option [Trim/No trim] <Trim>:
Select 2D polyline or [Radius]:
```

- Gli oggetti sono richieste di oggetto (`PromptKind::Object`). `Shift` sul
  secondo vale raggio 0. Il raggio è una richiesta di distanza: negativo dà
  `Value must be positive.` e la domanda resta; 0 vale lo spigolo (ADR 0036,
  punto 6). Un raggio oltre il dominio di solid-space si accetta come gli
  altri: è `fillet` a dare `None` (P19, ADR 0035 punto 23), e nessun controllo
  in più nel comando.
- Il risultato viene da `fillet`, `filletSegments` (due punti sulla stessa
  polilinea) o `filletPolyline` (*Polyline*), con `ToleranceRule::of` del
  documento:
  - le entità cambiano con `replace`; un `first` o un `second` vuoto con
    `Done` (P19, B14) cancella quell'entità invece di sostituirla, nella
    stessa operazione, con un solo punto di ripristino: nell'esempio di P19
    le due linee si cancellano e resta solo l'arco;
  - *Trim* vale solo fra entità distinte (B6): su due segmenti della stessa
    polilinea e con *Polyline* la polilinea cambia sempre, anche con *Trim
    No*;
  - l'arco nuovo con `addCopy` dalla prima entità se le due hanno le stesse
    proprietà, altrimenti con `add` e le proprietà correnti (ADR 0035, punto
    16);
  - `None`, `InfinitelyMany`, `AmbiguousPick` e `NotAllowed`: il messaggio
    inglese di AutoCAD (da controllare, ADR 0035 punto 22) e nessuna modifica;
  - `OutOfDomain` (due rette quasi parallele, il centro del raccordo o lo
    spigolo oltre il dominio di solid-space): `Lines are nearly parallel.` e
    nessuna modifica. Il messaggio è di Tecnigrafo, nello stile di quelli di
    AutoCAD (`Lines are parallel.` di `CHAMFER`): il caso nasce dal dominio di
    solid-space, e la prova in AutoCAD non lo copre;
  - *Polyline* scrive `<n> lines were filleted` e, se ce ne sono,
    `<m> were too short` (da controllare come sopra); i vertici rimasti per
    `OutOfDomain` sono tra questi, e i contatori seguono la visita dei vertici
    originali in ordine (P19, B15).
- *Multiple* ripete finché non si preme Invio; *Undo* torna all'ultimo punto di
  ripristino.
- **Sequenze:**
  - interattiva: dopo *Radius* e *Trim* torna la prima domanda;
  - degli script: dopo *Radius* e *Trim* il comando finisce (ADR 0032, punto 8).
- Un passo di annulla `Fillet`.

**`CIRCLE` *Ttr*:** l'opzione `Ttr` nella prima domanda di `CIRCLE`, poi:

```
Specify point on object for first tangent of circle:
Specify point on object for second tangent of circle:
Specify radius of circle <{default}>:
```

Le due domande sono richieste di oggetto; il cerchio viene da
`circleTangentTangentRadius`; senza soluzione (anche con un raggio oltre il
dominio, che dà `None`), `Circle does not exist.` e il comando finisce senza
disegnare; con `OutOfDomain`, `Lines are nearly parallel.` e il comando finisce
allo stesso modo.

## Regole della tappa che valgono qui

- Prima di cominciare controlla nella issue l'esito della prova a mano sul layer
  dell'arco ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da
  controllare", punto 2). Se smentisce l'ADR 0035, punto 16, fermati.
- Allo stesso modo controlla l'esito della prova di `FILLET` con `TRIMMODE` = 0
  su due segmenti di una stessa polilinea e con *Polyline* (punto 4). Se
  AutoCAD lascia la polilinea com'è e aggiunge il raccordo come entità a sé,
  fermati: la decisione B6 si rivede prima di questo pacchetto.
- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P18, dopo di te P21.
- Le descrizioni elencano le domande di entrambe le sequenze (ADR 0032, punto 9).
- Nessuna decisione geometrica propria: tutto viene da `fillet.hpp`.

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `FILLET` e *Ttr*, nelle due sequenze.
- Raggio 0, `Shift`, parallele, *Polyline*, *Trim No* (anche su due segmenti di
  una polilinea e con *Polyline*), *Multiple*, *Undo*, impossibile, punto
  ambiguo, proprietà dell'arco; le due linee che si cancellano (l'esempio di
  B14), con un solo passo di annulla; annulla e ripeti.
- Rette quasi parallele (i dati di P19) in `FILLET`, anche con `Shift`, e in
  *Ttr*: il messaggio e nessuna modifica; un raggio oltre il dominio (2e15):
  il messaggio del raccordo impossibile e `Circle does not exist.`.

## Fatto quando

- I casi di `[0.7-R20]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
