# [0.8-PT43] `core`: contorno rilevato da un punto e isole dei contorni scelti

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT43]` (#…), agente codex, fascia forte: la
  facciata (curve candidate, traduzione dei contorni in contorni del retino,
  segnali, esiti, entità nominata, contorni scelti scartati); i contorni, le
  isole, le profondità e gli estremi liberi dall'archivio dei casi di
  solid-space per `detectBoundary` e `nestContours`, citando il caso
  ([ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 14)
- **Dipende da:** PT30, PT40 (revisione delle interfacce),
  Matleyx78/solid-space#214, RT43. La richiesta diventa i pacchetti di
  solid-space che realizzano `region.hpp` e `boundary.hpp`, quando la tappa
  `ss-0.3` li avrà (ADR 0030, punto 13)
- **Modulo:** `core` (più due domande in `script`)
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punti 1, 5 e 6;
  [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md), punti 1, 2 e 7;
  l'[ADR 0014 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0014-contorno-e-isole-di-una-regione.md)
  (Decisione, Conseguenze); in `third_party/solid-space` le intestazioni qui
  sotto; la scheda [PT30](PT30-retini-core.md) (il modello); la scheda sorella
  [PT42](PT42-righe-dei-motivi.md) (la stessa funzione del ponte; come usa i
  segnali); `core/src/solid_space_bridge.hpp`, `core/include/tg/core/tolerance.hpp`,
  `core/src/document_query.cpp` (quali layer si interrogano);
  [verifica/domande.md](../../verifica/domande.md) e la riga `HATCH` di
  [verifica/rapporto.md](../../verifica/rapporto.md) (PT9)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0014 di solid-space e rivisto in
[`[0.3-CS]` #222](https://github.com/Matleyx78/solid-space/issues/222)
(decisioni #223–#228 e #232), al commit
[`fd69e80`](https://github.com/Matleyx78/solid-space/commit/fd69e807d4ebafe12f73f46ba26072a46ff72809)
di `main` di solid-space (2026-11-12, #231):

- `solid_space/geom2d/region.hpp` (`Loop`, `RegionLoop`, `LoopPieceOrigin`,
  `SkippedInput`);
- `solid_space/geom2d/boundary.hpp` (`detectBoundary`, `DetectedBoundary`,
  `BoundaryOutcome`, `OpenEnd`; `nestContours`, `ContourNesting`);
- con i tipi di `curves.hpp` e `solid_space/robust/tolerance.hpp`
  (`TolerancePolicy`) che la facciata usa già.

I commenti di `boundary.hpp` dicono i passi del risultato esatto, l'ordine dei
contorni e dei loro pezzi, gli errori e la fascia degli archi (ADR 0013 di
solid-space, punto 4), e i limiti rispetto ad AutoCAD: la scheda li cita e non
li ripete.

## Prima di cominciare

- **Sottomodulo.** Il pacchetto sposta `third_party/solid-space` al commit
  della riga «Sottomodulo di solid-space» della issue: quello che chiude
  l'ultimo dei pacchetti di solid-space che realizzano `boundary.hpp` (oggi la
  richiesta Matleyx78/solid-space#214; il piano lo sostituisce con il
  pacchetto quando la tappa `ss-0.3` lo avrà). Se la riga manca, fermati
  ([contratto.md](../../agenti/contratto.md)).
- Prima della pull request confronta `region.hpp` e `boundary.hpp` al commit di
  destinazione con quelli di `fd69e80` ([contratto.md](../../agenti/contratto.md),
  «Prima della pull request, se sposti il sottomodulo»).

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/hatch_boundary.hpp`, `core/src/hatch_boundary.cpp`
  (nuovi)
- `core/src/solid_space_bridge.hpp`, `core/src/solid_space_bridge.cpp` (file
  comune con PT42 e PT50: una funzione)
- `core/CMakeLists.txt` (file comune)
- `script/src/hatch_questions.cpp` (nuovo), `script/src/question_table.cpp`,
  `script/CMakeLists.txt` (file comuni)
- `docs/verifica/domande.md`
- `tests/core/hatch_boundary_test.cpp` (nuovo), `tests/script/questions_test.cpp`,
  `tests/CMakeLists.txt` (file comune)
- `tests/verifica/in-attesa/0.8-RT43.txt` (da cancellare)

## Cosa fare

### 1. Il ponte

In `solid_space_bridge.hpp`, se PT42 non l'ha già aggiunta (la stessa funzione,
con lo stesso commento, è nella sua scheda: chi arriva dopo la trova e la usa):

```cpp
/// The closed polyline of solid-space for a polyline of Tecnigrafo taken as
/// closed, whatever its flag says: the same vertices, with the bulge of
/// vertex i set to 0 when Polyline::segment(i).isArc() is false
/// (docs/adr/0036, point 7), and `closed` set.
[[nodiscard]] g::Polyline closedPolylineOf(const Polyline& polyline);
```

### 2. Il contorno da un punto

`hatch_boundary.hpp`:

```cpp
/// How the detection around a picked point ended (solid-space's
/// BoundaryOutcome, boundary.hpp).
enum class BoundaryOutcome : std::uint8_t {
    Enclosed,     ///< `loops` holds the contour and its islands
    OnCurve,      ///< the point lies on a curve: `nearEntity` says which
    NotEnclosed,  ///< no closed contour around the point: `openEnds` may help
};

struct DetectedBoundary {
    BoundaryOutcome outcome{BoundaryOutcome::NotEnclosed};
    /// With Enclosed: the contour first, then the islands, in solid-space's
    /// order (boundary.hpp, DetectedBoundary::loops), as loops of a hatch with
    /// their flags (point 4). Empty otherwise.
    std::vector<HatchLoop> loops;
    /// With OnCurve: the entity that owns the curve solid-space names.
    std::optional<EntityId> nearEntity;
    /// With NotEnclosed: the free ends solid-space gives, in its order.
    std::vector<Vec2> openEnds;
    /// False when solid-space left out a candidate curve, or the picked point
    /// (beyond its domain, docs/adr/0035, point 23).
    bool complete{true};
};

/// The boundary of a hatch from a point picked inside it (docs/adr/0048,
/// points 5 and 6): solid-space's detectBoundary over the candidate curves
/// of `document`, with the tolerance rule of the drawing.
/// Precondition: finite coordinates.
[[nodiscard]] DetectedBoundary detectHatchBoundary(const Document& document,
                                                   const Vec2& picked);
```

- **Curve candidate** (ADR 0048, punto 5; decisione del 2026-11-12,
  [DECISIONI.md](../../DECISIONI.md)): le entità `Line`, `Arc`, `Circle` e
  `Polyline` del disegno, nell'ordine di `Document::entities()`, sui layer
  accesi e non congelati, come la selezione (`document_query.hpp`; i layer
  bloccati contano). Linee, archi e cerchi con `bridge::curveOf`; una polilinea
  con i suoi pezzi, `bridge::piecesOf`, uno per segmento. Non sono candidate le
  ellissi (il contratto prende solo le `Curve`; i contorni con ellissi non sono
  gestiti, ADR 0048, punto 1), né le forme generate di inserimenti, quote e
  testi: i contorni fatti da blocchi e quote non si rilevano, una differenza da
  AutoCAD che il manuale riporta ([PT46](PT46-manuale.md)).
- La regola: `bridge::policyOf(ToleranceRule::of(document))`.
- **Indici.** Ogni curva ricorda la sua entità: `nearCurve` di `onCurve`, e la
  curva più bassa di un collegamento, diventano l'entità di quella curva.
- Un punto oltre il dominio (diagnosi `outOfDomain`): `NotEnclosed`, nessun
  estremo libero, `complete` falso.

### 3. I contorni scelti

```cpp
struct ChosenContours {
    /// One loop per entity taken, in the order of `chosen`, with its flags
    /// (point 4).
    std::vector<HatchLoop> loops;
    /// The entities that gave `loops`, in the same order.
    std::vector<EntityId> taken;
    /// The entities of `chosen` that give no loop, in their order: not a
    /// circle nor a closed polyline, not in the document, or left out by
    /// solid-space (nestContours, skipped).
    std::vector<EntityId> leftOut;
};

/// The contours chosen with Select objects (docs/adr/0048, point 6) and which
/// lies inside which (solid-space's nestContours).
[[nodiscard]] ChosenContours chosenHatchContours(const Document& document,
                                                 std::span<const EntityId> chosen);
```

- Un `Circle` diventa il `g::Circle`; una `Polyline` chiusa (`closed()`) la
  `closedPolylineOf`; ogni altra entità va in `leftOut`, anche una polilinea
  aperta e un'ellisse (ADR 0048, punto 6).
- Le profondità sono quelle di `nestContours`: un conflitto non aggiunge nulla
  alla profondità di nessuno dei due (`boundary.hpp`), quindi due contorni che
  si attraversano restano tutti e due nel retino, con la regola pari-dispari.

### 4. Dai contorni di solid-space ai contorni del retino

Con gli stessi `double`, senza ricalcoli:

- un `g::Circle` diventa un contorno a lati con un solo lato `Arc` di giro
  intero (centro, raggio, angoli 0 e `kTwoPi`), come PT6 e PT31 rappresentano
  un cerchio;
- una `g::Polyline` diventa un contorno a polilinea: una `Polyline` chiusa con
  gli stessi vertici e bulge, larghezze 0;
- **segnali** (gruppo 92, nei nomi della riga `HATCH` di rapporto.md):
  `polyline` per un contorno a polilinea, `edges` per un contorno a lati;
  `external` per la profondità 0; `outermost` per la profondità 1; `derived`
  per i contorni rilevati da un punto (punto 2), non per quelli scelti
  (punto 3). Sono i segnali con cui PT42 sceglie i contorni per lo stile
  delle isole.

### 5. Le domande

In domande.md, una sezione «Retini» dopo quelle della tappa 0.7, e in
`hatch_questions.cpp`; `<loop>` è un `loop=…` con i suoi elementi, nella forma
della riga `HATCH` di [rapporto.md](../../verifica/rapporto.md) (PT9).

| Domanda | Risposta | Significato |
|---|---|---|
| `hatch-boundary x,y [units=<n>]` | `enclosed <loop> <loop>…`; `on-curve <nome>`; `not-enclosed`, seguito da ` open=x,y` per ogni estremo libero; ognuna con ` incomplete` in fondo se qualcosa è stato scartato | il contorno e le isole rilevati dal punto fra le entità definite sopra (`detectHatchBoundary`), nell'ordine di solid-space; `<nome>` è l'entità su cui sta il punto; `units=` è `INSUNITS` del disegno, 4 se manca |
| `hatch-contours <E1> <E2> …` | `<loop> <loop>…`, poi ` left-out=<nome>` per ogni entità scartata; `none` se non c'è nulla | i contorni scelti (`chosenHatchContours`) nell'ordine dei nomi, con i segnali delle loro profondità |

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` (ADR 0044, punto 8).
- File comuni: `core/src/solid_space_bridge.*` (PT42, PT43 e PT50: ognuno la
  sua funzione, in ordine alfabetico; `closedPolylineOf` la scrive il primo dei
  due fra PT42 e PT43), `core/CMakeLists.txt`, `script/src/question_table.cpp`,
  `script/CMakeLists.txt`, `tests/CMakeLists.txt`.
- `docs/verifica/domande.md` lo toccano PT9, PT34, PT43 e PT50: solo le tue
  righe; chi arriva dopo si aggiorna.

## Test richiesti

- `hatch_boundary_test.cpp`:
  - i casi della richiesta (ADR 0014 di solid-space, «I casi della
    richiesta», 1, 1 bis, 2, 4 e 5) tradotti in entità: un rettangolo di
    quattro `Line` con un `Circle` dentro, punto fra i due (`enclosed`, il
    contorno `external+derived+polyline`, l'isola
    `outermost+derived+edges` con il cerchio); il punto dentro il cerchio;
    un'isola nell'isola (terzo contorno senza `external` né `outermost`); uno
    spazio più largo della tolleranza (`not-enclosed` con gli estremi); il
    punto sopra una linea (`on-curve`, quella linea);
  - una polilinea chiusa come contorno, una polilinea con un segmento che
    `isArc()` dà diritto (bulge 0 al ponte);
  - un layer spento e uno congelato: le loro curve non contano; uno bloccato
    sì;
  - un'ellisse e un inserimento non sono candidati;
  - `chosenHatchContours`: un rettangolo chiuso, due cerchi dentro, una
    polilinea chiusa in uno dei due (profondità 0, 1, 1, 2 e i loro segnali);
    una polilinea aperta e una linea in `leftOut`; due contorni che si
    attraversano.
- `questions_test.cpp`: `hatch-boundary` e `hatch-contours` su due casi,
  nella forma della tabella.

## Fatto quando

- I casi di RT43 passano.
- La CI è verde.
