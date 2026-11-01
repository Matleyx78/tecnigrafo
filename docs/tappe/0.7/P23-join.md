# [0.7-P23] `cmd`: `JOIN`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R23]` (#…), agente gemini
- **Dipende da:** P21, P22, P25, R23
- **Corretta:** 2026-11-01, con le decisioni del responsabile sui buchi B11 e
  B16 della #464 ([DECISIONI.md](../../DECISIONI.md))
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 17–22; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punto 7;
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punti 3 e 8; `core/include/tg/core/join.hpp` (P22),
  `core/include/tg/core/tolerance.hpp` (P14);
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-unione.dxf`, messaggi osservati;
  "Che cosa resta da controllare", punti 5 e 6)

## File

- `cmd/include/tg/cmd/commands/join_command.hpp`,
  `cmd/src/commands/join_command.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/join_command_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R23.txt` (da cancellare)

## Cosa fare

**Sequenza interattiva:** una richiesta di selezione (`PromptKind::Selection`)
con la domanda `Select source object or multiple objects to join at once:`, poi:

- il primo oggetto scelto è la sorgente, gli altri seguono nell'ordine in cui
  sono stati scelti (ADR 0035, punto 18);
- `join` decide, con `ToleranceRule::of` del documento;
- la sorgente cambia con `replace`, oppure nasce un'entità con `addCopy` dalla
  sorgente e la sorgente si cancella;
- le entità assorbite (`consumed`) si cancellano; le altre scelte restano e
  contano tra le scartate del messaggio, anche quelle che una scelta mista
  lascia fuori dalla catena (P22, B16).

**Sequenza degli script:**

```
Select source object:
Select lines to join to source:
Select arcs to join to source or [cLose]:
Select objects to join to source:
```

- La seconda domanda dipende dal tipo della sorgente: linee, archi o oggetti
  per le polilinee.
- Per gli archi la scelta è un oggetto per clic, ripetuta fino a Invio; *cLose*
  chiude l'arco in un cerchio. Per gli altri tipi è una richiesta di selezione
  (`PromptKind::Selection`).
- `joinToSource` decide, con la regola del documento.

**Messaggi**, come quelli di AutoCAD in `autocad-unione.dxf`, in inglese (da
controllare, ADR 0035 punto 22):

- `<n> lines joined into 1 line`;
- `<n> objects converted to 1 polyline`;
- `<n> segments added to polyline`;
- `0 objects joined, <n> objects discarded from the operation.`

Un passo di annulla `Join`.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P21.
- Le descrizioni elencano le domande di entrambe le sequenze (ADR 0032, punto 9).
- Nessuna decisione geometrica propria: tutto viene da `join.hpp`.
- Prima di cominciare controlla nella issue l'esito delle prove a mano di
  `JOIN` ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da
  controllare", punti 5 e 6): P22 si ferma se smentiscono le decisioni B11 e
  B16, e questo pacchetto ne dipende.

## Test richiesti

- I casi di `autocad-unione.dxf`, ognuno nella sequenza con cui è stato fatto
  (a, b, c negli script; gli altri interattivi).
- Sorgente polilinea nelle due sequenze; arco con *cLose*; niente da unire;
  una scelta mista (l'esempio di B16 in P22), con la linea staccata che resta;
  annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R23]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
