# [0.7-P16] `cmd`: `TRIM`, `EXTEND`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R16]` (#…), agente gemini-argon (Gemini 4 Argon; valori
  da `autocad-taglio.dxf`; i punti che stanno nell'archivio dei casi di
  solid-space si prendono da lì)
- **Dipende da:** P1, P8, P15, P25, R16
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 3–8, 21 e 22; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punto 5; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 5, 6, 9 e
  11; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punti 1, 3 e 8; [dxf-di-prova.md](dxf-di-prova.md) (`autocad-taglio.dxf`, e
  "Che cosa resta da controllare", punto 1);
  `core/include/tg/core/intersections.hpp` (P14),
  `core/include/tg/core/tolerance.hpp` (P14),
  `core/include/tg/core/curve_parts.hpp` (P15)

## File

- `cmd/include/tg/cmd/commands/trim_extend_commands.hpp`,
  `cmd/src/commands/trim_extend_commands.cpp` (nuovi; anche `TRIMEXTENDMODE`)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults::trimExtendMode`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/trim_extend_commands_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R16.txt` (da cancellare)

## Cosa fare

**Modo rapido** (`TRIMEXTENDMODE` = 1, il valore all'avvio). Prima il messaggio
`Current settings: Trim mode = Quick, Projection=UCS, Edge=None`, poi:

```
Select object to trim or shift-select to extend or [mOde]:
Select object to trim or shift-select to extend or [mOde/Undo]:
```

**Modo standard**, e sempre nella sequenza degli script. Prima i messaggi
`Current settings: Projection=UCS, Edge=None, Mode=Standard` e
`Select cutting edges ...`, poi:

```
Select objects or [mOde] <select all>:
Select object to trim or shift-select to extend or [mOde]:
Select object to trim or shift-select to extend or [mOde/Undo]:
```

Per `EXTEND` le domande sono le stesse, con *extend* e *trim* scambiati (e
`Select boundary edges ...` al posto di `Select cutting edges ...`).

- **Bordi** (ADR 0035, punto 4):
  - nel modo rapido, tutte le linee, gli archi, i cerchi e le polilinee
    visibili;
  - nel modo standard, quelli scelti (`PromptKind::Selection`); con una scelta
    vuota, tutti come nel modo rapido;
  - il testo non è mai un bordo.
- **Tagliare** (`TRIM`, o `EXTEND` con `Shift`), con l'oggetto scelto
  (`PromptKind::Object`):
  - i tagli vengono da `cutsAlong(oggetto, Extension::None, bordi,
    hostCutsItself)`, con i bordi senza l'oggetto stesso e `hostCutsItself`
    vero quando l'oggetto è una polilinea tra i bordi: i suoi vertici e i suoi
    incroci la tagliano;
  - il punto del clic dà `nearestPosition`; il pezzo da togliere è tra l'ultimo
    taglio che non viene dopo il clic e il primo che viene dopo, confrontando
    prima il segmento e poi la stazione (ADR 0035, punto 5: è la scelta
    dell'utente, non una decisione geometrica); se il clic cade esattamente su
    un taglio, si toglie il pezzo che segue. Per un cerchio e una polilinea
    chiusa i tagli girano: il pezzo tra l'ultimo e il primo passa per l'inizio;
  - i pezzi che restano si costruiscono con `partOf` dai tagli: il primo con
    `Editor::replace` (anche cerchio → arco), il secondo, se c'è, con
    `Editor::addCopy` dall'originale;
  - un pezzo che `isDegenerate` trova degenere con `ToleranceRule::of` del
    documento non si tiene; se nulla resta, l'oggetto si cancella;
  - senza tagli: nel modo rapido l'oggetto si cancella; nel modo standard
    compare il messaggio inglese di AutoCAD (da controllare, ADR 0035 punto 22)
    e l'oggetto resta.
- **Prolungare** (`EXTEND`, o `TRIM` con `Shift`):
  - l'estremo da prolungare è il più vicino al clic (confronto delle distanze
    del clic dai due estremi; a pari distanza la fine);
  - i tagli vengono da `cutsAlong(oggetto, Extension::Extended, bordi)`, con
    i bordi senza l'oggetto stesso; il
    primo bordo è, per la fine, il primo taglio `AfterEnd` (dell'ultimo
    segmento, per una polilinea); per l'inizio, l'ultimo taglio `BeforeStart`
    di una linea o del primo segmento dritto, l'ultimo taglio `AfterEnd` di un
    arco o del primo segmento curvo (il resto del cerchio, percorso all'indietro
    dall'inizio);
  - l'entità prolungata viene da `partOf` con quel taglio e cambia con
    `Editor::replace`; senza bordo resta com'è;
  - cerchi, polilinee chiuse e testo non si prolungano.
- **Opzioni:** *mOde* chiede `Enter a trim mode option [Quick/Standard] <Quick>:`
  e cambia `TRIMEXTENDMODE`; *Undo* torna all'ultimo punto di ripristino.
- **`TRIMEXTENDMODE`** come comando: `Enter new value for TRIMEXTENDMODE <1>:`,
  0 o 1; altro dà `Requires an integer between 0 and 1.` e la domanda resta.
- **Annulla ed `Esc`:** ogni oggetto tagliato o prolungato è un'operazione, con
  un punto di ripristino; il comando è un passo di annulla `Trim` o `Extend`;
  `Esc` conserva le operazioni fatte (ADR 0015).

## Regole della tappa che valgono qui

- Prima di cominciare controlla nella issue l'esito della prova a mano del modo
  rapido ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da controllare",
  punto 1). Se smentisce l'ADR 0035, punto 4, fermati: si corregge prima l'ADR.
- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P8, dopo di te P18.
- Nessuna decisione geometrica propria: dove si taglia e in che ordine viene da
  `intersections.hpp`; il pezzo lo costruisce `partOf`. Non usare stazioni per
  ordinare i tagli.
- Le descrizioni dei comandi elencano le domande di entrambe le sequenze
  (ADR 0032, punto 9).

## Test richiesti

- Ogni caso di `autocad-taglio.dxf` nei due modi.
- Taglio nel mezzo di linea, arco, segmento di polilinea; cerchio; polilinea
  chiusa; bordo tangente; bordo che si sovrappone; due bordi per lo stesso
  punto; pezzo che resta degenere; oggetto senza tagli nei due modi; testo
  scelto.
- Prolungamento di linea, arco, polilinea aperta all'inizio e alla fine (anche
  con il segmento curvo); cerchio (nulla); senza bordo.
- `Shift`, *mOde*, *Undo*, `Esc` dopo due tagli; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R16]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
