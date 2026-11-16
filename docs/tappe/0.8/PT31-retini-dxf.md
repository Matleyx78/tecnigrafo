# [0.8-PT31] `io`: `HATCH` nel DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT31]` (#…), agente claude
- **Dipende da:** PT30, RT31
- **Modulo:** `io`
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punto 1;
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md), punti 3–5;
  `io/src/dxf_lwpolyline.cpp`; la scheda [PT30](PT30-retini-core.md)

## File

- `io/src/dxf_hatch.cpp` (nuovo), `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp` (file comuni)
- `tests/io/dxf_hatch_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)
- `tests/verifica/oracolo/ezdxf_attesi.py` (solo l'elenco dei tipi attivi)
- `tests/verifica/casi/**/domande.tgq` (solo le righe `? count` e
  `? unhandled`, copiate dal log dell'oracolo)

## Cosa fare

1. Codec `HATCH` con i gruppi dell'ADR 0048, punto 1, **nell'ordine del DXF**,
   che è rigido: 10/20/30 (punto di quota), 210/220/230, 2, 70, 71, 91 e i
   contorni (92; per la polilinea 72, 73, 93, vertici 10/20 e 42; per i lati
   93, e per ogni lato 72 con 10/20/11/21 o 10/20/40/50/51/73; dopo ogni
   contorno 97 e gli handle 330 in `extras` del contorno), 75, 76, 52, 41, 77,
   78 e le righe (53, 43, 44, 45, 46, 79, 49…), 47, 98 e i punti interni, e il
   resto in coda (`extras`).
2. **Grezzi** (ADR 0048, punto 1): lati d'ellisse (72 = 3) o spline (4), sfumatura
   (450 diverso da 0), contorni non chiusi, estrusione diversa da `(0,0,1)` e
   `(0,0,-1)`. **La chiusura la controlla la lettura**, non `supported`
   ([DECISIONI.md](../../DECISIONI.md), 2026-11-16, «Retini: chiusura dei
   contorni, domande dal documento, cerchio dei contorni a lati»): un retino
   resta grezzo se `supported` (PT30, punto 1: una polilinea senza il segnale
   di chiusa, un contorno senza vertici o senza lati) è falso, oppure se un
   suo contorno non è chiuso secondo `isClosed(loop, ToleranceRule::of(document))`
   di PT30 (punto 1), con il documento che si sta leggendo: la sua unità è
   già quella di `$INSUNITS`, letta dall'intestazione prima delle entità (0 se
   manca, con la regola dei millimetri: `tolerance.hpp`).
3. **Angoli** ([DECISIONI.md](../../DECISIONI.md), 2026-11-12, «Angoli in
   gradi ridotti a un giro»): i gruppi 52 (angolo del motivo) e 53 (angolo di
   ogni riga) entrano nel modello con `reducedDegreesToRadians` di PT1, come
   gli angoli degli archi letti dal DXF (`dxf_circle_arc.cpp`), e si
   riscrivono ridotti: un 53 = 3690 torna 90.
4. Un `HATCH` letto e scritto senza modifiche resta identico (ADR 0018, punto
   6: i valori del modello sono gli stessi, anche con gli angoli ridotti).
5. Aggiungere `HATCH` all’elenco dei tipi della 0.8 attivi nell’oracolo ezdxf e,
   nella stessa pull request, aggiornare dal log dell’oracolo le righe `count` e
   `unhandled` di tutti i casi che cambiano. Questa è l'eccezione a
   [pacchetto.md](../../agenti/pacchetto.md),
   «In un pacchetto di lavoro», e all'ADR 0011 (casi scritti da un altro
   agente), decisa il 2026-11-16 in [DECISIONI.md](../../DECISIONI.md).

## Regole della tappa che valgono qui

- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `dxf_hatch_test.cpp`: giro di un `SOLID` con una polilinea e un cerchio
  (lato d'arco intero), di un `ANSI31` con contorno a lati e due punti interni;
  un retino con un lato d'ellisse resta grezzo; estrusione `(0,0,-1)`;
  un `SOLID` con il contorno a lati `0,0→10,0`, `10,0→10,8`, `10,8→0,8`,
  `0,8→0.0000005,0` (scarto 5e-7 fra l'ultimo lato e il primo): con
  `$INSUNITS` 4 (o senza `$INSUNITS`) è gestito, con `$INSUNITS` 6 (metri,
  tolleranza 1e-9) resta grezzo e si riscrive identico; lo stesso con
  l'ultimo lato che finisce in `0,1` resta grezzo in ogni unità;
  un `NET` con 52 = 3600 e righe con 53 = 3600 e 3690 (dieci giri più quelli
  di 0 e 90): si legge con angolo `+0` e righe a `+0` e a
  `degreesToRadians(90)`, si riscrive con 0, 0 e 90; lo stesso con 52 = −3600
  e 53 = −3600 e −3510.

## Fatto quando

- I casi di RT31 passano, compresi quelli di `autocad-retini.dxf`.
- La CI è verde.
