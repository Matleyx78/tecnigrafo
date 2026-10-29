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
| vuota, o che inizia con `;` | ignorata |
| `!tolerance <tipo> <valore>` | cambia una tolleranza, come negli [script](script.md#dettagli), **per le domande che seguono** |
| `!current <proprietà>` | cambia i valori correnti del documento delle definizioni (dalla tappa 0.6, [Layer e proprietà](#layer-e-proprietà)) |
| `<Nome> = <entità>` | una **definizione** |
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
fino ad allora i casi che la usano stanno in `in-attesa.txt`. Le entità delle
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
| `side <E> x,y` | `left`, `right` o `on` | da che parte della retta o del cerchio della curva sta il punto, guardando nel verso della curva (linea dall'inizio alla fine, arco e cerchio in senso antiorario: a sinistra l'interno); per una polilinea, del segmento più vicino al punto, a pari distanza il primo |
| `coincident x1,y1 x2,y2 [units=<n>]` | `yes` / `no` | i due punti coincidono per la regola delle tolleranze del disegno |
| `degenerate <E> [segment=<i>] [units=<n>]` | `yes` / `no` | la curva di una linea, di un arco o di un cerchio, o il segmento `i` di una polilinea, è degenere per la regola (ogni due suoi punti distano al più la tolleranza) |

**Curva parallela** (P17, richiesta [Matleyx78/solid-space#22](https://github.com/Matleyx78/solid-space/issues/22),
[ADR 0035](../adr/0035-taglio-offset-raccordi-unione.md), punti 10–12)

| Domanda | Risposta | Significato |
|---|---|---|
| `offset <E> <d> x,y [units=<n>]` | un'entità, oppure `vanished`, `failed` o `no-side` | la curva a distanza `d` (> 0) dal lato del punto `x,y`; `vanished` quando non resta nulla, `failed` quando la copia di una polilinea non si può fare senza autointersezioni, `no-side` quando il punto sta esattamente sulla retta o sul cerchio |

**Raccordi e tangenze** (P19, richiesta [Matleyx78/solid-space#21](https://github.com/Matleyx78/solid-space/issues/21),
ADR 0035, punti 13 e 14)

Gli esiti che non costruiscono nulla: `none` (non esiste), `infinite` (le curve
stanno sulla stessa retta o sullo stesso cerchio, o simili), `ambiguous` (un
punto sta esattamente sulla retta o sul cerchio dell'altra curva),
`not-allowed` (un'entità che il comando non prende).

| Domanda | Risposta | Significato |
|---|---|---|
| `fillet <A> x1,y1 <B> x2,y2 radius=<r> [trim=no] [units=<n>]` | elenco separato da `;` di `<nome> <entità>`, o un esito | il raccordo tra `A`, scelta in `x1,y1`, e `B`, scelta in `x2,y2`: le due entità come diventano (`A …`, `B …`) e l'arco nuovo (`new ARC …`; nessuno con raggio 0) |
| `fillet <P> x1,y1 x2,y2 radius=<r> [units=<n>]` | `P <entità>`, o un esito | il raccordo tra i due segmenti della polilinea `P` più vicini ai due punti |
| `fillet <P> polyline radius=<r> [units=<n>]` | `<entità> done=<n> short=<m>` | la polilinea `P` con tutti i vertici raccordati (opzione *Polyline*), quanti vertici raccordati e quanti con segmenti troppo corti |
| `tangents x,y <E>` | elenco di punti `x,y` separati da `;`, o `none` | i punti di tangenza dal punto a un cerchio, un arco o un arco di polilinea, solo quelli sulla curva; `none` per un punto dentro il cerchio o una curva dritta; il punto stesso se sta esattamente sul cerchio |
| `circle-ttr <A> x1,y1 <B> x2,y2 radius=<r>` | `CIRCLE center=… radius=…`, o un esito | il cerchio di raggio `r` tangente ad `A` e `B`, quello con i punti di tangenza più vicini ai due punti, come `CIRCLE` *Ttr* |

**Smussi** (P21, ADR 0035, punto 15)

| Domanda | Risposta | Significato |
|---|---|---|
| `chamfer <A> x1,y1 <B> x2,y2 distances=<d1>,<d2> [trim=no]` | come `fillet`, con `new LINE …` | lo smusso con le due distanze, la prima su `A` |
| `chamfer <A> x1,y1 <B> x2,y2 angle=<l>,<gradi> [trim=no]` | come `fillet`, con `new LINE …` | lo smusso con la lunghezza su `A` e l'angolo da `A` |
| `chamfer <P> polyline distances=<d1>,<d2>` | `<entità> done=<n> short=<m>` | la polilinea `P` con tutti i vertici tra segmenti dritti smussati |

**Unione** (P22, richiesta [Matleyx78/solid-space#23](https://github.com/Matleyx78/solid-space/issues/23),
ADR 0035, punti 17–20)

| Domanda | Risposta | Significato |
|---|---|---|
| `join <A> <B> … [units=<n>]` | elenco separato da `;` di `<nome> <entità>`, o `none` | il risultato di `JOIN` interattivo sulle entità nominate, con `A` come sorgente e le altre nell'ordine scritto: le entità che restano come diventano (`A …`), quella nuova se il tipo cambia (`new LWPOLYLINE …`); `none` se non si unisce nulla |

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
  "not implemented yet";
- un **modo di snap** (`intersection`, `perpendicular`, `tangent`, `quadrant`)
  o un **argomento con nome** (`from=` della domanda `snap`) descritti da questo
  catalogo dalla tappa 0.7, fino al pacchetto P7, fermano il file con
  "not implemented yet".

In tutti questi casi il caso non è superato: se è elencato in
`tests/verifica/in-attesa.txt` aspetta il suo lavoro senza bloccare la CI. Un
modo o un argomento che il catalogo non descrive (`modes=corner`,
`aperure=1`) resta un errore del file.

## Valori attesi

Le risposte attese si ricavano da calcoli, da file pubblici o da programmi di
riferimento, **mai** eseguendo Tecnigrafo (ADR 0011, punto 1). Le misure del
font si leggono dal file del font con uno strumento indipendente, per esempio
fontTools. `tecnigrafo-script check` scrive accanto al risultato il file
`ottenuto.tgq`, con le risposte del programma al posto di quelle attese: serve
a chi lavora per vedere le differenze, mai a scrivere i riferimenti.
