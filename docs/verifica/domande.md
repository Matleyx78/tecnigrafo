# Domande al modello (`.tgq`)

Catalogo delle domande con cui la modalità di controllo verifica il modello
geometrico anche dove nessun comando arriva ancora
([ADR 0011](../adr/0011-modalita-di-controllo.md), punto 3). È un **contratto**,
con la stessa regola del [linguaggio degli script](script.md): una domanda si
aggiunge, non si cambia.

Le domande esistono solo in `tecnigrafo-script`, al livello 1: il programma
distribuito non le legge.

## Il file

Testo UTF-8, con o senza BOM, righe terminate da LF o CRLF. Una riga per
istruzione; gli spazi all'inizio e alla fine della riga non contano.

| Riga | Significato |
|---|---|
| vuota, o che inizia con `;` | ignorata; con `--pending`, `; cambia con #<numero>` ferma il controllo anticipato prima del comportamento che cambia apposta ([ADR 0037](../adr/0037-controllo-anticipato-dei-riferimenti.md), punti 4–6) |
| `!tolerance <tipo> <valore>` | cambia una tolleranza, come negli [script](script.md#dettagli), **per le domande che seguono** |
| `!current <proprietà>` | cambia i valori correnti del documento delle definizioni (dalla tappa 0.6, [Layer e proprietà](#layer-e-proprietà)) |
| `!block "<NOME>" base=x,y` … `!endblock` | una definizione di blocco nel documento delle definizioni (dalla tappa 0.8, [Blocchi e geometria generata](#blocchi-e-geometria-generata)) |
| `<Nome> = <entità>` | una **definizione** |
| `<Nome> = BLOCK-IMPORT source=<nome> [target-units=N]` / `file=<nome> [target-units=N]` | il risultato dell'importazione di un DXF come blocco (dalla tappa 0.8, [Importazione di un file come blocco](#importazione-di-un-file-come-blocco)) |
| `? <domanda> <argomenti> -> <risposta attesa>` | una **domanda** |

```
; polilinea a L e mezza circonferenza sotto l'asse
A = LWPOLYLINE vertex=0,0 vertex=10,0 vertex=10,10
B = LWPOLYLINE vertex=20,0 bulge=1 vertex=30,0
? bbox B                  -> 20,-5 30,0
? length B                -> 15.707963267948966
? closest A 5,5           -> 5,0
? snaps B                 -> Endpoint 20,0; Endpoint 30,0; Midpoint 25,-5; Center 25,0
? pick 25,-5.5 tolerance=1 -> B
? window -1,-6 31,11      -> A B
? snap 27,-4.5 aperture=1 -> Center 25,0
```

### Definizioni

- Il **nome** inizia con una lettera o `_` e continua con lettere, cifre e `_`;
  `none` non è un nome. Ogni nome si definisce una volta.
- L'**entità** si scrive nel formato del [rapporto](rapporto.md#entità), con le
  libertà delle [definizioni](rapporto.md#definizioni-nelle-domande): proprietà
  in qualunque ordine, predefinite omesse, derivate ignorate.
- Le entità definite formano un documento nuovo, numerate da 257 nell'ordine
  in cui compaiono ([ADR 0023](../adr/0023-layer-e-proprieta.md), punto 1).
  Una domanda vede le entità definite **sopra** di lei: si può definire,
  chiedere, definire ancora e chiedere di nuovo.
- Dalla tappa 0.5 una definizione può anche leggere un **disegno da un
  file** ([File](#file)): il nome indica allora il disegno, che non entra nel
  documento delle entità definite.
- Dalla tappa 0.6 una definizione può anche aggiungere un **layer** o un
  **tipo di linea** al documento delle entità definite
  ([Layer e proprietà](#layer-e-proprietà)); il nome non indica un'entità, e
  come ogni nome si definisce una volta.
- Dalla tappa 0.8 una definizione può conservare il risultato, riuscito o
  fallito, di **un'importazione di blocco**. Il nome non indica un'entità e,
  come ogni nome, si definisce una volta.

### Domande

- Dopo `?` vengono il nome della domanda e i suoi argomenti, separati da spazi;
  poi `->` e la risposta attesa. Un testo tra virgolette (`"a -> b"`) è un solo
  argomento, e `->` al suo interno non conta.
- Gli argomenti con nome (`tolerance=3`, `aperture=1`, `modes=…`) vengono dopo
  quelli senza, in qualunque ordine.
- Un **punto** è `x,y`; un **rettangolo** è due angoli opposti `x1,y1 x2,y2`, in
  qualunque ordine; i bordi fanno parte del rettangolo.
- I **modi dello snap** si danno con `modes=` e i nomi separati da virgole:
  `endpoint`, `midpoint`, `center` e, dal pacchetto P13, `insertion`; dalla
  tappa 0.7 anche `intersection`, `perpendicular`, `tangent` e `quadrant`
  ([ADR 0033](../adr/0033-nuovi-snap-e-tracciamento-polare.md)).
  `modes=none` li spegne tutti. Senza `modes=` sono accesi i quattro modi
  della tappa 0.4 (`endpoint`, `midpoint`, `center`, `insertion`), così le
  domande scritte prima della 0.7 non cambiano.

## Le risposte

Ogni domanda ha una forma fissa della risposta. Il confronto:

- tra la risposta attesa e quella ottenuta, più spazi di fila contano come uno;
- i **numeri** sono uguali entro le tolleranze in vigore a quella riga (per
  difetto `1e-9` assoluto), scritti in qualunque forma (`100`, `100.0`, `1e2`);
  il resto deve coincidere;
- le risposte che sono **elenchi** si confrontano **in qualunque ordine**: ogni
  elemento atteso deve corrispondere a un elemento ottenuto, e nessuno avanza;
- `none` è un elenco vuoto, o l'assenza di una risposta.

Valori: numeri e punti come nel [rapporto](rapporto.md#numeri); `yes` / `no`;
i nomi dei tipi di snap `Endpoint`, `Midpoint`, `Center`, `Insertion` e,
dalla tappa 0.7, `Intersection`, `Perpendicular`, `Tangent`, `Quadrant`.

## Catalogo

### Su un oggetto

`<E>` è il nome di un'entità definita sopra.

| Domanda | Risposta | Significato |
|---|---|---|
| `bbox <E>` | `xmin,ymin xmax,ymax`, o `empty` | il riquadro esatto, archi compresi |
| `length <E>` | numero | la lunghezza della curva |
| `distance <E> x,y` | numero, o `inf` | la distanza del punto dalla curva |
| `closest <E> x,y` | `x,y`, o `none` | il punto della curva più vicino; a pari distanza il primo segmento, e in un arco il punto iniziale |
| `inside <E> x1,y1 x2,y2` | `yes` / `no` | l'entità è tutta nel rettangolo (selezione a finestra) |
| `crosses <E> x1,y1 x2,y2` | `yes` / `no` | l'entità tocca il rettangolo o vi sta dentro (selezione a intersezione) |
| `snaps <E> [modes=…]` | elenco di `Tipo x,y` separati da `;`, o `none` | i punti di snap dell'entità, ognuno una volta |

Come ogni tipo di entità risponde ([ADR 0008](../adr/0008-selezione-e-snap.md),
[ADR 0012](../adr/0012-linea-cerchio-arco.md),
[ADR 0013](../adr/0013-testo-e-font.md)):

| Tipo | `length` | `distance`, `closest` | `snaps` |
|---|---|---|---|
| `LWPOLYLINE` | somma dei segmenti, archi compresi | dalla curva, non dall'interno; un vertice solo è un punto; senza vertici `inf` e `none` | estremo di ogni vertice, medio di ogni segmento (sull'arco), centro di ogni arco |
| `LINE` | lunghezza del segmento | dal segmento | estremi, medio |
| `ARC` | lunghezza dell'arco | dall'arco, mai dal resto del cerchio | estremi, medio sull'arco, centro |
| `CIRCLE` | circonferenza | dalla circonferenza: il centro è a distanza pari al raggio | centro |
| `TEXT` | `none` | dal parallelogramma d'ingombro, che è una regione: distanza 0 e punto stesso all'interno | punto d'ancoraggio; `position` per `Aligned` e `Fit` (ADR 0022, punto 5) |

### Sul documento

| Domanda | Risposta | Significato |
|---|---|---|
| `pick x,y tolerance=<t>` | un nome, o `none` | l'entità scelta con un clic in `x,y` entro `t` unità del disegno; a pari distanza vince l'identificativo più alto |
| `window x1,y1 x2,y2` | elenco di nomi separati da spazi, o `none` | le entità selezionate a finestra |
| `crossing x1,y1 x2,y2` | elenco di nomi separati da spazi, o `none` | le entità selezionate a intersezione |
| `snap x,y aperture=<a> [modes=…]` | `Tipo x,y`, o `none` | lo snap per il puntatore in `x,y` con l'apertura `a` in unità del disegno, con le regole di priorità dell'ADR 0008: tra estremi e medi entro l'apertura vince il più vicino (a pari distanza l'estremo), altrimenti il centro (e dalla tappa 0.4 l'inserimento) dell'entità su cui passa il puntatore |
| `snap x,y aperture=<a> [modes=…] [from=x,y]` | come sopra | dalla tappa 0.7, con i modi nuovi e le priorità dell'[ADR 0033](../adr/0033-nuovi-snap-e-tracciamento-polare.md), punto 5; `from=` è il punto base da cui partono perpendicolare e tangente, che senza `from=` non danno nulla |

### Costruzioni

Il risultato di un algoritmo che crea geometria, scritto come una
[definizione](rapporto.md#entità). Realizzate dal pacchetto P6.

| Domanda | Risposta | Significato |
|---|---|---|
| `circle-through x1,y1 x2,y2 x3,y3` | `CIRCLE center=… radius=…`, o `collinear` | il cerchio per tre punti; `collinear` per punti allineati o coincidenti secondo l'ADR 0012, punto 4 |
| `arc-through x1,y1 x2,y2 x3,y3` | `ARC center=… radius=… start-angle=… end-angle=…`, o `collinear` | l'arco dal primo punto per il secondo fino al terzo, scritto in senso antiorario come nel DXF |

### Testo

Realizzate dai pacchetti P11 (`text-width`, `font-metrics`) e P13 (`box`). Le
misure sono **in unità dell'altezza delle maiuscole** (ADR 0013, punto 3): un
testo alto 1, con fattore di larghezza 1.

| Domanda | Risposta | Significato |
|---|---|---|
| `text-width "<testo>"` | numero | la larghezza di una riga, somma delle larghezze dei caratteri, senza crenatura; i codici di controllo (`%%c`) **non** si convertono: si misurano i caratteri come sono scritti |
| `font-metrics` | `ascent=<a> descent=<d>` | ascendente (> 1) e discendente (< 0) del font incluso |
| `box <E>` | quattro punti `x,y` separati da spazi | il parallelogramma d'ingombro di un `TEXT`, nell'ordine definito sotto (ADR 0022, punto 5) |

`box` trasforma, nell'ordine, i punti `(0, d·h)`, `(w, d·h)`, `(w, a·h)`,
`(0, a·h)` del sistema del testo: `w = L(valore)·h·f`, con altezza `h` e
fattore di larghezza `f` usati (ricavati per `Aligned` e `Fit`). Allineamento,
obliquo, specchiatura e rotazione seguono l'[ADR 0022](../adr/0022-allineamenti-obliquo-specchiatura-testo.md).
Senza specchiatura i punti girano in senso antiorario; con una sola
specchiatura in senso orario. `bbox` contiene i quattro angoli; selezione,
`distance` e `closest` usano la regione del parallelogramma. Lo snap
inserimento è il punto d'ancoraggio (`alignment-point` salvo `Left` sulla
linea di base), oppure `position` per `Aligned` e `Fit`.

### Modifiche (tappa 0.7)

Le domande della tappa 0.7, pubblicate con il suo piano perché i riferimenti si
possano scrivere prima del lavoro. Ognuna indica il pacchetto che la realizza;
fino ad allora i casi che la usano stanno in `in-attesa/`. Le entità delle
risposte si scrivono come [definizioni](rapporto.md#entità).

**Trasformazioni** (P4, [ADR 0031](../adr/0031-modifiche-alle-entita.md), punti 1–4)

| Domanda | Risposta | Significato |
|---|---|---|
| `transform <E> move dx,dy` | un'entità | `E` spostata di `(dx, dy)` |
| `transform <E> rotate <gradi> x,y` | un'entità | `E` ruotata dell'angolo, in gradi antiorari, intorno a `x,y` |
| `transform <E> scale <f> x,y` | un'entità, o `invalid` | `E` scalata del fattore `f` intorno a `x,y`; `invalid` se `f` non è maggiore di 0 |
| `transform <E> mirror x1,y1 x2,y2 [mirrtext=0\|1]` | un'entità, o `invalid` | `E` riflessa rispetto alla retta per i due punti, con `MIRRTEXT` 0 se manca; `invalid` se i due punti coincidono |

**Grip** (P9, [ADR 0034](../adr/0034-grip.md))

| Domanda | Risposta | Significato |
|---|---|---|
| `grips <E>` | elenco di punti `x,y` separati da `;` | i grip dell'entità, ognuno una volta |
| `grip <E> x1,y1 x2,y2` | un'entità, `invalid`, o `none` | `E` dopo aver portato in `x2,y2` il grip che sta in `x1,y1`; `invalid` se il risultato non esiste (ADR 0034, punto 5); `none` se in `x1,y1` non c'è un grip |

**Intersezioni, lato e tolleranze** (P14, richieste
[Matleyx78/solid-space#20](https://github.com/Matleyx78/solid-space/issues/20) e
[#23](https://github.com/Matleyx78/solid-space/issues/23),
[ADR 0035](../adr/0035-taglio-offset-raccordi-unione.md), punto 1, e
[ADR 0036](../adr/0036-tolleranze-di-tecnigrafo.md))

Le risposte vengono dalla facciata di `core` sopra solid-space: i valori
geometrici attesi si prendono dall'archivio dei casi di solid-space quando il
caso c'è ([ADR 0030](../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto
14). Una **posizione** si scrive `<segmento>:<dove>`: il segmento è l'indice del
segmento di una polilinea (0 per le altre entità), `<dove>` è `before`, `start`,
`inside`, `end` o `after` (prima dell'inizio, all'inizio, dentro, alla fine, oltre
la fine), decisa esattamente da solid-space. Le stazioni non compaiono. `units=`
è `INSUNITS` del disegno, 4 (millimetri) se manca.

| Domanda | Risposta | Significato |
|---|---|---|
| `intersections <A> <B> [extend=a\|b\|both]` | elenco separato da `;` di `x,y <contatto> <posizione su A> <posizione su B>` e di `overlap x1,y1 x2,y2`, o `none` | dove si incontrano le curve di `A` e `B`; `<contatto>` è `crossing`, `touching` o `same-curve`; un vertice di una polilinea compare una volta, come `start` del segmento che segue; `overlap` è un tratto in comune, dai suoi estremi nel verso di `A`. Con `extend=` la curva indicata si prende con il suo prolungamento (la retta di una linea, il cerchio di un arco; di una polilinea aperta, il primo e l'ultimo segmento). Il testo non ha curva: `none` |
| `self-intersections <P>` | come `intersections` | dove la polilinea `P` incontra se stessa, senza i vertici comuni di due segmenti consecutivi |
| `side <E> x,y` | `left`, `right` o `on` | da che parte della retta o del cerchio della curva sta il punto, guardando nel verso della curva, come `side()` di solid-space (`side.hpp`: per un cerchio e un arco a sinistra l'interno, per un segmento curvo orario di polilinea a destra); per una polilinea, del segmento più vicino al punto, a pari distanza il primo |
| `coincident x1,y1 x2,y2 [units=<n>]` | `yes` / `no` | i due punti coincidono per la regola delle tolleranze del disegno |
| `degenerate <E> [segment=<i>] [units=<n>]` | `yes` / `no` | la curva di una linea, di un arco o di un cerchio, o il segmento `i` di una polilinea, è degenere per la regola (ogni due suoi punti distano al più la tolleranza) |

**Curva parallela** (P17, richiesta [Matleyx78/solid-space#22](https://github.com/Matleyx78/solid-space/issues/22),
[ADR 0035](../adr/0035-taglio-offset-raccordi-unione.md), punti 10–12)

| Domanda | Risposta | Significato |
|---|---|---|
| `offset <E> <d> x,y [units=<n>]` | un'entità, oppure `vanished`, `failed` o `no-side` | la curva a distanza `d` (> 0) dal lato del punto `x,y`; `vanished` quando non resta nulla, `failed` quando solid-space non può fare la copia (il suo esito `failed`, o una diagnosi di `offsetCurve` o di `offsetPolyline`: `offset.hpp`) e quando l'entità non dà un lato (una curva esattamente puntiforme, una polilinea senza segmenti o con soli segmenti puntiformi, dati che solid-space rifiuta; decisione B5 della #464, 2026-11-01), `no-side` solo quando il punto sta esattamente sulla retta o sul cerchio |

**Raccordi e tangenze** (P19, richiesta [Matleyx78/solid-space#21](https://github.com/Matleyx78/solid-space/issues/21),
ADR 0035, punti 13 e 14)

Gli esiti che non costruiscono nulla: `none` (non esiste), `infinite` (le curve
stanno sulla stessa retta o sullo stesso cerchio, o simili), `ambiguous` (un
punto sta esattamente sulla retta o sul cerchio dell'altra curva),
`not-allowed` (un'entità che il comando non prende), `out-of-domain` (esiste,
ma una sua costruzione cade oltre il dominio di solid-space: la diagnosi
`outOfDomain` sul raggio di `tangency.hpp`, o lo spigolo che `intersect` omette;
aggiunto il 2026-11-01, ADR 0035, punti 13–15), `radius-too-large` (solo
`fillet`: la parte tenuta farebbe tornare una linea indietro oltre l'estremo
che tiene, l'esito `radiusTooLarge` di `tangency.hpp`; aggiunto il 2026-11-05,
dalla decisione del 2026-11-04 sulla Matleyx78/solid-space#187). Un raggio
oltre 2^50 dà `none`, e così una parte tenuta che il modello non rappresenta
(un arco che si chiuderebbe in un cerchio, P15). Gli smussi hanno gli stessi
esiti, tranne `radius-too-large`, più `distance-too-large`: una distanza, o la
lunghezza, più lunga della parte tenuta della sua linea, o con l'angolo il
secondo punto oltre la parte tenuta di `B` (decisioni del 2026-11-05, #507 e
#508). Con `polyline` un vertice che darebbe uno di questi esiti resta com'è e
conta in `short`.

Con il taglio attivo, un'entità la cui parte tenuta è vuota o degenere per la
regola del disegno si cancella, e non compare nella risposta: per esempio
`fillet` sulle linee da 0,0 a 1,0 e da 0,0 a 0,1, scelte in 1,0 e in 0,1, con
raggio 1, risponde solo `new ARC …`. Con `trim=no` le due entità compaiono
sempre, invariate. `trim=no` vale anche per le polilinee, come in AutoCAD: sui
segmenti di una polilinea e con `polyline` la polilinea resta com'è e la
geometria nuova compare a parte, come `new ARC …` (decisione B14 della #464,
2026-11-01, e B6, rivista il 2026-11-02 dopo la prova in AutoCAD, #468).

| Domanda | Risposta | Significato |
|---|---|---|
| `fillet <A> x1,y1 <B> x2,y2 radius=<r> [trim=no] [units=<n>]` | elenco separato da `;` di `<nome> <entità>`, o un esito | il raccordo tra `A`, scelta in `x1,y1`, e `B`, scelta in `x2,y2`: le due entità come diventano (`A …`, `B …`) e l'arco nuovo (`new ARC …`; nessuno con raggio 0) |
| `fillet <P> x1,y1 x2,y2 radius=<r> [trim=no] [units=<n>]` | `P <entità>`, con `trim=no` anche `; new ARC …`, o un esito | il raccordo tra i due segmenti della polilinea `P` più vicini ai due punti; con `trim=no` `P` resta com'è e l'arco nuovo compare a parte (nessuno con raggio 0) |
| `fillet <P> polyline radius=<r> [trim=no] [units=<n>]` | `<entità> done=<n> short=<m>`; con `trim=no` `<entità>`, poi uno `; new ARC …` per vertice raccordato, nell'ordine della visita, e `done=<n> short=<m>` | la polilinea `P` con tutti i vertici raccordati (opzione *Polyline*), quanti vertici raccordati e quanti con segmenti troppo corti; i vertici originali in ordine, ognuno sulla polilinea già modificata dai precedenti (decisione B15 della #464, 2026-11-01). Con `trim=no` la polilinea resta com'è e ogni vertice si giudica sui suoi due segmenti originali (decisione B6, rivista il 2026-11-02) |
| `tangents x,y <E>` | elenco di punti `x,y` separati da `;`, o `none` | i punti di tangenza dal punto a un cerchio, un arco o gli archi di una polilinea, solo quelli sulla curva; `none` per un punto dentro il cerchio o una curva dritta; il punto stesso se sta esattamente sul cerchio. Per una polilinea, per indice di segmento e poi nell'ordine del segmento; un vertice comune a due segmenti compare una volta; un segmento rifiutato o la cui costruzione è oltre il dominio non dà punti, e gli altri restano (decisione B7 della #464, 2026-11-01) |
| `circle-ttr <A> x1,y1 <B> x2,y2 radius=<r>` | `CIRCLE center=… radius=…`, o un esito | il cerchio di raggio `r` tangente ad `A` e `B`, quello con i punti di tangenza più vicini ai due punti, come `CIRCLE` *Ttr* |

**Smussi** (P21, ADR 0035, punto 15)

| Domanda | Risposta | Significato |
|---|---|---|
| `chamfer <A> x1,y1 <B> x2,y2 distances=<d1>,<d2> [trim=no]` | come `fillet`, con `new LINE …` | lo smusso con le due distanze, la prima su `A` |
| `chamfer <A> x1,y1 <B> x2,y2 angle=<l>,<gradi> [trim=no]` | come `fillet`, con `new LINE …` | lo smusso con la lunghezza su `A` e l'angolo: dal primo punto, sulla parte tenuta di `A` alla lunghezza dallo spigolo, la direzione verso lo spigolo ruotata dell'angolo verso il lato della parte tenuta di `B`; `none` se quella retta è parallela a `B` o la incontra nello spigolo o dall'altra parte; `distance-too-large` se la incontra dalla parte tenuta ma oltre la sua fine (decisione del 2026-11-05, #508) |
| `chamfer <P> x1,y1 x2,y2 distances=<d1>,<d2> [trim=no]` (anche `angle=<l>,<gradi>`) | `P <entità>`, con `trim=no` anche `; new LINE …`, o un esito | lo smusso tra i due segmenti dritti della polilinea `P` più vicini ai due punti, il primo con la prima distanza; con `trim=no` `P` resta com'è e la linea nuova compare a parte (nessuna con le due distanze zero) |
| `chamfer <P> polyline distances=<d1>,<d2> [trim=no]` | `<entità> done=<n> short=<m>`; con `trim=no` `<entità>`, poi uno `; new LINE …` per vertice smussato e `done=<n> short=<m>` | la polilinea `P` con tutti i vertici tra segmenti dritti smussati, nell'ordine di `fillet … polyline`; con `trim=no` come per `fillet` (decisione B6, rivista il 2026-11-02) |

Un punto la cui proiezione sulla retta della sua linea è esattamente lo
spigolo dà `ambiguous`; una sola distanza zero dà `none`; una distanza (o la
lunghezza) uguale alla parte tenuta cancella quella linea (decisioni B8 e B9
della #464, 2026-11-01); una più lunga dà `distance-too-large` (decisione del
2026-11-05, #507), tranne quando l'altra distanza è zero: allora `none`, in
tutti e due gli ordini (decisione del 2026-11-05, #508).

**Unione** (P22, richiesta [Matleyx78/solid-space#23](https://github.com/Matleyx78/solid-space/issues/23),
ADR 0035, punti 17–20)

| Domanda | Risposta | Significato |
|---|---|---|
| `join <A> <B> … [circle=no] [units=<n>]` | elenco separato da `;` di `<nome> <entità>`, o `none` | il risultato di `JOIN` interattivo sulle entità nominate, con `A` come sorgente e le altre nell'ordine scritto: le entità che restano come diventano (`A …`), quella nuova se il tipo cambia (`new LWPOLYLINE …`); `none` se non si unisce nulla. Una linea o un arco solo se tutte le entità sono linee allineate alla sorgente o archi del suo cerchio; altrimenti la catena (decisione B16 della #464, 2026-11-01). Gli archi dello stesso cerchio si coprono in senso antiorario a partire dall'inizio della sorgente; se arrivano al giro intero la risposta è il cerchio (`new CIRCLE …`), come con *Yes* alla domanda *Convert to circle?*, e con `circle=no`, come con *No*, è `none` (decisione B11, rivista il 2026-11-02) |

### Polilinee (tappa 0.8)

Le domande della conversione, dell'inversione e dell'unione con distanza di
`PEDIT` ([ADR 0050](../adr/0050-pedit.md), punti 2–4), pubblicate prima del
pacchetto che le realizza perché i riferimenti si possano scrivere prima del
lavoro (decisione del 2026-11-15, F9). Chiedono delle funzioni di `core`,
non del comando: le entità delle risposte si scrivono come
[definizioni](rapporto.md#entità), senza proprietà, come in `join`; `units=`
è `INSUNITS` del disegno, 4 (millimetri) se manca.

| Domanda | Risposta | Significato |
|---|---|---|
| `to-polyline <E>` | `LWPOLYLINE …` | la linea o l'arco `E` come polilinea aperta di un segmento (`toPolyline`): dall'inizio della linea alla sua fine; per l'arco dal punto dell'angolo iniziale a quello dell'angolo finale, con il bulge dell'arco (antiorario, positivo). Un'entità che non è `LINE` né `ARC` è un errore del caso |
| `reverse <P>` | `LWPOLYLINE …` | la polilinea `P` invertita (`reversed`, opzione *Reverse*), con le regole della scheda PT32, punto 2: vertici in ordine opposto, bulge con il segno cambiato sul segmento opposto, larghezze iniziale e finale scambiate, una chiusa con lo stesso primo vertice, i dati dell'ultimo vertice di un'aperta sul nuovo ultimo vertice. Un'entità che non è `LWPOLYLINE` è un errore del caso |
| `pedit-join <P> <B> … fuzz=<d> [type=extend\|add\|both] [units=<n>]` | `<P> LWPOLYLINE … added=<n> consumed=<nomi>`, o `none` | l'unione con distanza (`joinWithGaps`) con la polilinea `P` come sorgente e le altre entità come `others`, nell'ordine scritto; `fuzz=` è la distanza di tolleranza (≥ 0, obbligatoria), `type=` il tipo di unione, *Extend* se manca, come *Jointype* di AutoCAD. La risposta è la polilinea unita, il numero dei segmenti aggiunti, ponti compresi, e i nomi degli oggetti usati nell'ordine di aggancio, separati da `,`; `none` se non si unisce nessun oggetto. Le polilinee chiuse non si uniscono mai; la quota non entra nella domanda (la regola della quota è del comando `PEDIT`, PT33). Una sorgente che non è `LWPOLYLINE` è un errore del caso (la conversione si chiede con `to-polyline`) |

Esempio, dalla scheda PT32 (le due curve stanno sulla stessa retta, quindi
non c'è un punto d'intersezione isolato):

```
S = LWPOLYLINE vertex=-4,0 vertex=0,0
L = LINE start=1,0 end=3,0
? pedit-join S L fuzz=2 type=extend -> none
? pedit-join S L fuzz=2 type=both   -> S LWPOLYLINE closed=no vertex=-4,0 vertex=0,0 vertex=1,0 vertex=3,0 added=2 consumed=L
```

Le realizza il pacchetto PT32 della 0.8; fino ad allora rispondono
`error: question not implemented yet`.

### Retini (tappa 0.8)

Le domande del contorno rilevato da un punto e dei contorni scelti di un
retino ([ADR 0048](../adr/0048-retini.md), punti 5 e 6), come le specifica la
scheda [PT43](../tappe/0.8/PT43-contorni-dei-retini.md), punto 5, pubblicate
prima del pacchetto che le realizza perché i riferimenti si possano scrivere
prima del lavoro (decisione del 2026-11-17, F12). Chiedono delle funzioni di
`core` (`detectHatchBoundary`, `chosenHatchContours`), non del comando.
`<loop>` è un `loop=…` con i suoi elementi, nella forma della riga `HATCH` di
[rapporto.md](rapporto.md) (PT9): i segnali `external` (profondità 0),
`outermost` (profondità 1) e `derived` (solo per i contorni rilevati da un
punto), poi `polyline` o `edges`. Un cerchio è un contorno a lati con un solo
`circle=`; una polilinea chiusa un contorno a polilinea con gli stessi
vertici e bulge.

| Domanda | Risposta | Significato |
|---|---|---|
| `hatch-boundary x,y [units=<n>]` | `enclosed <loop> <loop>…`; `on-curve <nome>`; `not-enclosed`, seguito da ` open=x,y` per ogni estremo libero; ognuna con ` incomplete` in fondo se qualcosa è stato scartato | il contorno e le isole rilevati dal punto (`detectHatchBoundary`) fra le curve candidate della scheda PT43, punto 2 (`LINE`, `ARC`, `CIRCLE` e `LWPOLYLINE` sui layer accesi e non congelati, nell'ordine del documento), nell'ordine di solid-space: prima il contorno, poi le isole; `<nome>` è l'entità su cui sta il punto; `units=` è `INSUNITS` del disegno, 4 se manca |
| `hatch-contours <E1> <E2> …` | `<loop> <loop>…`, poi ` left-out=<nome>` per ogni entità scartata; `none` se non c'è nulla | i contorni scelti (`chosenHatchContours`) nell'ordine dei nomi, con i segnali delle loro profondità: un `CIRCLE` o una `LWPOLYLINE` chiusa danno un contorno; ogni altra entità, anche una polilinea aperta o un'ellisse, va in `left-out` |

Le realizza il pacchetto PT43 della 0.8; fino ad allora rispondono
`error: question not implemented yet`.

### Blocchi e geometria generata (tappa 0.8)

La geometria generata delle entità composte e le definizioni di blocco nei
file di domande ([ADR 0044](../adr/0044-entita-composte-e-geometria-generata.md),
punto 2; [ADR 0045](../adr/0045-blocchi-e-attributi.md)), pubblicate prima dei
pacchetti che realizzano i tipi perché i riferimenti si possano scrivere prima
del lavoro (scheda PT9). Le entità della tappa 0.8 si definiscono nel formato
del [rapporto](rapporto.md#tappa-08-entità-composte); finché il pacchetto di
un tipo non è unito, la sua definizione ferma il file con
`<TIPO> is not supported yet.` ([Errori](#errori)).

| Domanda | Risposta | Significato |
|---|---|---|
| `generated <E>` | elenco di forme separate da `;`, **in ordine**, o `none` | la geometria generata dell'entità (ADR 0044, punto 2), nell'ordine in cui si disegna: ogni forma nel formato delle entità (`LINE`, `ARC`, `CIRCLE`, `ELLIPSE`, `LWPOLYLINE`, `TEXT`, `SOLID`, e `REGION` con i suoi `loop=` come `HATCH`), seguita dalle proprietà effettive diverse dal predefinito e, per ultime, dalle derivate |

- Le **forme** si scrivono come le entità delle risposte di `join`, senza
  proprietà della tappa 0.6; un `TEXT` ha la sua derivata `~box`. Una
  **`REGION`** è una regione piena (pari e dispari): `REGION` e, per ogni
  anello, `loop=polyline` seguito dai suoi `vertex=` e `bulge=` come in
  `LWPOLYLINE` (`REGION loop=polyline vertex=0,0 vertex=10,0 vertex=10,10`); con
  una trasformazione diversa dall'identità ha anche la derivata
  `~affine=a,b,c,d,e,f`, i coefficienti di `x' = a·x + c·y + e`,
  `y' = b·x + d·y + f`.
- Le **proprietà effettive** sono quelle della domanda `effective`
  (`color=`, `linetype=`, `ltscale=`, `lineweight=`), ognuna scritta solo se
  diversa da quella di un'entità con le proprietà predefinite in una sessione
  nuova (`color=255,255,255 linetype="Continuous" ltscale=1
  lineweight=0.25`); `locked=yes` solo per una forma su un layer bloccato.
  Una forma su un layer spento o congelato non si genera.
- La risposta si confronta **nell'ordine**: le forme sono quelle che la
  grafica disegna, una dopo l'altra.
- `generated` di un'entità delle tappe precedenti è l'entità stessa:

```
L = LINE start=0,0 end=10,0
? generated L -> LINE start=0,0 end=10,0
```

**Definizioni di blocco.** `!block "NOME" base=x,y` apre una definizione di
blocco nel documento delle definizioni e `!endblock` la chiude: le
definizioni di entità fra le due righe entrano nel blocco, nell'ordine, e non
nel documento. Così si definiscono `INSERT` da interrogare prima che esistano
i comandi:

```
!block "TG_EL" base=0,0
E1 = LINE start=0,0 end=10,0
!endblock
I = INSERT block="TG_EL" insertion=100,50
? generated I -> LINE start=100,50 end=110,50
```

Il nome segue le regole dei nomi delle tabelle (ADR 0023, punto 1) e si
definisce una volta; un `!block` dentro un altro, un `!endblock` senza
`!block` o un file che finisce dentro un blocco sono errori del file.

Chi le realizza: `generated` il pacchetto PT9 della 0.8, e risponde per i tipi
già realizzati; `!block` e `!endblock` il pacchetto PT16, quando
`BlockDefinition` ha il punto base e le entità. Fino ad allora `!block` e
`!endblock` sono righe non valide del file, come ogni direttiva sconosciuta.

### File

Le domande sui disegni letti da un file: la lettura e la scrittura dei
formati della tappa 0.5 e la conservazione dei dati non gestiti
([ADR 0018](../adr/0018-conservazione-dati-non-gestiti.md),
[ADR 0021](../adr/0021-comandi-dei-file.md)). Realizzate dal pacchetto P8;
le risposte dipendono dal modulo `io` (pacchetti P10–P14).

**Definizioni.** Il nome indica un disegno letto con il modulo `io`:

| Riga | Significato |
|---|---|
| `D = DXF source=<nome>` | legge con `io::readDxf` il file `<nome>` di `tests/verifica/sorgenti.json`, dalla cartella di `--sources` |
| `D = DXF file=<nome>` | lo stesso con un file della cartella del caso ([casi.md](casi.md#file-del-caso)) |
| `C = CADX source=<nome>` / `C = CADX file=<nome>` | lo stesso con `io::readCadx` |

- `<nome>` segue le regole dei nomi degli [script](script.md#file): per
  `source=` lettere minuscole, cifre e `-`; per `file=` un nome relativo,
  senza `..`.
- Un errore di lettura (file che manca, nome non valido, `source=` senza
  `--sources`, file danneggiato) rende il file di domande **non valido**,
  con il messaggio di `io`. Finché `io` non sa leggere il formato
  (`NotImplemented`, prima dei pacchetti P10 e P14), la definizione ferma il
  file con "not implemented yet", come un tipo di entità non ancora
  realizzato: il caso non è superato.
- Il nome di un disegno non si ridefinisce, né come disegno né come entità.

**Le entità di un disegno.** Nelle domande su un oggetto (`bbox`, `length`,
`distance`, `closest`, `inside`, `crosses`, `snaps`, `box`) un'entità di un
disegno letto si indica con `D:<handle>`: il nome del disegno, `:` e
l'handle dell'entità in esadecimale, maiuscolo o minuscolo (`D:2F`). Un
handle senza entità è un errore del file di domande. Un'entità **non
gestita** invece non lo è, perché un pacchetto successivo può gestirne il
tipo ([casi.md](casi.md), "Ciò che il catalogo prevede ma il programma non ha
ancora"): la domanda risponde `error: D:2F is an unhandled LINE entity` e
non è superata. La domanda tratta l'entità da sola, come se fosse l'unica
definita.

| Domanda | Risposta | Significato |
|---|---|---|
| `entity D <handle>` | un'entità nel formato del [rapporto](rapporto.md#entità), `unhandled <TIPO>`, o `none` | l'entità con quell'handle: gestita, non gestita (con il suo tipo DXF), o assente (anche l'handle di una tabella o di un oggetto) |
| `count D` | elenco di `TIPO n` separati da `;`, o `none` | le entità gestite per tipo, in ordine alfabetico |
| `unhandled D` | elenco di `TIPO n` separati da `;`, o `none` | lo stesso per le entità non gestite |
| `version D` | `AC1015`…, o `none` | `$ACADVER` letto; `none` se il file non lo dice |
| `units D` | intero | `INSUNITS` del disegno, come lo dà la lettura |
| `warnings D` | elenco di avvisi separati da `;`, o `none` | gli avvisi della lettura, come li scrive `io` |
| `roundtrip D [as=dxf\|cadx]` | `identical`, o `different: <la prima differenza>` | scrive il disegno e lo rilegge, e confronta il file riscritto con l'originale (sotto); per difetto nello stesso formato e nella stessa versione |
| `open-error source=<nome>` / `open-error file=<nome>` | il messaggio tra virgolette, o `none` | prova a leggere il file e dice l'errore della lettura, con la sua riga (`"line 812: expected a group code, found \"abc\""`); `none` se si legge |

- Gli elenchi si confrontano in qualunque ordine, come gli altri.
- `open-error` legge un `.dxf` con `io::readDxf` e un `.cadx` con
  `io::readCadx`, secondo l'estensione, come `OPEN`; i file di
  `sorgenti.json`, che non hanno estensione, sono DXF. Un file che manca o un
  nome non valido è un errore del file di domande, non una risposta.
- Un errore di `io` durante `roundtrip` (per esempio un formato che non sa
  ancora scrivere) risponde `error: <messaggio>`.

**`roundtrip`** applica la definizione di "identico" dell'ADR 0018, punto 6.
Confronta, **a livello di gruppi** (`io::readGroups`), il file **originale**
con quello **riscritto**: così non basta che il lettore sia coerente con sé
stesso.

1. Scrive il disegno letto: in DXF (`as=dxf`) nella versione letta, o R2018
   per un disegno letto da un `.cadx`; in `.cadx` (`as=cadx`) con
   `io::writeCadx`. Rilegge il file scritto: un errore è una differenza
   (`different: the rewritten file cannot be read: …`).
2. Prende i gruppi dell'originale e del riscritto. Di un `.cadx` si prendono
   i gruppi del suo `drawing.dxf` come lo riscrive il modello (in R2018):
   `tecnigrafo-script` non apre gli archivi.
3. Confronta i gruppi uno per uno, nell'ordine: stesso codice e stesso
   valore, carattere per carattere. Fanno eccezione i numeri delle **entità
   gestite**, che si riscrivono dal modello e sono uguali entro le
   tolleranze in vigore (`1.0` e `1`); il valore di `$HANDSEED`, che può
   cambiare; e, se le versioni sono diverse, i valori di `$ACADVER` e
   `$DWGCODEPAGE`.

La prima differenza si descrive con la riga dell'originale:
`different: line 812: 10 "1.5" became 10 "2.5"`,
`different: line 900: 0 "HATCH" is missing`,
`different: the rewritten file has more groups: 0 "CIRCLE"`.

```
; un DXF pubblico: conservazione e conteggi (valori da ezdxf)
D = DXF source=librecad-sample
? version D               -> AC1015
? count D                 -> ARC 2; LINE 12
? unhandled D             -> HATCH 1; INSERT 3
? entity D 2F             -> LINE start=0,0 end=100,0
? bbox D:2F               -> 0,0 100,0
? roundtrip D             -> identical
? roundtrip D as=cadx     -> identical
? open-error file=troncato.dxf -> "line 41: the file ends inside a group"
```

I numeri dell'esempio mostrano solo la forma.

### Importazione di un file come blocco

Queste definizioni e domande, realizzate da PT19, espongono direttamente
`io::importBlock` prima che il comando `-INSERT` esista. Non applicano il
risultato e non cambiano il documento delle definizioni.

Fino a PT19 la definizione ferma il file con `not implemented yet` e le
domande rispondono `error: question not implemented yet`.

**Definizione.** `I = BLOCK-IMPORT source=<nome>` cerca il DXF come le altre
definizioni `source=`; `I = BLOCK-IMPORT file=<nome>` lo cerca nella cartella
del caso. Chiama `io::importBlock` passando come bersaglio una copia del
documento delle definizioni nello stato raggiunto **prima** della riga: vi
entrano quindi entità, layer, tipi di linea, stili e blocchi dichiarati sopra,
insieme ai valori correnti e a `nextId`. L'argomento facoltativo
`target-units=N`, con `N` intero da 0 a 24, imposta `$INSUNITS` della sola
copia bersaglio; se manca conserva le unità del documento (0 in una sessione
nuova). Il nome del file passato a
`importBlock` è quello scritto dopo `file=`; per `source=` è il percorso
risolto da `sorgenti.json`, ma il nome della definizione principale resta il
nome del file della sorgente dichiarato nel catalogo, senza estensione. Le
regole e gli errori dei nomi sono quelli delle definizioni di [file](#file).
La definizione conserva anche un `io::Error`: così un rifiuto previsto si può
interrogare e non rende non valido il `.tgq`.

**Domande.** Nelle risposte seguenti gli identificativi sono handle in
esadecimale maiuscolo, senza prefisso. Gli elenchi separati da `;` si
confrontano in qualunque ordine; le entità dentro un blocco restano invece
nell'ordine della definizione.

| Domanda | Risposta | Significato |
|---|---|---|
| `import-error <I>` | il messaggio tra virgolette, o `none` | l'errore restituito da `importBlock`; `none` se è riuscito |
| `import-definition <I>` | una definizione nel formato sotto | il campo `definition` |
| `import-nested <I>` | elenco di definizioni separate da `;`, o `none` | il campo `nested` |
| `import-layers <I>` | elenco di record separati da `;`, o `none` | il campo `layers` |
| `import-linetypes <I>` | elenco di record separati da `;`, o `none` | il campo `linetypes` |
| `import-text-styles <I>` | elenco di record separati da `;`, o `none` | il campo `textStyles` |
| `import-dim-styles <I>` | elenco di record separati da `;`, o `none` | il campo `dimStyles` |
| `import-unit-factor <I>` | numero | il campo `unitFactor` |
| `import-next-id <I>` | handle esadecimale | il campo `nextId` |
| `import-groups <I> <handle>` | gruppi `codice="valore"` separati da spazi, o `none` | l'handle e tutti i gruppi DXF conservati nell'oggetto restituito con quell'handle, nell'ordine definito sotto; comprende proprietario, riferimenti e reattori |

Una **definizione** risponde `name="NOME" base=x,y units=N entities=[H
<entità>; …]`: `units=` è sempre presente; `entities=none` per il blocco
vuoto. Le entità gestite e non gestite sono intercalate nell'ordine originale
della definizione. Ogni entità gestita ha prima il proprio handle `H` e poi il
formato del [rapporto](rapporto.md#entità); una non gestita risponde `H
unhandled TIPO`, come la risposta di `entity D <handle>`. Un record di tabella comincia sempre con
`id=H name="NOME"` e continua con le proprietà della rispettiva riga di
rapporto: [`layers:` e `linetypes:`](rapporto.md#layer-tipi-di-linea-e-valori-correnti),
[`text styles:` e `dimension styles:`](rapporto.md#stili-e-blocchi).
Si scrivono anche i valori predefiniti, affinché due record siano distinguibili
senza dipendere dallo stato del bersaglio. Le definizioni annidate e i record
mantengono l'ordine dei rispettivi vettori di `BlockImport`.

Tranne `import-error`, una domanda su un'importazione fallita risponde
`error: <messaggio>` e il caso non è superato. `import-groups` con un handle
che non appartiene a definizione, annidati o record restituiti è un errore del
file di domande. Il valore di ogni gruppo è quotato con gli escape dei testi;
in particolare rende osservabili i gruppi 330, 340, 360 e i gruppi 102 dei
reattori dopo la rinumerazione. Per un'entità gestita la risposta comincia con
il gruppo sintetico `5` del suo handle e prosegue concatenando, in quest'ordine,
`beforeEntity`, `common`, `specific`, `placement` ed `extended`. Per un'entità
non gestita restituisce tutti i suoi gruppi grezzi nell'ordine conservato,
compreso il gruppo `5` nella sua posizione. Per un record di tabella comincia
con il gruppo sintetico `5` del suo id e prosegue con `extras`. Per una
definizione, `recordId`, `blockId` ed `endBlockId` rispondono rispettivamente
con il gruppo sintetico `5` seguito da `recordExtras`, `blockExtras` ed
`endBlockExtras`. Un gruppo sintetico `5` compare anche quando non ci sono
altri gruppi, perciò `none` vale soltanto per un oggetto grezzo senza gruppi.

```
L = LAYER name="Muri" color=1
I = BLOCK-IMPORT file=pezzo.dxf target-units=6
? import-error I          -> none
? import-definition I     -> name="pezzo" base=0,0 units=4 entities=[101 LINE start=0,0 end=10,0 layer="Muri"]
? import-unit-factor I    -> 0.001
? import-next-id I        -> 108
? import-groups I 101     -> 5="101" 330="100"
```

I valori dell'esempio mostrano solo la forma.

### Layer e proprietà

Le domande della tappa 0.6 su layer, colori, tipi di linea e spessori
([ADR 0023](../adr/0023-layer-e-proprieta.md),
[ADR 0024](../adr/0024-colori.md),
[ADR 0025](../adr/0025-tipi-di-linea-e-spessori.md)). Le proprietà delle
entità si scrivono come nel [rapporto](rapporto.md#tappa-06-layer-e-proprietà).

**Definizioni.** Il documento delle entità definite è quello di una sessione
nuova: ha i tipi di linea `ByBlock`, `ByLayer`, `Continuous` e il layer `0`.
Due definizioni lo cambiano:

```
T = LTYPE name="Tratteggio" pattern=5,-2,0,-2 description="mio"
I = LTYPE name="ACAD_ISO02W100"
L = LAYER name="Muri" color=1 linetype="Tratteggio" lineweight=0.5 off frozen locked noplot
Z = LAYER name="0" color=3
A = LINE start=0,0 end=10,0 layer="Muri" color=bylayer linetype="ByLayer"
!current layer="Muri" color=2
```

| Definizione | Proprietà |
|---|---|
| `LAYER` | `name=` tra virgolette, obbligatorio; `color=` un indice o `r,g,b` (mai `bylayer` né `byblock`); `linetype=` un tipo definito sopra, mai `ByLayer` né `ByBlock`; `lineweight=` millimetri o `default` (mai `bylayer` né `byblock`); gli stati `off`, `frozen`, `locked`, `noplot`, parole senza valore. Ciò che manca vale come nel layer `0` di un disegno nuovo: colore 7, `Continuous`, `default`, acceso, scongelato, sbloccato, stampabile |
| `LTYPE` | `name=` e `pattern=` obbligatori: il motivo è come nel rapporto, numeri separati da `,` o `continuous`; `description=` tra virgolette, vuota se manca. `ACAD_ISO02W100` e `ACAD_ISO10W100` (ADR 0025, punto 2) si definiscono anche con il solo nome: motivo e descrizione sono quelli standard |

- Le proprietà si scrivono in qualunque ordine, ognuna una volta.
- I nomi seguono le regole dell'ADR 0023, punto 1 (non vuoti, al massimo 255
  caratteri, senza `< > / \ " : ; ? * | , =` `` ` `` e senza spazi ai capi)
  e si confrontano senza distinguere le maiuscole. Un nome già in tabella è
  un errore del file, con un'eccezione: il layer `0`, che esiste già, si può
  ridefinire **una volta** con `name="0"` per cambiarne colore, tipo di
  linea, spessore e stati.
- Nelle entità, un layer o un tipo di linea non definito sopra è un errore
  del file.
- **`!current`** cambia i valori correnti del documento: le proprietà
  scritte, in qualunque ordine e una volta ciascuna, tra `layer`, `color`,
  `linetype`, `ltscale` e `lineweight` (almeno una); le altre restano
  com'erano. Non cambia le entità definite né quelle che seguono, che hanno
  le proprietà scritte o i valori predefiniti: serve a `effective` con
  `ByLayer` e alle domande sul documento.

**Domande.**

| Domanda | Risposta | Significato |
|---|---|---|
| `aci <n>` | `r,g,b` | il colore dell'indice `n`, da 1 a 255, nella tabella dell'ADR 0024, punto 3 |
| `effective <E>` | `color=<r,g,b> linetype="<nome>" ltscale=<n> lineweight=<mm> visible=yes\|no locked=yes\|no` | le proprietà con cui l'entità si disegna (sotto) |
| `dashes <E>` | elenco di pezzi separati da `;`, o `continuous` | i tratti dell'entità secondo l'ADR 0025, punto 4: `LINE x1,y1 x2,y2`, `ARC center=… radius=… start-angle=… end-angle=…`, `POINT x,y` |
| `quickselect [type=<TIPO>] [<proprietà><op><valore>] [within=<nomi>] [exclude]` | elenco di nomi separati da spazi, o `none` | le entità che soddisfano il filtro (sotto) |
| `layers D` | elenco di righe separate da `;`, o `none` | i layer del disegno `D` letto da un file, una riga come in [`layers:`](rapporto.md#layer-tipi-di-linea-e-valori-correnti); `none` per un disegno senza tabella dei layer |
| `linetypes D` | elenco di righe separate da `;`, o `none` | i tipi di linea del disegno `D`, una riga come in [`linetypes:`](rapporto.md#layer-tipi-di-linea-e-valori-correnti), senza `ByBlock`, `ByLayer` e `Continuous` |

- **`effective`** risolve le proprietà dell'entità: `ByLayer` prende il
  valore del layer dell'entità; `ByBlock` vale come colore 7, tipo
  `Continuous` e spessore `default` finché non ci sono blocchi (ADR 0024,
  Conseguenze). Il colore è sempre `r,g,b`, dalla tabella degli indici;
  `linetype` è il nome del tipo risolto; `ltscale` è `LTSCALE` per la scala
  dell'entità; `lineweight` è in millimetri (`0.25`), con `default` risolto
  da `LWDEFAULT` (0.25 in un documento nuovo); `visible=no` se il layer è
  spento o congelato, `locked=yes` se è bloccato.
- **`dashes`** dà i pezzi in coordinate del disegno, con il motivo
  moltiplicato per `LTSCALE` e per la scala dell'entità; `continuous` per
  un'entità che si disegna continua (tipo senza tratti, o più corta del
  motivo). Un testo non ha tipo di linea: `continuous`.
- **`quickselect`**: `type=` un tipo DXF (`LINE`); una condizione su
  `layer`, `color`, `linetype` o `lineweight` con `<op>` `=` o `!=` e il
  valore scritto come nelle definizioni (`layer="Muri"`, `color!=bylayer`),
  confrontato con la proprietà **scritta** dell'entità, non con quella
  effettiva (`color=bylayer` trova le entità con `ByLayer`, come in
  AutoCAD), con i nomi senza distinguere le maiuscole; un valore che non si
  legge (colore o spessore non validi) non trova nulla e non è un errore;
  `within=` limita la ricerca alle entità nominate, separate da `,`;
  `exclude` dà le entità che **non** soddisfano il filtro. Senza filtro le
  dà tutte. Le entità su layer spenti, congelati o bloccati non sono mai
  scelte, come dai gesti di selezione.
- `layers` e `linetypes` chiedono di un disegno letto da un file come le
  domande della sezione [File](#file); `entity D <handle>` scrive anche le
  proprietà dell'entità, come il rapporto. Finché `io` non legge le tabelle
  (pacchetto 0.6-P10), un disegno letto non ha layer e le sue entità hanno
  i valori predefiniti.

**Gli stati dei layer nelle domande sul documento.** `pick`, `window`,
`crossing` e `snap` seguono la tabella dell'ADR 0023, punto 4:

| Stato del layer dell'entità | `pick`, `window`, `crossing` | `snap` |
|---|---|---|
| acceso | trovata | sì |
| spento | mai trovata | no |
| congelato | mai trovata | no |
| bloccato | trovata: le domande chiedono del modello, non della selezione di un comando | sì |

Le domande su un oggetto (`bbox`, `length`, `snaps`…) rispondono per ogni
entità, qualunque sia lo stato del suo layer.

Chi realizza le domande: `layers`, `linetypes` e le proprietà di `entity` il
pacchetto 0.6-P5; `aci`, `effective` e gli stati nelle domande sul
documento il pacchetto 0.6-P9; `dashes` il pacchetto 0.6-P11; `quickselect`
il pacchetto 0.6-P23. Fino ad allora rispondono
`error: question not implemented yet`.

## Errori

Un file che non rispetta questa specifica non è un caso valido: definizione o
domanda scritta male, nome già definito o non definito sopra, domanda
sconosciuta, argomenti mancanti o in più. `tecnigrafo-script` indica la riga e
il motivo ed esce con il codice 2 ([casi.md](casi.md)).

Non sono errori, ma casi non superati, tre situazioni previste da questo
catalogo e non ancora realizzate dal programma:

- una **domanda** del catalogo non ancora realizzata risponde
  `error: question not implemented yet`;
- una **definizione** di un tipo del [rapporto](rapporto.md#tappa-04) non
  ancora realizzato (`CIRCLE` prima del pacchetto P6) ferma il file con
  "not implemented yet"; dalla tappa 0.8, una definizione di un tipo il cui
  pacchetto non è ancora unito lo ferma con `<TIPO> is not supported yet.`
  ([rapporto.md](rapporto.md#tappa-08-entità-composte));
- un **modo di snap** (`intersection`, `perpendicular`, `tangent`, `quadrant`)
  o un **argomento con nome** (`from=` della domanda `snap`) descritti da questo
  catalogo dalla tappa 0.7, fino al pacchetto P7, fermano il file con
  "not implemented yet".

In tutti questi casi il caso non è superato: se è elencato in
`tests/verifica/in-attesa/` aspetta il suo lavoro senza bloccare la CI. Un
modo o un argomento che il catalogo non descrive (`modes=corner`,
`aperure=1`) resta un errore del file.

## Valori attesi

Le risposte attese si ricavano da calcoli, da file pubblici o da programmi di
riferimento, **mai** eseguendo Tecnigrafo (ADR 0011, punto 1). Le misure del
font si leggono dal file del font con uno strumento indipendente, per esempio
fontTools. `tecnigrafo-script check` scrive accanto al risultato il file
`ottenuto.tgq`, con le risposte del programma al posto di quelle attese: serve
a chi lavora per vedere le differenze, mai a scrivere i riferimenti.
