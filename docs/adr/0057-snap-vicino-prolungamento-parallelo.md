# ADR 0057: Snap *Vicino*, *Prolungamento* e *Parallelo*

- **Stato:** Proposto
- **Data:** 2026-11-21

## Contesto

L'[ADR 0033](0033-nuovi-snap-e-tracciamento-polare.md) ha portato
intersezione, perpendicolare, tangente e quadrante, con la priorità del punto
5; l'[ADR 0049](0049-tracciamento-degli-snap-a-oggetto.md) il tracciamento,
lasciando fuori *Prolungamento* e *Parallelo*. *Vicino* manca per tutte le
entità (idea [#665](https://github.com/Matleyx78/tecnigrafo/issues/665)). Il
responsabile li ha portati nella 0.8.1 ([DECISIONI.md](../DECISIONI.md),
2026-11-21, «Testo formattato, griglia, snap e attributi della 0.8.1», punto 6,
ed «Esiti dei DXF di prova e della seduta della 0.8.1», punti 1 e 7). La
seduta in AutoCAD della #920 ha dato un caso per *Prolungamento* (10 oltre
l'estremo 30,−50 → 40,−50) e uno per *Parallelo* (20 lungo la parallela a una
linea da 0,−80 a 30,−70, da 0,−60 → 18.9737,−53.6754); `autocad-vicino.dxf` i
punti di *Vicino* su linea, arco, cerchio, polilinea ed ellisse.

## Decisione

### 1. Modi

`core::SnapModes` ha `nearest`, `extension` e `parallel`, e `core::SnapKind`
`Nearest`, `Extension` e `Parallel`. Tutti e tre **spenti** per difetto. Negli
script `!osnap nea,ext,par`; nella domanda `snap` `modes=nea`. **[D5]**
AutoCAD accende *Prolungamento* in `acadiso.dwt` (`OSMODE` 4133); acceso qui
cambierebbe i casi del tracciamento che usano `!hover` su un estremo.

### 2. *Vicino*

- Il punto della curva più vicino al puntatore: per linee, archi, cerchi e
  polilinee la proiezione di `core::closestPoint`; per l'ellisse `nearest()` di
  solid-space, con la sua fascia ([ADR 0052](0052-ellisse.md), punto 4). Anche
  sulla geometria generata di inserimenti, quote e `SOLID` (i suoi lati).
- **Niente *Vicino*** su `TEXT`, `MTEXT`, `ATTDEF`, `ATTRIB` e sui testi della
  geometria generata, come AutoCAD (`Punto non valido`), né sui retini, che non
  danno snap.
- **Priorità**, che estende l'ADR 0033, punto 5: gli snap di punto entro
  l'apertura; se nessuno, *Vicino* sulla curva più vicina al puntatore entro
  l'apertura; solo dopo centro e inserimento
  ([REGOLE-NUMERICHE.md](../REGOLE-NUMERICHE.md), sezione 4, «Selezione e snap
  a oggetto», punto 2).
- **Pareggi:** **[D7]** fra due curve alla stessa distanza vince l'id più alto,
  come il clic (ADR 0008, punto 2); dentro una polilinea il primo segmento, come
  la domanda `closest`. Sono proposte: REGOLE-NUMERICHE.md non le ha.

### 3. Acquisizione di *Prolungamento* e *Parallelo*

Come i punti del tracciamento (ADR 0049, punto 2): durante una richiesta di
punto, fermarsi (500 ms; negli script `!hover x,y`) **acquisisce**, fermarsi di
nuovo **rilascia**; al più 7 acquisizioni in tutto, l'ottava toglie la più
vecchia; si azzerano quando la richiesta riceve un punto o finisce. Valgono
anche con il tracciamento spento, come in AutoCAD, ma solo con lo snap a
oggetto acceso.

- **Prolungamento:** fermarsi entro l'apertura dall'estremo di una `LINE`, di
  un `ARC` o del primo o ultimo segmento di una `LWPOLYLINE` aperta acquisisce
  quel **prolungamento**: per una linea la semiretta oltre l'estremo, nella
  direzione che si allontana dall'altro estremo; per un arco il resto del suo
  cerchio oltre l'estremo (mai sopra l'arco). Ellissi e spline no (tappa 0.8.2).
- **Parallelo:** con un punto base del comando (`Command::basePoint()`),
  fermarsi su una `LINE` o su un segmento diritto di `LWPOLYLINE` (non entro
  l'apertura dai suoi estremi) acquisisce la sua **direzione**.

### 4. Aggancio

- Con il puntatore entro l'apertura da un prolungamento, il punto va sulla sua
  proiezione (per l'arco, sul cerchio lungo il raggio del puntatore, se cade nel
  tratto oltre l'estremo).
- Con il puntatore entro l'apertura dalla **parallela** per il punto base con
  la direzione acquisita, il punto va sulla sua proiezione.
- Prolungamenti e parallele sono **guide** come quelle del tracciamento: due
  guide qualsiasi (del tracciamento, prolungamenti di linee, parallele) che
  prendono il puntatore danno il loro **incrocio**, con l'intersezione delle
  rette di solid-space; un prolungamento d'arco incrocia le guide diritte con
  la stessa intersezione, sul tratto oltre l'estremo.
- **Priorità:** come l'ADR 0049, punto 5: snap a oggetto, incrocio di due
  guide, una guida, polare, orto. **[D6]** Pareggi proposti, da aggiungere a
  REGOLE-NUMERICHE.md: a pari distanza (al quadrato, sezione 3) vince la guida
  o l'incrocio che usa l'acquisizione più recente; fra la guida di un punto e
  un prolungamento dello stesso istante, il prolungamento.
- **Distanza scritta:** con una sola guida diritta che aggancia il puntatore
  (prolungamento di una linea o parallela), un numero scritto dà il punto a
  quella distanza dall'origine della guida (l'estremo o il punto base), verso
  il puntatore, come l'ADR 0049, punto 6. Lungo il prolungamento di un arco la
  distanza scritta vale come oggi (direzione dal punto base al puntatore).

### 5. Nel rapporto e nelle domande

- `active snap:` con `Nearest x,y`.
- `acquired:` elenca anche `extension x,y` (l'estremo) e `parallel x1,y1
  x2,y2` (il segmento), nell'ordine di acquisizione, mescolati ai punti;
  `active tracking:` `Extension x,y from x,y`, `Parallel x,y from x,y`, e gli
  incroci con i nomi delle due guide. Si scrivono solo con i modi accesi, quindi
  i casi di oggi non cambiano.

## Alternative considerate

- ***Prolungamento* e *Parallelo* come snap di punto in `findSnap`:** in AutoCAD
  si acquisiscono come i punti del tracciamento; dentro `findSnap` non ci
  sarebbe la memoria dell'acquisizione.
- **Intersezioni prolungate (`APPINT`, prolungamento contro un oggetto):**
  fuori, con l'intersezione apparente (scheda della 0.8.1).

## Conseguenze

- La riga «Snap a oggetto» del catalogo di parità perde *vicino*,
  *prolungamento* e *parallelo* fra le mancanze.
- Le regole di pareggio [D6] e [D7], decise, vanno in REGOLE-NUMERICHE.md.
