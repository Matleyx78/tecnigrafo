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
- **Corretta:** 2026-11-01, con le decisioni del responsabile sulla #359 e sui
  buchi B1–B4 e B18 della #464 ([DECISIONI.md](../../DECISIONI.md))
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 3–8 e 21–23; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
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
  - i pezzi che restano si costruiscono con `partOf` dai tagli della lista,
    con il `PartCourse` che dà la lista (P15, decisione della #359): `Direct`
    tra due tagli in ordine, `AcrossStart` per il pezzo di un cerchio o di una
    polilinea chiusa che passa per l'inizio, `SamePlace` quando il taglio è
    uno solo e il pezzo che resta va da lui a se stesso. Il primo pezzo con
    `Editor::replace` (anche cerchio → arco), il secondo, se c'è, con
    `Editor::addCopy` dall'originale;
  - un pezzo `Empty`, o che `isDegenerate` trova degenere con
    `ToleranceRule::of` del documento, non si tiene; se nulla resta, l'oggetto
    si cancella. Un pezzo di polilinea è degenere quando **tutti** i suoi
    segmenti lo sono, ciascuno giudicato con `isDegenerate(const
    PolylineSegment&, rule)` (B4): nessuna soglia nuova; `isDegenerate`
    sull'entità resta falso per le polilinee, e una catena di segmenti
    ciascuno sotto la tolleranza si scarta anche se nel complesso la supera di
    poco. Per esempio, in millimetri, [(0,0),(10,0),(10,10)] che fa da bordo a
    se stessa, con il bordo x=5e-7 e il clic sul tratto orizzontale verso
    x=1: il pezzetto [(0,0),(5e-7,0)] si scarta;
  - un pezzo `NotRepresentable` (P15, B1): l'operazione non riesce, l'oggetto
    resta com'è e compare il messaggio dell'operazione non riuscita (ADR
    0035, punto 23);
  - senza tagli, con `HostCuts::complete` vero: nel modo rapido l'oggetto si
    cancella; nel modo standard compare il messaggio inglese di AutoCAD (da
    controllare, ADR 0035 punto 22) e l'oggetto resta.
- **Prolungare** (`EXTEND`, o `TRIM` con `Shift`):
  - l'estremo da prolungare è il più vicino al clic (confronto delle distanze
    del clic dai due estremi; a pari distanza la fine);
  - i tagli vengono da `cutsAlong(oggetto, Extension::Extended, bordi)`, con
    i bordi senza l'oggetto stesso; il
    primo bordo è, per la fine, il primo taglio `AfterEnd` (dell'ultimo
    segmento non puntiforme, per una polilinea); per l'inizio, l'ultimo taglio
    `BeforeStart` di una linea o del primo segmento dritto non puntiforme,
    l'ultimo taglio `AfterEnd` di un arco o del primo segmento curvo non
    puntiforme (il resto del cerchio, percorso all'indietro dall'inizio). Il
    primo e l'ultimo segmento sono quelli di `Extension::Extended` (P14, B3);
  - l'entità prolungata viene da `extendTo` (P15, B2) con l'estremo e quel
    taglio, e cambia con `Editor::replace`; i vertici dei segmenti puntiformi
    tra quel segmento e l'estremo seguono il nuovo estremo (P15). Senza bordo
    resta com'è; con `NotRepresentable` l'operazione non riesce, come sopra;
  - cerchi, polilinee chiuse e testo non si prolungano.
- **Risposta incompleta** (B18): con `HostCuts::complete` falso, in `TRIM` e in
  `EXTEND`, nei due modi:
  - **nessun taglio** (o, per prolungare, nessun bordo da raggiungere):
    l'operazione non riesce, l'oggetto resta e compare il messaggio
    dell'operazione non riuscita (ADR 0035, punto 23). La regola del modo
    rapido «senza tagli l'oggetto si cancella» vale solo con `complete` vero;
  - **ci sono tagli:** si usano come con `complete` vero; i bordi che
    solid-space rifiuta non tagliano (ADR 0035, punto 23).

  Si usa solo l'interfaccia di P14: `intersections.hpp` non cambia. Per
  esempio la linea da (2^51,0) a (2^51+1,0), con il bordo x=0, resta, con il
  messaggio.
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
  `intersections.hpp`; il pezzo lo costruiscono `partOf` ed `extendTo`. Non
  usare stazioni per ordinare i tagli né per decidere lo stesso posto o il
  giro.
- Le descrizioni dei comandi elencano le domande di entrambe le sequenze
  (ADR 0032, punto 9).

## Test richiesti

- Ogni caso di `autocad-taglio.dxf` nei due modi.
- Taglio nel mezzo di linea, arco, segmento di polilinea; cerchio; polilinea
  chiusa; bordo tangente; bordo che si sovrappone; due bordi per lo stesso
  punto; pezzo che resta degenere, anche di polilinea (l'esempio di B4);
  oggetto senza tagli nei due modi; oggetto
  oltre il dominio senza tagli (`complete` falso) nei due modi; testo scelto.
- Prolungamento di linea, arco, polilinea aperta all'inizio e alla fine (anche
  con il segmento curvo, e con un segmento puntiforme all'estremo, l'esempio
  di B3); arco 0°→90° all'inizio fino a un bordo sull'asse Y (diventa
  270°→90°); cerchio (nulla); senza bordo.
- `Shift`, *mOde*, *Undo*, `Esc` dopo due tagli; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R16]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
