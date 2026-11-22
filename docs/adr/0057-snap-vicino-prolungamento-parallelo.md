# ADR 0057: Snap *Vicino*, *Prolungamento* e *Parallelo*

- **Stato:** Accettato
- **Data:** 2026-11-21
- **Corretto:** 2026-11-22 — punti 2, 3 e 4, dopo la revisione delle
  interfacce PT5 (#928) e della sua correzione (#992): pareggi di *Vicino*
  fra forme dello stesso id e punto d'inserimento dell'`INSERT` fra gli snap
  di punto; acquisizione con più candidati e allo spegnimento di un modo;
  pareggi rimasti fra due incroci; distanza scritta fuori dal tratto di un
  prolungamento (decisioni del
  2026-11-22 in `docs/DECISIONI.md`, «Revisione delle interfacce PT5 della
  0.8.1»).

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
script `!osnap nea,ext,par`; nella domanda `snap` `modes=nea`. AutoCAD accende
*Prolungamento* in `acadiso.dwt` (`OSMODE` 4133); qui resta spento, perché
acceso cambierebbe i casi del tracciamento che usano `!hover` su un estremo
([DECISIONI.md](../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano della
0.8.1», punto 5).

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
  l'apertura; solo dopo centro e inserimento (il punto d'inserimento di un
  `INSERT` no: è fra gli snap di punto, come dalla 0.8, e vince su *Vicino*)
  ([REGOLE-NUMERICHE.md](../REGOLE-NUMERICHE.md), sezione 4, «Selezione e snap
  a oggetto», punto 2).
- **Pareggi:** fra due curve alla stessa distanza vince l'id più alto, come il
  clic (ADR 0008, punto 2); dentro una polilinea il primo segmento, come la
  domanda `closest`; fra forme con lo stesso id (la geometria generata di un
  inserimento, i lati di un `SOLID`) la prima nell'ordine di generazione (REGOLE-NUMERICHE.md, sezione 4, «Selezione e snap a
  oggetto», punto 2; [DECISIONI.md](../DECISIONI.md), 2026-11-22, «Decisioni
  D1–D10 del piano della 0.8.1», punto 7).

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
- **Più candidati:** la sosta acquisisce lo snap che vince, con le sue
  priorità e i suoi pareggi (a pari distanza l'id più alto), e il
  prolungamento solo se quello snap è un *Estremo*; la direzione di
  *Parallelo* è quella del segmento che vince il clic (l'id più alto, e in
  una polilinea il primo segmento); una seconda sosta sullo stesso punto
  rilascia quell'acquisizione intera. Spegnere un modo (tracciamento,
  *Prolungamento*, *Parallelo*) toglie la parte delle acquisizioni che ne
  dipende, e riaccenderlo non la riporta.

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
- **Priorità:** come l'ADR 0049, punto 5: snap a oggetto, incrocio di due guide,
  una guida, polare, orto. **Pareggi:** a pari distanza (al quadrato, sezione 3)
  vince la guida o l'incrocio che usa l'acquisizione più recente; fra la guida
  di un punto e un prolungamento della stessa acquisizione, il prolungamento
  (REGOLE-NUMERICHE.md, sezione 4, «Tracciamento degli snap», punto 6;
  [DECISIONI.md](../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano della
  0.8.1», punto 6). Se due incroci usano tutti e due l'acquisizione più
  recente, vince quello la cui altra guida è più recente; con la stessa coppia
  di guide, il primo lungo il prolungamento dall'estremo acquisito.
- **Distanza scritta:** con una sola guida diritta che aggancia il puntatore
  (prolungamento di una linea o parallela), un numero scritto dà il punto a
  quella distanza dall'origine della guida (l'estremo o il punto base), verso
  il puntatore, come l'ADR 0049, punto 6. Lungo il **prolungamento di un
  arco** la distanza si misura **lungo l'arco**, come AutoCAD: il punto sta sul
  cerchio, a un angolo `d / r` dall'estremo acquisito, nel verso che si
  allontana dall'arco (seduta del 2026-11-22: 10 su raggio 20 dà 0,5 rad;
  [DECISIONI.md](../DECISIONI.md), 2026-11-22, «Esiti della seconda seduta in
  AutoCAD della 0.8.1», punto 1). Oltre il tratto (`d / r` più grande di `2π`
  meno l'ampiezza dell'arco, confine compreso nel tratto) la guida non vale e
  il numero è una distanza diretta verso il puntatore; lo stesso per una
  distanza negativa, che cadrebbe prima dell'estremo acquisito.

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
- Le regole di pareggio sono in REGOLE-NUMERICHE.md, sezione 4.
