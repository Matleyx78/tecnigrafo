# [0.7-P15] `core`: snap a intersezione; parti delle entità

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R15]` (#…), agente gemini, per lo snap (anche la
  revisione dei casi esistenti, ADR 0033 punto 6). Le parti delle entità non si
  raggiungono ancora da uno script: le provano i test di unità qui e i casi di
  `TRIM`, `EXTEND` e `FILLET` più avanti (R16, R20)
- **Dipende da:** P7, P14, P25, R15
- **Corretta:** 2026-11-01, con le decisioni del responsabile sulla #359 e sui
  buchi B1–B3 della #464 ([DECISIONI.md](../../DECISIONI.md)); 2026-11-03,
  operazione snellimento (#478): rimanda a `intersection.hpp` invece di
  ripeterlo; revisione della #487: correzione angolare minima (C, che sostituisce A:a e C:A) e
  identificativo delle intersezioni (B:a), decise dal responsabile; terzo
  verbale: fascia del cerchio al taglio 0/2π (scelta a) e riporto di uno o
  tutti e due gli angoli
- **Modulo:** `core`
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 3, 5 e 6; [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 5, 6 e 23; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 9 e
  10; `core/include/tg/core/intersections.hpp` (P14),
  `core/src/solid_space_bridge.hpp` (P14); `core/src/document_query.cpp`
  (`findSnap`, P7); `core/include/tg/core/arc_geometry.hpp`, `polyline.hpp`

## Intestazioni di solid-space che usa

Nessuna nuova: la facciata di P14 per lo snap, e `solid_space/robust/
trigonometry.hpp` (`atan2`) per gli angoli nuovi degli archi. Il sottomodulo
resta dove l'ha messo P14; se la issue indica un altro commit (perché `atan2`
arriva con un pacchetto di solid-space diverso), lo sposti a quello.

## File

- `core/src/document_query.cpp`, `core/include/tg/core/document_query.hpp` se
  serve
- `core/include/tg/core/curve_parts.hpp`, `core/src/curve_parts.cpp` (nuovi)
- `core/CMakeLists.txt`
- `tests/core/document_query_test.cpp`, `tests/core/curve_parts_test.cpp`
  (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R15.txt` (da cancellare)

## Cosa fare

### Snap a intersezione

In `findSnap`, con `modes.intersection`:

- si prendono le entità visibili la cui curva passa entro l'apertura (indice
  spaziale, poi `distance`);
- si calcola `intersect` per ogni coppia, e `selfIntersections` per ogni
  polilinea;
- i punti di `crossings` entro l'apertura sono candidati `Intersection`, con le
  priorità dell'ADR 0033, punto 5; i tratti in comune non danno punti (i loro
  estremi sono già estremi delle entità);
- un punto in cui si incontrano più coppie è uno solo: tra i candidati alla
  stessa distanza vale l'ordine di `findSnap`: il punto di una coppia porta
  il maggiore dei due identificativi, un'autointersezione quello della
  polilinea; e la facciata non deve dare due
  volte lo stesso punto per la stessa coppia.

Prestazioni: al più qualche decina di entità vicine al puntatore. Il test di
prestazioni esistente (`tests/bench`, se ha un caso dello snap) non deve
peggiorare in modo visibile; se lo fa, scrivilo nella pull request con le
misure.

### Parti delle entità (`curve_parts.hpp`)

La costruzione che serve a `TRIM`, `EXTEND`, `FILLET` e `CHAMFER` (ADR 0031,
punto 10): dove tagliare lo dice solid-space, il pezzo lo costruisce
Tecnigrafo.

```cpp
/// A place on the curve of an entity, as intersect(), cutsAlong() and the
/// constructions of solid-space give it: the point and its position.
struct CurvePoint {
    Vec2 point;
    CurvePosition position;
};

/// How the part runs from `from` to `to`. The caller knows it exactly, from
/// the order of HostCuts::cuts or from a construction; partOf never deduces it
/// from points or stations, which are rounded (#359).
enum class PartCourse : std::uint8_t {
    Direct,       ///< `from` comes before `to`, without passing the start
    AcrossStart,  ///< a circle or a closed polyline, passing its start
                  ///< (angle 0, vertex 0)
    SamePlace,    ///< `from` and `to` are the same place: nothing in between
};

enum class PartOutcome : std::uint8_t {
    Done,              ///< `entity` holds the part
    Empty,             ///< nothing is left: the part does not stay
    NotRepresentable,  ///< the exact part exists, but the model cannot hold it
};

struct PartResult {
    PartOutcome outcome{PartOutcome::Empty};
    std::optional<Entity> entity;  ///< with Done
};

/// The part of `entity` from `from` to `to`, each std::nullopt for its start
/// or its end (docs/adr/0035, point 5):
/// - Line: from `from` to `to`;
/// - Arc: counter-clockwise from `from` to `to`;
/// - Circle: the arc counter-clockwise from `from` to `to` (both required);
/// - Polyline: the vertices between the two places, with the cut segments
///   ending at the given points; a closed polyline becomes open, running from
///   `from` to `to`, across its vertex 0 with AcrossStart (both required).
/// Every supplied boundary point replaces the endpoint or vertex exactly,
/// including AtStart/AtEnd at an existing vertex. Supplied arc endpoints are
/// also rebuilt, even at AtStart/AtEnd; omitted endpoints keep their angles.
/// New arc angles come from solid_space::robust::atan2
/// of (point - centre), brought into [0, 2π) with normalizeAngle: the ends of
/// an arc lie within the error, not exactly on the given points. A cut
/// polyline arc gets the bulge of its new included angle; the widths of a cut
/// polyline segment are interpolated by length (docs/adr/0031, point 9), with
/// stations measured from its start in either direction. An interior end cut
/// copies the following original vertex's bulge and widths to the final,
/// non-drawing vertex, replacing only its position.
/// Every other property of the entity stays.
/// Empty with SamePlace, and when the part is a single arc (an Arc, a part of
/// a Circle, or a part of a polyline within one curved segment) whose two
/// angles, or two vertices, come out equal while the exact part is the short
/// one (Direct, on one segment); NotRepresentable when they come out equal
/// and the exact part is the long one (AcrossStart): it never becomes a
/// circle. If computed angles contradict PartCourse or containment in the
/// original arc, use the minimum angular correction restoring both facts:
/// collapse the two angles, or move one or both angles to original endpoints
/// (for a circle, to the side of the 0/2*pi cut required by PartCourse). Minimize
/// the sum of circular angular displacements with exact computed comparisons,
/// no threshold; ties prefer the original start, then the part's start.
/// This applies to either direction of curved polyline segments. A collapsed
/// cut arc within several segments has bulge 0 and keeps the supplied points.
/// Circle cut band (solid-space ADR 0013, point 4): when both exact places
/// are within the declared error of the 0/2*pi cut, on opposite sides, the
/// minimum correction has a free but deterministic outcome, which may be
/// the opposite piece to the exact one, for Direct and AcrossStart alike.
/// Precondition: not a Text; no place is BeforeStart or AfterEnd (extendTo
/// takes those); with Direct, `from` does not come after `to` in the order of
/// cutsAlong; AcrossStart only for a circle or a closed polyline, with `to`
/// not after `from`. A polyline vertex is AtStart of the following segment
/// (vertex 0 is segment 0, also on a closed polyline); only the final vertex
/// of an open polyline is AtEnd of its last segment.
[[nodiscard]] PartResult partOf(const Entity& entity, const std::optional<CurvePoint>& from,
                                const std::optional<CurvePoint>& to, PartCourse course);

enum class EntityEnd : std::uint8_t { Start, End };

/// `entity` with its end `end` brought to `to`, beyond that end (EXTEND, and
/// FILLET and CHAMFER when the point is BeforeStart or AfterEnd; docs/adr/0035,
/// point 6):
/// - Line: along its line, `to` BeforeStart for the start, AfterEnd for the end;
/// - Arc: along its circle, `to` AfterEnd for both ends: for the start the
///   circle is run backwards from the start;
/// - open Polyline: its first or last segment that is not exactly a point
///   (as Extension::Extended, P14), the same way as a line or an arc; the
///   vertices of the segments exactly a point between that segment and the
///   end take the new point and keep all their other data (docs/adr/0006).
/// The new point of a line and of a polyline is `to`, exactly; a new arc
/// angle comes as in partOf, with the same minimum angular correction when
/// it contradicts extension: the allowed interval is the complement of the
/// original arc. Choose between the minimum extension (the changed original
/// endpoint) and closing the full turn (the other endpoint); ties prefer the
/// original arc's start, in either direction. The latter is NotRepresentable.
/// Precondition: a Line, an Arc or an open Polyline; `to` is on the segment
/// above, BeforeStart or AfterEnd as said.
[[nodiscard]] PartResult extendTo(const Entity& entity, EntityEnd end, const CurvePoint& to);

```

- **Lo stesso posto e il giro** li decide il chiamante, che li ha esatti
  (decisione A della #359): per `TRIM` ed `EXTEND` dalla lista ordinata di
  `HostCuts::cuts`, dove lo stesso posto è lo stesso taglio della lista, mai
  due tagli con gli stessi dati arrotondati; per `FILLET` e `CHAMFER` dalla
  costruzione, che dà un solo posto per curva. Nessuna richiesta a
  solid-space: identità e ordine esatti li dà già il contratto
  (`intersection.hpp`, in testa, e `HostIntersections`).
- **Archi e correzione angolare minima** (B1 e C della #487): se gli
  angoli calcolati contraddicono `PartCourse` o il contenimento del pezzo
  esatto nell'arco originale, si sceglie la correzione angolare minima che
  rende coerenti entrambi. Le alternative sono il collasso dei due angoli
  e il riporto di uno o tutti e due gli angoli agli estremi originali;
  per il cerchio, al taglio 0/2π dal lato richiesto da `PartCourse`, sia `Direct` sia
  `AcrossStart`. Si minimizza la somma degli spostamenti angolari circolari
  con confronti esatti sui valori calcolati, senza soglie. A parità prevale
  l'inizio originale, poi quello del pezzo. Vale per `Arc` e segmenti curvi
  di polilinea nei due versi. Il collasso segue B1: `Empty` per il pezzo
  corto, `NotRepresentable` per il lungo; in una parte di più segmenti il
  tratto collassato prende bulge 0 e conserva i punti ricevuti. Per
  `extendTo` l'intervallo coerente è il complemento dell'arco originale:
  fra prolungamento minimo e chiusura del giro vince la correzione minore,
  a parità l'inizio originale; chiudere il giro dà `NotRepresentable`.
  Questa regola sostituisce le regole caso per caso di A:a e C:A, anche per
  nuovi casi della stessa classe. La regola è di Tecnigrafo; quella della
  curva parallela di solid-space (`offset.hpp`) qui non si applica.
- **Fascia del cerchio al taglio 0/2π** (terzo verbale della #487, punto 1,
  [scelta a del responsabile](https://github.com/Matleyx78/tecnigrafo/pull/487#issuecomment-6613739609)):
  quando i due posti esatti stanno entro l'errore dichiarato dal taglio 0/2π,
  da lati opposti, l'esito della correzione angolare minima è libero e
  deterministico, e può essere il pezzo opposto a quello esatto, sia con
  `Direct` sia con `AcrossStart`. È una fascia ai sensi dell'[ADR 0013 di
  solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
  punto 4; non cambia l'algoritmo né aggiunge una soglia.
- **Nei comandi** `Empty` vuol dire che quel pezzo non resta;
  `NotRepresentable` che l'operazione non riesce e l'oggetto resta com'è, con
  il messaggio dell'operazione non riuscita (ADR 0035, punto 23).
- **Prolungare o tagliare** (B2): il chiamante sceglie fra `partOf` ed
  `extendTo` guardando la `Location` (`intersection.hpp`), che è esatta e
  non cambia significato.
- **Polilinee aperte** (B3): il «primo» e l'«ultimo» segmento sono il primo e
  l'ultimo non puntiforme, come in P14 (`Extension::Extended`). Per esempio
  [(0,0),(0,0),(10,0)] prolungata all'inizio fino a x=−5 diventa
  [(−5,0),(−5,0),(10,0)].

## Regole della tappa che valgono qui

- `document_query.cpp`: prima di te P7, dopo di te P19 (snap a tangente).
- `core/CMakeLists.txt`: solo le tue righe.
- Nessuna decisione geometrica propria: dove tagliare e in che ordine viene da
  `intersections.hpp`.

## Test richiesti

- Snap: due linee che si incrociano, linea e arco, due cerchi, polilinea che si
  interseca; tre linee nello stesso punto; incrocio fuori dall'apertura;
  tangenza; layer spento (nessuna intersezione con le sue entità); priorità tra
  intersezione ed estremo alla stessa distanza (vince l'estremo).
- `partOf`: linea accorciata ai due lati; arco tagliato; cerchio che diventa
  arco, con `Direct` e con `AcrossStart`; polilinea aperta tagliata in un
  segmento dritto e in uno curvo (bulge e larghezze), polilinea chiusa aperta
  attraverso il vertice 0; `SamePlace`; i due tagli distinti della #359 sulla
  linea da (0,0) a (2,0), con lo stesso punto arrotondato (una linea di
  lunghezza zero, `Done`); due posti di un arco con gli stessi angoli
  arrotondati, che danno `Empty` con `Direct` e, su un cerchio,
  `NotRepresentable` con `AcrossStart`; da (2,0) a (8,0) con `Direct` e da
  (8,0) a (2,0) con `AcrossStart` sul rettangolo chiuso della #359.
- `extendTo`: linea prolungata ai due estremi; arco prolungato alla fine oltre
  `2π` e all'inizio, all'indietro (l'arco 0°→90° fino al taglio a 270° dà
  270°→90°); arco che si chiuderebbe (`NotRepresentable`); polilinea aperta
  prolungata al primo e all'ultimo segmento, anche curvi, e con un segmento
  puntiforme all'estremo (l'esempio di B3).

- Revisione #487, A:a: `Arc{{0,0},5,0,π/2}`, da (3,4) a
  (3.000000000000001,4), entrambi `Inside`, `Direct`: `Empty`;
  `Polyline` da (−5,0) a (5,0), bulge 1, da (0,−5) a
  (−1e−15,−5), entrambi `Inside` del segmento 0, `Direct`: `Empty`.
  Ripetere con bulge negativo, arco attraverso 0, cerchio `Direct` corto
  (`Empty`) e `AcrossStart` lungo (`NotRepresentable`), e prolungamenti
  invertiti ai due estremi (`NotRepresentable`), anche di segmenti curvi nei
  due versi. In una parte con più segmenti l'arco invertito conserva i punti
  e prende bulge 0.
- Revisione #487, C (secondo verbale, esempi 1–4):
  1. `Arc{{0,0},5,0,π/2}`, `from` omesso, `to=(-1e-15,5)` Inside,
     `Direct`: `Done`, l'arco originale; anche il caso simmetrico all'inizio
     e un arco che passa per zero.
  2. `Circle{{0,0},5}`, da `(5,-1e-14)` a `(0,5)`, `Direct`:
     `Done`, arco 0→π/2. Scambiando i tagli e usando `AcrossStart`:
     arco π/2→0. Taglio vicino a zero nell'altro verso: da `(0,-5)` a
     `(5,1e-14)` Direct dà 3π/2→0; scambiati, AcrossStart dà 0→3π/2.
  3. `Polyline{{{-5,0},1},{{5,0}},{{10,0}}}`, da `(-5,1e-14)`
     Inside a fine omessa: `Done`, conserva il semicerchio (bulge 1)
     seguito dalla linea e il punto ricevuto. Anche fine tagliata e bulge −1.
  4. L'arco del punto 1 prolungato alla fine a `(1e-15,5)` AfterEnd:
     `Done`, prolungamento minimo; anche all'inizio e sui segmenti curvi
     nei due versi, conservando punti e larghezze.
- Terzo verbale #487, fascia del cerchio (punto 1, scelta a): su
  `Circle{{0,0},5}`, `Direct` da `(5,-1e-14)` a `(5,1e-14)` dà `Done`,
  arco 0→circa 2e-15 radianti; `AcrossStart` da `(5,1e-14)` a `(5,-1e-14)`
  dà `Done`, arco circa 2e-15→0. Il test fissa l'esito attuale dentro la
  fascia del taglio 0/2π, non quello esatto: il primo pezzo esatto è quasi
  tutto il cerchio, il secondo è minuscolo attraverso lo zero.
- Riporto di tutti e due gli angoli: `Arc{{0,0},5,0,π/2}`, da `(5,-1e-14)`
  a `(-1e-15,5)`, entrambi `Inside`, `Direct`: `Done`, l'arco intero,
  come nel terzo controllo di `MinimumCorrectionKeepsAlmostTheWholeOriginalArc`.
- Parità C: `u=0x1p-50`, `T=0x1.921fb54442d18p+2`,
  `Arc{{0,0},5,0,T-2*u}`, `to=(5,-5*u)` Inside, `from` omesso,
  `Direct`: prevale l'inizio, `Empty`; usando lo stesso punto come `from`
  e omettendo `to`, `Done` con l'arco originale.
- Revisione #487, B:a: linea 1 da (−10,0) a (10,0), linea 2 a x=−1,
  linea 3 a x=1, puntatore (0,0), apertura 1, solo intersezione: vince
  (1,0), identificativo della coppia {1,3} pari a 3.
- Scelte non bloccanti #487: punti ricevuti diversi di qualche ulp dai
  vertici `AtStart`/`AtEnd` sostituiscono le coordinate; anche gli estremi
  espliciti di un arco ricalcolano l'angolo. Il vertice finale di un taglio
  interno prende bulge e larghezze da `vertexAt(index + 1)`. Vertice 0 di
  una chiusa rappresentato come `AtStart` del segmento 0, in entrambi i
  percorsi. Larghezze di un arco orario tagliato fuori dal medio:
  stazione dall'inizio del segmento, non dalla fine.

## Fatto quando

- I casi di `[0.7-R15]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`; i casi esistenti (rivisti da R15) passano.
- I test di `partOf` esistono e passano.
- La CI è verde.
