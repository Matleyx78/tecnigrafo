# [0.8-PT42] `core`: righe dei motivi tagliate sulla regione

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT42]` (#…), agente codex, fascia forte: la
  facciata (quali righe di ogni famiglia, origini e passo, tratteggi, ordine,
  stili delle isole, limite delle righe); i punti di taglio sulla regione
  dall'archivio dei casi di solid-space per `clipToRegion`, citando il caso
  ([ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 14)
- **Dipende da:** PT30, PT40 (revisione delle interfacce),
  Matleyx78/solid-space#215, RT42. La richiesta diventa i pacchetti di
  solid-space che realizzano `region.hpp` e `clipping.hpp`, quando la tappa
  `ss-0.3` li avrà (ADR 0030, punto 13)
- **Modulo:** `core`
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punti 1–3;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punti 2 e
  8; l'[ADR 0015 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0015-parti-di-una-curva-dentro-una-regione.md)
  (Decisione); in `third_party/solid-space` le intestazioni qui sotto; la
  scheda [PT30](PT30-retini-core.md) (il modello, i motivi, la regione piena);
  la scheda sorella [PT43](PT43-contorni-dei-retini.md) (stessa funzione del
  ponte); `core/src/solid_space_bridge.hpp`; il [verbale della revisione PT40](https://github.com/Matleyx78/tecnigrafo/issues/735)
  (note non bloccanti per chi realizza)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0015 di solid-space e rivisto in
[`[0.3-CS]` #222](https://github.com/Matleyx78/solid-space/issues/222)
(decisione della #229: le due liste di `ClippedHost` sono indipendenti), al
commit [`fd69e80`](https://github.com/Matleyx78/solid-space/commit/fd69e807d4ebafe12f73f46ba26072a46ff72809)
di `main` di solid-space (2026-11-12, #231):

- `solid_space/geom2d/region.hpp` (`Loop`, `SkippedInput`);
- `solid_space/geom2d/clipping.hpp` (`clipToRegion`, `RegionClipping`,
  `ClippedHost`, `HostStretch`, `BoundaryStretch`, `StretchEnd`);
- con i tipi di `curves.hpp` (`Segment`, `Circle`, `Polyline`) e
  `intersection.hpp` (`Extension`) che la facciata usa già.

I commenti di `clipping.hpp` dicono che cosa è deciso esattamente (tratti,
ordine, lati) e che cosa è arrotondato (gli estremi): la scheda li cita e non
li ripete. Nessun esito sta in una fascia.

## Prima di cominciare

- **Sottomodulo.** Il pacchetto sposta `third_party/solid-space` al commit
  della riga «Sottomodulo di solid-space» della issue: quello che chiude
  l'ultimo dei pacchetti di solid-space che realizzano `clipping.hpp` (oggi la
  richiesta Matleyx78/solid-space#215; il piano lo sostituisce con il
  pacchetto quando la tappa `ss-0.3` lo avrà). Se la riga manca, fermati
  ([contratto.md](../../agenti/contratto.md)).
- Prima della pull request confronta `region.hpp` e `clipping.hpp` al commit di
  destinazione con quelli di `fd69e80` ([contratto.md](../../agenti/contratto.md),
  «Prima della pull request, se sposti il sottomodulo»).

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/hatch.hpp`, `core/src/hatch.cpp` (di PT30)
- `core/src/solid_space_bridge.hpp`, `core/src/solid_space_bridge.cpp` (file
  comune con PT43 e PT50: una funzione)
- `tests/core/hatch_pattern_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)
- `tests/verifica/in-attesa/0.8-RT42.txt` (da cancellare)

## Cosa fare

### 1. Il ponte

In `solid_space_bridge.hpp`, se PT43 non l'ha già aggiunta (la stessa funzione,
con lo stesso commento, è nella sua scheda: chi arriva dopo la trova e la usa):

```cpp
/// The closed polyline of solid-space for a polyline of Tecnigrafo taken as
/// closed, whatever its flag says: the same vertices, with the bulge of
/// vertex i set to 0 when Polyline::segment(i).isArc() is false
/// (docs/adr/0036, point 7), and `closed` set.
[[nodiscard]] g::Polyline closedPolylineOf(const Polyline& polyline);
```

### 2. Quali contorni usa lo stile delle isole

In `hatch.hpp`:

```cpp
/// The loops of `hatch` that its island style keeps (docs/adr/0048, point 3),
/// by increasing index in Hatch::loops. Normal: all. Outer: those flagged
/// external (group 92, bit 1) or outermost (bit 16). Ignore: those flagged
/// external. When no loop is flagged external (a file written by another
/// program), all of them, whatever the style.
[[nodiscard]] std::vector<std::size_t> regionLoops(const Hatch& hatch);
```

Le isole sono già scritte nel file con i loro segnali: Tecnigrafo non le
ricalcola dai contorni arrotondati (una nuova misura delle profondità potrebbe
cambiare con l'arrotondamento dei contorni che si toccano, `boundary.hpp`).
I segnali dei retini creati da Tecnigrafo li scrive PT43.

Ogni contorno usato diventa un `g::Loop`:

- un contorno a lati fatto di un solo lato `Arc` di giro intero (come PT6 e
  PT31 rappresentano un cerchio) diventa il `g::Circle` con lo stesso centro e
  lo stesso raggio;
- ogni altro contorno diventa la polilinea chiusa di PT30 (punto 3: un
  contorno a lati diventa una polilinea con i bulge degli archi; una polilinea
  resta com'è), tradotta con `closedPolylineOf`.

### 3. Le righe del motivo

```cpp
/// HPMAXLINES of AutoCAD at its initial value: a pattern that needs more
/// lines, or more dashes, is not drawn.
inline constexpr std::size_t kMaxHatchLines = 1'000'000;

enum class PatternOutcome : std::uint8_t {
    Done,      ///< `lines` holds the pattern (possibly none)
    TooDense,  ///< more than kMaxHatchLines lines or dashes: `lines` is empty
};

struct PatternLines {
    PatternOutcome outcome{PatternOutcome::Done};
    std::vector<Line> lines;  ///< drawing coordinates, in the order of point 4
    /// False when solid-space left out a loop or a line (beyond its domain,
    /// docs/adr/0035, point 23): what it left out is not drawn.
    bool complete{true};
};

/// The lines of the pattern of `hatch` cut on its region (docs/adr/0048,
/// points 2 and 3). Precondition: supported(hatch) and !hatch.solid.
[[nodiscard]] PatternLines patternLines(const Hatch& hatch);
```

Per ogni famiglia (`PatternLine`, nell'ordine del file), con le funzioni di
`tg/core/math.hpp`:

1. `d = (cos a, sin a)` con `patternDirection(angle)` di PT30 (le famiglie a
   multipli di 90° corrono esattamente lungo gli assi: [DECISIONI.md](../../DECISIONI.md),
   2026-11-12, «Direzioni cardinali esatte»), `n = (−sin a, cos a)`; il passo
   `h = offset · n` (con segno) e lo spostamento lungo la riga
   `s = offset · d`. Con `h` uguale a 0, o non finito, la famiglia non dà
   righe.
2. **Quali righe.** `B` è l'unione degli ingombri dei contorni usati (punto 2;
   nessun contorno: nessuna riga). Per i quattro angoli `c` di `B`,
   `v = ((c − origin) · n) / h`; le righe sono quelle di indice
   `k = ceil(min v) … floor(max v)`, in ordine crescente. La riga `k` passa per
   `p_k = origin + k · offset` (`k` come `double`, prodotti e somme in questo
   ordine) con direzione `d`: è il `g::Segment` da `p_k` a
   `p_k + L · d`, con `L = max(larghezza di B, altezza di B, 1)`, preso con
   `Extension::extended`, cioè tutta la sua retta.
3. **Limite.** Se le righe di tutte le famiglie sono più di `kMaxHatchLines`,
   l'esito è `TooDense` e non si chiama solid-space.
4. **Taglio.** Una sola chiamata `clipToRegion(righe, Extension::extended,
   contorni)`. Per ogni riga, nell'ordine (famiglia, poi `k` crescente):
   prima i tratti `inside`, nel loro ordine, poi i tratti `boundary`, nel loro
   ordine. Anche i tratti di bordo, una riga che corre lungo un lato, fanno
   parte del retino (decisione del 2026-11-12, [DECISIONI.md](../../DECISIONI.md)).
   Le due liste sono indipendenti (#229): non si confrontano i loro estremi e
   non si uniscono tratti dell'una e dell'altra.
5. **Tratteggi.** I tratti di una riga si misurano dall'origine della sua
   famiglia (ADR 0048, punto 2): per la riga `k` la misura parte da `p_k`, che
   già contiene lo spostamento `k · s`, come nella definizione dei motivi di
   AutoCAD. Per un tratto con estremi `P0` e `P1` (i punti di solid-space),
   `u0 = (P0 − p_k) · d` e `u1 = (P1 − p_k) · d`:
   - senza trattini (`dashes` vuoto), o con la somma dei valori assoluti `T`
     uguale a 0, o con `u1 ≤ u0` (un tratto più corto dei suoi errori,
     `clipping.hpp`): una `Line` da `P0` a `P1`;
   - altrimenti il motivo si ripete con periodo `T` da `u = 0`: un valore
     positivo è un trattino, uno negativo uno spazio, uno zero un punto. Ogni
     trattino che si sovrappone a `[u0, u1]` per una lunghezza positiva dà la
     `Line` della parte comune, da `p_k + max(u0, inizio) · d` a
     `p_k + min(u1, fine) · d`, ma con `P0` o `P1` così come sono quando la
     parte comincia in `u0` o finisce in `u1`; ogni punto in `[u0, u1]`
     (estremi compresi) dà una `Line` di lunghezza zero. In ordine di `u`.
   - Se le `Line` diventano più di `kMaxHatchLines`, l'esito è `TooDense` e
     `lines` resta vuoto.
6. Righe e contorni scartati da solid-space (`skippedHosts`, `skippedLoops`):
   `complete` è falso e il resto si disegna.

### 4. La geometria generata

`generateOf(…, const Hatch&)` di PT30 cambia così:

- `solid`: la `FilledRegion` con gli anelli dei contorni di `regionLoops`, per
  tutti e tre gli stili (PT30 lo faceva solo per *normale*);
- motivo: le `Line` di `patternLines` (nessuna con `TooDense`), con le
  proprietà effettive del retino e il tipo di linea `Continuous`: i trattini
  vengono dal motivo, non dal tipo di linea, come AutoCAD con `HPLINETYPE` 0.

Selezione, trasformazioni e inserimenti non cambiano: le righe sono geometria
generata come le altre (PT6).

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` (ADR 0044, punto 8).
- File comuni: `core/src/solid_space_bridge.*` (PT42, PT43 e PT50: ognuno la
  sua funzione, in ordine alfabetico; `closedPolylineOf` la scrive il primo
  dei due fra PT42 e PT43), `tests/CMakeLists.txt`.

## Test richiesti

- `hatch_pattern_test.cpp`:
  - `ANSI31` metrico su un quadrato 0,0–10,10: righe a 45° con passo 3,175,
    ordine per `k`; lo stesso con un cerchio dentro (stile *normale*): ogni
    riga che lo attraversa dà due tratti;
  - `NET` con origine nello spigolo: le righe che corrono lungo i lati ci
    sono (tratti di bordo);
  - `NET` metrico, scala 1, origine `(0, 0)`, sul rettangolo
    `[−10, 0] × [0, 10]`: la riga verticale di indice 0 dà un tratto di bordo
    lungo 10 sul lato destro; sul rettangolo `[0, 10] × [0, 10]` lo stesso
    tratto sul lato sinistro. Lo stesso con l'angolo della famiglia a 90° più
    un giro intero (`ROTATE` di 360°) e dopo `MIRROR` sull'asse x (angolo −90°
    o 270°): la direzione è `(0, ±1)` esatta e il tratto di bordo c'è;
  - un motivo con trattini, spazi e un punto (`49`: 5, −2.5, 0, −2.5) su una
    riga obliqua: la fase parte da `p_k`, e la riga `k + 1` è spostata di `s`;
  - stile *solo esterno* con un'isola nell'isola segnata come in AutoCAD
    (segnali 1, 16 e 0): la terza non conta; *ignora*: solo il contorno
    esterno; nessun contorno segnato esterno: tutti;
  - un `SOLID` con stile *ignora*: la regione ha un solo anello;
  - passo minuscolo su un'area grande: `TooDense`, nessuna riga;
  - `h` = 0: la famiglia non dà righe.

## Fatto quando

- I casi di RT42 passano, compresi quelli di `autocad-retini.dxf` per i retini
  a motivo letti dal file.
- La CI è verde.
