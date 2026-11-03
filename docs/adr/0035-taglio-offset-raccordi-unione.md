# ADR 0035: Taglio, prolungamento, offset, raccordi, smussi e unione

- **Stato:** Accettato
- **Data:** 2026-10-29
- **Corretto:** 2026-11-01 — su decisione del responsabile, dopo due decisioni
  di solid-space prese realizzando o rivedendo il contratto; nessun codice
  unito attua i punti corretti (eccezione di [GOVERNANCE.md](../../GOVERNANCE.md)):
  P14 (#460, unito lo stesso giorno) attua i punti 1, 2 e 4, che non cambiano.
  (1) La diagnosi `outOfDomain` di `fillet` e `tangentCircleNear` per una
  costruzione oltre il dominio (`tangency.hpp`; ADR 0011 di solid-space, righe
  **Corretto:**; [Matleyx78/solid-space#66](https://github.com/Matleyx78/solid-space/issues/66),
  [#70](https://github.com/Matleyx78/solid-space/issues/70),
  [#122](https://github.com/Matleyx78/solid-space/pull/122),
  [#126](https://github.com/Matleyx78/solid-space/pull/126)): il raccordo, il
  cerchio o lo spigolo esistono ma non si rappresentano, e `FILLET`, `CIRCLE`
  *Ttr* e `CHAMFER` (lo spigolo che `intersect` omette oltre il dominio) lo
  dicono con un messaggio proprio, distinto da "non esiste" e da "parallele",
  e nessuna entità cambia; un raggio scritto dall'utente oltre 2^50 resta un
  dato fuori dal dominio (punto 23) (punti 13–15). (2) Una curva parallela può
  mancare anche nei casi semplici di una polilinea, quando solid-space non la
  rappresenta (`offset.hpp`;
  [Matleyx78/solid-space#127](https://github.com/Matleyx78/solid-space/issues/127))
  (punto 12).
- **Corretto:** 2026-11-01 — su decisione del responsabile sui buchi della
  revisione delle interfacce delle schede
  ([#464](https://github.com/Matleyx78/tecnigrafo/issues/464), B1–B18, e
  [#359](https://github.com/Matleyx78/tecnigrafo/issues/359)); nessun codice
  unito attua i punti corretti (eccezione di [GOVERNANCE.md](../../GOVERNANCE.md)).
  (1) Con una risposta incompleta di solid-space (dati rifiutati) e nessun
  taglio, `TRIM` ed `EXTEND` non riescono in tutti e due i modi, e un oggetto
  non si cancella (punti 5 e 6). (2) *Trim No* vale solo fra entità distinte:
  sui segmenti di una stessa polilinea e con *Polyline* la polilinea cambia
  sempre (punti 13 e 15), da confermare con una prova in AutoCAD prima del
  pacchetto di `FILLET` (smentita dalla prova, rivista il 2026-11-02: riga
  seguente). (3) Con il taglio attivo, una parte tenuta che si
  riduce a nulla cancella la sua entità (punti 13 e 15). (4) In `JOIN` una
  linea o un arco nascono solo se tutte le entità scelte sono linee allineate
  alla sorgente o archi del suo cerchio, altrimenti vale la catena; la catena
  si ferma quando si chiude (punti 17 e 18); la prima regola è da confermare
  con una prova in AutoCAD prima del pacchetto dell'unione. Le altre decisioni
  precisano le schede senza cambiare questo ADR.
- **Corretto:** 2026-11-02 — su decisione del responsabile, dopo le prove a
  mano in AutoCAD LT dello stesso giorno
  ([#467](https://github.com/Matleyx78/tecnigrafo/issues/467),
  [dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md), punti 4 e 5;
  [#468](https://github.com/Matleyx78/tecnigrafo/issues/468)), che hanno
  smentito due decisioni del 2026-11-01 (B6 e B11 della #464); il responsabile
  sceglie la strada di AutoCAD per tutte e due. Nessun codice unito attua i
  punti corretti (eccezione di [GOVERNANCE.md](../../GOVERNANCE.md)). (1) *Trim
  No* vale anche per le polilinee, come fra entità distinte: sui segmenti di
  una stessa polilinea e con *Polyline* la polilinea resta com'è e il raccordo
  o lo smusso nasce come entità a sé (punti 13 e 15); sostituisce il punto (2)
  della riga del 2026-11-01. (2) Gli archi dello stesso cerchio si coprono
  come in AutoCAD, in senso antiorario a partire dall'inizio della sorgente;
  se arrivano al giro intero formano un cerchio, e `JOIN` interattivo chiede se
  convertirlo (punti 17 e 18). La regola vecchia, il cerchio meno il vuoto più
  grande, decade. La regola nuova è confermata dalla prova 7 di
  [dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md), fatta il 2026-11-02 sui due
  casi che la distinguono dalla vecchia.
- **Corretto:** 2026-11-02 — su decisione del responsabile, chiusa la prova 3
  di [dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md): il punto 22 non chiede più
  di controllare i testi su un AutoCAD inglese; valgono quelli della tabella di
  quel file. Nessun codice unito attua il punto (eccezione di
  [GOVERNANCE.md](../../GOVERNANCE.md)).
- **Corretto:** 2026-11-02 — su decisione del responsabile sui buchi trovati
  da P22 ([#373](https://github.com/Matleyx78/tecnigrafo/issues/373)); nessun
  codice unito attua il punto corretto (eccezione di
  [GOVERNANCE.md](../../GOVERNANCE.md)). (1) Una polilinea chiusa diversa
  dalla sorgente non ha estremi liberi e non entra nella catena: si scarta.
  (2) Gli archi dello stesso cerchio formano un cerchio anche quando l'arco
  calcolato avrebbe l'angolo finale uguale a quello iniziale, purché almeno un
  altro arco si unisca: la sola sorgente non cambia (punto 17).
  L'ordine degli estremi delle linee allineate viene da solid-space, come dice
  già il punto 1: la funzione la aggiunge al contratto una issue `contratto`
  di solid-space. Le altre decisioni precisano la scheda P22 senza cambiare
  questo ADR.
- **Corretto:** 2026-11-03 — operazione snellimento
  ([#478](https://github.com/Matleyx78/tecnigrafo/issues/478);
  [DECISIONI.md](../DECISIONI.md), stessa data): dove questo ADR descriveva il
  contratto di solid-space (il contesto, la prima riga **Corretto:** del
  2026-11-01 e i punti 1, 5, 10–15 e 23), rimanda alle sue intestazioni e ai
  suoi ADR, 0013 compreso. Nessuna regola di Tecnigrafo cambia; nel punto 1,
  che il codice unito attua (P14), il rimando sostituisce solo la
  descrizione. Dove la descrizione era già diversa dal contratto vale il
  contratto: nel punto 10, il lato di un arco orario (un segmento di
  polilinea con bulge negativo), il cui interno è a destra.

## Contesto

`TRIM`, `EXTEND`, `OFFSET`, `FILLET`, `CHAMFER` e `JOIN` sono i comandi più
difficili della tappa 0.7. La loro geometria la decide solid-space (ADR 0030),
che ha accettato le quattro richieste della tappa con l'esito 3 (serve un pezzo
suo) e il loro contratto, nel nuovo componente `geom2d` sopra `robust`:

| Richiesta | Che cosa | ADR di solid-space | Intestazioni |
|---|---|---|---|
| [Matleyx78/solid-space#20](https://github.com/Matleyx78/solid-space/issues/20) | intersezioni e lato di un punto | [0010](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0010-posizione-e-intersezioni-nel-piano.md) | `curves.hpp`, `intersection.hpp`, `side.hpp` |
| [Matleyx78/solid-space#21](https://github.com/Matleyx78/solid-space/issues/21) | raccordi e tangenze | [0011](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0011-raccordi-e-tangenze-nel-piano.md) | `tangency.hpp` |
| [Matleyx78/solid-space#22](https://github.com/Matleyx78/solid-space/issues/22) | curva parallela | [0012](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0012-curva-parallela-nel-piano.md) | `offset.hpp` |
| [Matleyx78/solid-space#23](https://github.com/Matleyx78/solid-space/issues/23) | regola delle tolleranze | [0009](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0009-regola-delle-tolleranze-del-piano.md) | `tolerance.hpp`, `coincidence.hpp` |

Il componente è descritto dall'[ADR 0008 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0008-componente-geom2d.md).
Che cosa decide esattamente e che cosa costruisce entro un errore lo dicono
`curves.hpp`, in testa, e l'[ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md);
la regola delle tolleranze si usa solo con le funzioni che la nominano (ADR
0009 di solid-space). Le intestazioni citate qui stanno in
`geom2d/include/solid_space/geom2d/` di solid-space (`tolerance.hpp` in
`robust/include/solid_space/robust/`) e valgono nella versione fissata dal
sottomodulo (ADR 0030, punto 9).

Restano a Tecnigrafo il comportamento dei comandi (domande, opzioni, che cosa si
tiene e che cosa si toglie, messaggi), la traduzione dalle entità alle curve, la
proiezione di un punto su una curva (ADR 0010 di solid-space, punto 9), lo
smusso e che cosa si garantisce a chi usa i comandi. La scheda della tappa chiede
di scriverlo, perché `OFFSET` e `TRIM` su polilinee complesse sono notoriamente
difficili.

I DXF di prova preparati in AutoCAD LT
([tappe/0.7/dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md)) fissano il
comportamento di AutoCAD in una trentina di casi. Mostrano anche che negli script
alcuni di questi comandi usano sequenze vecchie (ADR 0032).

## Decisione

### La facciata di `core`

1. **Una facciata, che traduce soltanto.** `core` usa `solid_space::geom2d`
   attraverso le sue funzioni (intersezioni, curva parallela, raccordi, unione),
   che parlano con i tipi di Tecnigrafo; nessun'altra parte di Tecnigrafo
   include le intestazioni di solid-space.
   - **Dalle entità alle curve:** una `LINE` è un `Segment`; un `CIRCLE` è un
     `Circle`; un `ARC` è un `Arc` con lo stesso centro, raggio e angolo
     iniziale e con ampiezza `endAngle − startAngle`, a cui si aggiunge
     `kFullTurn` se non è positiva, calcolata in `double` in quest'ordine; una
     `LWPOLYLINE` è un `Polyline` con le posizioni e i bulge dei suoi vertici,
     divisa in pezzi da `pieces`. Un segmento che `PolylineSegment::isArc()` di
     Tecnigrafo considera dritto passa con bulge 0, così solid-space decide
     sulla stessa curva che Tecnigrafo disegna (ADR 0036, punto 7). Il testo
     non ha curva.
   - **Dalle risposte alle entità:** l'ordine dei punti lungo una curva, la
     loro posizione e quali punti sono lo stesso punto vengono da solid-space
     (`intersection.hpp`, in testa, e `Location`). La facciata non riordina e
     non confronta mai da sé le stazioni.
   - **Le polilinee** si compongono pezzo per pezzo, nell'ordine dei segmenti.
     Un punto alla fine di un segmento e all'inizio del successivo (per una
     polilinea chiusa, anche alla fine dell'ultimo e all'inizio del primo) è il
     loro vertice comune, deciso dalla posizione esatta: compare una volta sola,
     come inizio del segmento che segue (per il vertice 0 di una polilinea
     chiusa, come inizio del segmento 0).
   - **Gli estremi nuovi** delle entità che un comando taglia, prolunga o
     raccorda sono i punti che solid-space restituisce, senza ricalcolarli; gli
     angoli nuovi degli archi si ricavano da quei punti con
     `solid_space::robust::atan2`, come gli angoli degli archi di raccordo di
     solid-space, e si riportano in [0, 2π).
2. **La regola delle tolleranze ha l'unità del disegno.** La facciata dà a
   solid-space la lunghezza in metri dell'unità del disegno, ricavata da
   `INSUNITS` (ADR 0036, punto 1). Le funzioni che usano la regola (pezzi
   degeneri, coincidenze di `JOIN`, controllo della curva parallela) la
   ricevono costruita per l'unità del documento.

### `TRIM` ed `EXTEND`

3. **Due modi, come AutoCAD**, scelti con `TRIMEXTENDMODE` (valore della
   sessione, 1 all'avvio) o con l'opzione *mOde* dentro il comando:
   - **rapido** (1): tutte le curve visibili fanno da bordo; non si scelgono
     bordi;
   - **standard** (0): prima si scelgono i bordi (`Select objects or <select
     all>:`, e Invio li sceglie tutti), poi gli oggetti da tagliare o prolungare.

   Negli script vale sempre il modo standard (ADR 0032, punto 8).
4. **Bordi.** Fanno da bordo linee, archi, cerchi e polilinee sui layer visibili,
   anche bloccati; il testo no.
   - **Una polilinea fa da bordo anche a se stessa, e i suoi vertici delimitano
     i pezzi**: AutoCAD taglia un rettangolo da un vertice all'altro anche se
     una linea lo attraversa più lontano (`autocad-taglio.dxf`, casi d ed e).
   - Un bordo **tangente** all'oggetto lo taglia nel punto di tangenza, come un
     bordo che lo attraversa (caso g).
   - Un bordo che ha un **tratto in comune** con l'oggetto (stessa retta o stesso
     cerchio, sovrapposti) non lo taglia: l'ordine degli estremi del tratto
     rispetto agli altri tagli non è una decisione di solid-space, e la
     facciata non lo ricava confrontando stazioni (punto 1).
5. **`TRIM`** toglie il pezzo dell'oggetto scelto compreso tra i due tagli più
   vicini al punto del clic.
   - I tagli lungo l'oggetto vengono da `intersectAlong` di solid-space, nel
     suo ordine e riuniti come dice `HostIntersections`.
   - Il punto del clic si porta sull'oggetto con la proiezione di Tecnigrafo e
     si confronta con le posizioni dei tagli: è la scelta dell'utente, non una
     decisione geometrica. Se cade esattamente su un taglio, si toglie il pezzo
     che segue.
   - Un cerchio tagliato diventa un arco con lo stesso handle (ADR 0031,
     punto 5); un arco o una linea tagliati in mezzo diventano due entità, e il
     primo pezzo tiene l'handle; una polilinea chiusa tagliata diventa aperta.
   - Un pezzo che resta e che la regola delle tolleranze trova degenere (ADR
     0036, punto 5) non si tiene: se nulla resta, l'oggetto si cancella.
   - Nel modo rapido un oggetto senza tagli si cancella; nel modo standard
     compare il messaggio di AutoCAD che non interseca il bordo, e l'oggetto
     resta. Se però solid-space ha rifiutato dei dati (punto 23) e non c'è
     nessun taglio, l'operazione non riesce nei due modi e l'oggetto resta
     (corretto il 2026-11-01).
   - Il testo non si taglia.
6. **`EXTEND`** prolunga l'estremo più vicino al clic fino al primo bordo che
   incontra: le linee lungo la retta, gli archi lungo il loro cerchio, le
   polilinee aperte con il loro primo o ultimo segmento. Il primo bordo è il
   primo taglio che solid-space dà sul prolungamento, nel suo ordine. Cerchi,
   polilinee chiuse e testo non si prolungano. Senza un bordo da raggiungere,
   l'entità resta com'è; se solid-space ha rifiutato dei dati (punto 23),
   l'operazione non riesce e lo dice (corretto il 2026-11-01).
7. **`Shift`.** Con `Shift` premuto il clic fa l'altro comando: in `TRIM`
   prolunga, in `EXTEND` taglia.
8. **Opzioni:** *mOde* e *Undo*. Restano fuori *cuTting edges*/*Boundary edges*,
   *Crossing*, *Fence*, *Project*, *Edge* ed *eRase*, che quindi non compaiono
   nelle domande. Un clic nel vuoto non apre un rettangolo (ADR 0032, punto 1).

### `OFFSET`

9. **Domande e opzioni come AutoCAD:** distanza o *Through*, poi *Erase* e
   *Layer* (*Source* o *Current*); nel ciclo *Exit*, *Multiple* e *Undo*. La
   copia è un'entità nuova (ADR 0031, punto 6); con *Layer Current* prende il
   layer corrente.
10. **Il lato.** Il punto indicato dà il segno della distanza con `side()` di
    solid-space, con i lati di `side.hpp` e il segno di `offset.hpp` (in
    testa). Per una
    polilinea vale il segmento più vicino al punto, con la proiezione di
    Tecnigrafo (a pari distanza il primo). Un punto esattamente sulla retta o
    sul cerchio di quel segmento non indica un lato: nessuna copia, con il
    messaggio di AutoCAD. Con *Through* la distanza è quella del punto dalla
    curva, misurata da Tecnigrafo, e il lato è il suo.
11. **Giunzioni**: i passi di `offsetPolyline` (`offset.hpp`; ADR 0012 di
    solid-space, punto 4), che corrispondono ad AutoCAD con `OFFSETGAPTYPE` = 0
    (`autocad-offset.dxf`). Ogni segmento della copia tiene le larghezze del segmento da cui viene; un
    arco di giunzione ha larghezza 0, salvo la larghezza costante della
    polilinea (gruppo 43), che resta.
12. **Che cosa si garantisce** è ciò che garantiscono `offsetCurve` e
    `offsetPolyline` (`offset.hpp`, «What is guaranteed»; ADR 0012 di
    solid-space, punto 6): Tecnigrafo non promette di più. Quando solid-space
    non dà una copia (una curva che si annulla, una copia che non può fare o
    non rappresenta), il comando dà il messaggio di AutoCAD e nessuna copia:
    mai una copia sbagliata. Il risultato può differire da quello di AutoCAD:
    nel caso della U troppo stretta (`autocad-offset.dxf`, caso f) anche
    AutoCAD non crea nulla; nel caso g con distanza 5 solid-space non ha potuto
    confrontare i suoi passi con AutoCAD, e se differiscono vale la regola dei
    passi (lo segnala il pacchetto di riferimenti, e decide il responsabile).

### `FILLET` e `CHAMFER`

13. **`FILLET`.** Opzioni *Undo*, *Polyline*, *Radius*, *Trim* e *Multiple*,
    con `Shift` che vale raggio 0. Il comando lavora su linee, archi, cerchi e
    segmenti della stessa polilinea; il raccordo viene da `fillet` di
    solid-space, con le sue regole (`tangency.hpp`, regole 1–4), che seguono
    `autocad-raccordi.dxf`. Restano a Tecnigrafo:
    - le linee e gli archi si accorciano o si allungano fino ai punti di
      tangenza, con le posizioni che solid-space dà; i cerchi non si tagliano;
    - con *Trim*, due segmenti della stessa polilinea ricevono un segmento
      curvo in mezzo, con il bulge che dà solid-space; *Polyline* raccorda
      tutti i vertici, segmento per segmento, e i segmenti troppo corti restano
      come sono e si contano nel messaggio;
    - con *Trim No* le entità non cambiano e si aggiunge solo l'arco. Vale
      anche per i segmenti di una stessa polilinea e con *Polyline*, come in
      AutoCAD (`autocad-prove-a-mano.dxf`, punto 4): la polilinea resta com'è
      e ogni arco di raccordo nasce come entità a sé, con le proprietà della
      polilinea (punto 16) (corretto il 2026-11-02);
    - con il taglio attivo, un'entità la cui parte tenuta si riduce a nulla,
      o è degenere per la regola delle tolleranze (ADR 0036, punto 5), si
      cancella (corretto il 2026-11-01);
    - un arco di raccordo che la regola delle tolleranze trova degenere (ADR
      0036, punto 5) non si aggiunge: il comando fa lo spigolo, come con
      raggio 0.

    Le risposte "non esiste", "infiniti raccordi" e "il punto non indica un
    lato" di solid-space diventano messaggi; nessuna entità cambia. Lo stesso
    per "oltre il dominio", la diagnosi `outOfDomain` sul raggio per una
    costruzione oltre il dominio (`tangency.hpp`, `fillet`): il raccordo
    esiste ma non si rappresenta, quindi il messaggio è suo, distinto da "non
    esiste"; con *Polyline* il vertice resta com'è e si conta tra i troppo
    corti. Un raggio
    scritto oltre 2^50 è invece un dato fuori dal dominio (punto 23): la
    facciata lo riconosce prima di chiedere il raccordo a solid-space
    (corretto il 2026-11-01). Fuori: il raccordo tra una polilinea e
    un'altra entità.
14. **`CIRCLE` tangente-tangente-raggio** usa `tangentCircleNear` di
    solid-space, con la sua scelta tra i cerchi tangenti (`tangency.hpp`). Per
    una polilinea vale il segmento più vicino al punto. Con la diagnosi
    `outOfDomain` sul raggio il cerchio non si disegna, con il messaggio del
    punto 13 per "oltre il dominio" (corretto il 2026-11-01).
15. **`CHAMFER`.** Opzioni *Undo*, *Polyline*, *Distance*, *Angle*, *Trim*,
    *mEthod* e *Multiple*. Lavora su linee e segmenti dritti di polilinee.
    - Lo spigolo è il punto comune delle due rette prolungate, dato da
      `intersect` di solid-space (`intersection.hpp`, `PairIntersections`).
      Senza punto comune (rette parallele o coincidenti) il comando dà il
      messaggio di AutoCAD; con lo spigolo omesso oltre il dominio
      (`omittedBeyondDomain`), il messaggio del punto 13 per "oltre il
      dominio", come il raccordo di raggio 0 (corretto il 2026-11-01).
    - Lo smusso lo costruisce Tecnigrafo (ADR 0011 di solid-space, punto 11): i
      suoi estremi stanno alle distanze date dallo spigolo, ciascuno sulla
      parte della sua linea che il punto indica, cioè dalla parte dello spigolo
      dove cade la proiezione del punto. Una distanza più lunga della parte da
      tenere dà il messaggio di AutoCAD.
    - *Trim No*, anche sui segmenti di una polilinea e con *Polyline* (la
      polilinea resta com'è e ogni smusso è una linea a sé), e una parte tenuta
      che si riduce a nulla, come al punto 13 (corretto il 2026-11-01 e il
      2026-11-02).
16. **Proprietà dell'arco di raccordo e della linea di smusso:** quelle della
    prima entità scelta se le due hanno le stesse proprietà, altrimenti quelle
    correnti. I file di prova hanno tutto sul layer 0 e non lo dicono: la regola
    va confermata con una prova in AutoCAD LT prima del pacchetto di `FILLET`.

### `JOIN`

17. **Interattivo, più oggetti insieme**, come AutoCAD dal 2012
    (`autocad-unione.dxf`):
    - linee allineate diventano una linea, anche con spazi tra loro;
    - archi dello stesso cerchio diventano un arco, anche con spazi: ogni arco
      si copre in senso antiorario a partire dall'inizio della sorgente, e
      l'arco che risulta va dall'inizio della sorgente fino al più lontano dei
      loro estremi, come in AutoCAD (`autocad-prove-a-mano.dxf`, punto 5). Se
      arriva al giro intero, gli archi formano un cerchio e il comando chiede
      `Arc segments combined form a circle. Convert to circle? [Yes/No]
      <Yes>:` (testo del punto 22): con *Yes* il
      risultato è il cerchio, con *No* niente si unisce e il disegno non
      cambia (corretto il 2026-11-02). Lo stesso vale quando l'arco calcolato
      avrebbe l'angolo finale uguale a quello iniziale, purché almeno un altro
      arco si unisca: senza, niente si unisce (corretto il 2026-11-02, #373 e
      #476);
    - una linea o un arco nascono solo se tutte le entità scelte sono linee
      allineate alla sorgente o archi del suo cerchio; altrimenti vale la
      catena (punto 18), e le entità che non vi entrano si scartano (corretto
      il 2026-11-01);
    - linee, archi e polilinee che si toccano agli estremi diventano una
      polilinea; una polilinea chiusa diversa dalla sorgente non ha estremi
      liberi e si scarta (corretto il 2026-11-02, #373);
    - entità che non si toccano non si uniscono, e compare il messaggio
      `0 objects joined, <n> objects discarded from the operation.`

    "Allineate", "stesso cerchio" e "si toccano" sono la regola delle
    tolleranze del disegno (ADR 0036): `areAligned`, `onSameCircle` e
    `coincident` di solid-space. Due estremi che coincidono per la regola
    diventano un vertice, nel punto dell'entità che segue.
18. **Ordine fissato.** La regola non è transitiva (ADR 0009 di solid-space,
    punto 5), quindi `JOIN` la applica in un ordine scritto: la catena parte
    dalla sorgente e cresce prima dalla sua fine, poi dal suo inizio; a ogni
    passo si aggiunge la prima entità, nell'ordine in cui sono state scelte, che
    ha un estremo coincidente con l'estremo libero della catena; ci si ferma
    quando nessuna si aggiunge, o quando la catena si chiude (corretto il
    2026-11-01). Linee e archi si uniscono tra loro con la stessa
    regola, confrontando ciascuno con la sorgente. Gli archi di un solo
    cerchio non seguono la catena: li copre la regola del punto 17 (corretto
    il 2026-11-02).
19. **Negli script** vale la sequenza vecchia (ADR 0032): prima l'oggetto
    sorgente, poi gli oggetti da unire, accettati secondo il tipo della sorgente.
    Una linea accetta linee allineate, un arco archi dello stesso cerchio (uno per
    clic, con l'opzione *cLose*), una polilinea linee, archi e polilinee contigue.
    Gli archi si coprono come al punto 17; se coprono il giro intero senza
    *cLose*, anche questa sequenza chiede la conversione in cerchio, con la
    stessa domanda e le stesse risposte (prova 7 di
    [dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md), 2026-11-02); con *cLose*
    non chiede niente (corretto il 2026-11-02).
20. **Identità:** l'entità sorgente resta (ADR 0031, punto 7); con più oggetti
    insieme la sorgente è il primo scelto.

### Per tutti

21. **`Esc`** conserva le operazioni già completate e annulla solo quella in
    corso (ADR 0015).
22. **Testi dei messaggi.** I messaggi di questi comandi sono in inglese e
    seguono lo stile di AutoCAD, ma Tecnigrafo non deve ripeterne le parole
    una per una: i testi sono quelli della tabella "Testi inglesi dei
    messaggi" di [dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md), tradotti
    dall'italiano osservato nelle prove a mano o scritti nello stesso stile
    (corretto il 2026-11-02, su decisione del responsabile: non c'è un AutoCAD
    inglese su cui controllarli). Un testo nuovo si aggiunge a quella tabella
    nella scheda del pacchetto che lo introduce.
23. **Dati fuori dal dominio di solid-space** (`curves.hpp`, «Checking the
    data»; in breve, oltre 2^50, circa 1.1e15): nessun disegno reale ci arriva. Le curve che solid-space
    rifiuta non tagliano, non si raccordano e non si uniscono, e il comando lo
    dice con il messaggio dell'operazione non riuscita.

## Alternative considerate

- **Solo il modo rapido:** gli script `.scr` di AutoCAD con `TRIM` o `EXTEND`
  non funzionerebbero in Tecnigrafo (ADR 0011, punto 3).
- **La polilinea che non fa da bordo a se stessa:** più intuitivo, ma diverso da
  AutoCAD, e uno script darebbe un altro disegno.
- **Le intersezioni a coppie, con l'ordine dei tagli fatto dalla facciata:** la
  facciata ordinerebbe stazioni arrotondate, e due tagli esattamente nello stesso
  punto potrebbero risultare distinti o invertiti. Per questo i tagli vengono da
  `intersectAlong`, già ordinati e riuniti.
- **I tratti in comune come tagli:** servirebbe ordinare i loro estremi tra gli
  altri tagli confrontando stazioni (punto 4).
- **`OFFSET` che promette il risultato di AutoCAD in tutti i casi:** solid-space
  non lo promette (`offset.hpp`, «What is guaranteed»), e Tecnigrafo non
  promette di più.
- **Il lato di `OFFSET` deciso dentro solid-space con un punto:** per una
  polilinea dipende dalla proiezione sul segmento più vicino, che resta a
  Tecnigrafo (ADR 0012 di solid-space, alternative).
- **`FILLET` e `CHAMFER` anche su archi di polilinee e su entità miste con
  polilinee:** rimandati, perché AutoCAD stesso li tratta caso per caso.
- **Lo smusso chiesto a solid-space:** usa solo le intersezioni, e solid-space
  lo ha lasciato a Tecnigrafo (ADR 0011 di solid-space, alternative).
- **`JOIN` con la regola applicata a tutte le coppie insieme:** non essendo
  transitiva, il risultato dipenderebbe dall'ordine in cui il codice guarda le
  coppie, che nessuno ha scritto.

## Conseguenze

- I comandi stanno in `cmd`; la geometria nelle facciate di `core` sopra
  `solid_space::geom2d` e `solid_space::robust` (ADR 0030, punto 4):
  intersezioni e lato, parti delle entità, curva parallela, raccordi e
  tangenze, smusso, unione.
- Il criterio di chiusura su `OFFSET` si legge con il punto 12.
- I riferimenti dei comandi prendono i valori geometrici dall'archivio dei casi
  di solid-space, quando il caso c'è (ADR 0030, punto 14), e dal DXF di AutoCAD
  negli altri casi.
- La regola degli archi dello stesso cerchio (punto 17) è confermata dalla
  prova 7 di [dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md) (2026-11-02), anche
  per la domanda del cerchio nella sequenza degli script (punto 19).
- Le due domande aperte per la prova in AutoCAD LT, proprietà dell'arco di
  raccordo (punto 16) e testi inglesi dei messaggi (punto 22), sono chiuse
  (2026-11-02). Se la prova
  smentisce il punto 4 nel modo rapido, lo si corregge prima del pacchetto di
  `TRIM`.
- Le funzioni di solid-space si usano solo quando il loro pacchetto di
  solid-space è unito; i pacchetti di Tecnigrafo che le usano spostano il
  sottomodulo a quel commit (ADR 0030, punto 9).
