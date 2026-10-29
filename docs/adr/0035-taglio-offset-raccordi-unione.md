# ADR 0035: Taglio, prolungamento, offset, raccordi, smussi e unione

- **Stato:** Proposto
- **Data:** 2026-10-29

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
Ogni decisione di solid-space (quanti punti comuni, dove sta un punto lungo una
curva, in che ordine, da che parte) è esatta sui `double` ricevuti; ogni punto
costruito ha un errore dichiarato; la regola delle tolleranze dice che cosa
intendeva l'utente e si usa solo con le funzioni che la nominano.

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
     loro posizione (prima dell'inizio, all'inizio, dentro, alla fine, oltre la
     fine) e quali punti sono lo stesso punto vengono da solid-space. La
     facciata non riordina e non confronta mai da sé le stazioni, che sono
     arrotondate e servono solo come informazione.
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
   - I tagli lungo l'oggetto e il loro ordine vengono da solid-space
     (`intersectAlong`); tagli di bordi diversi nello stesso punto esatto sono
     un taglio solo.
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
     resta.
   - Il testo non si taglia.
6. **`EXTEND`** prolunga l'estremo più vicino al clic fino al primo bordo che
   incontra: le linee lungo la retta, gli archi lungo il loro cerchio, le
   polilinee aperte con il loro primo o ultimo segmento. Il primo bordo è il
   primo taglio che solid-space dà sul prolungamento, nel suo ordine. Cerchi,
   polilinee chiuse e testo non si prolungano. Senza un bordo da raggiungere,
   l'entità resta com'è.
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
    solid-space: a sinistra positiva, a destra negativa, guardando nel verso
    della curva (per un cerchio e per un arco la sinistra è l'interno). Per una
    polilinea vale il segmento più vicino al punto, con la proiezione di
    Tecnigrafo (a pari distanza il primo). Un punto esattamente sulla retta o
    sul cerchio di quel segmento non indica un lato: nessuna copia, con il
    messaggio di AutoCAD. Con *Through* la distanza è quella del punto dalla
    curva, misurata da Tecnigrafo, e il lato è il suo.
11. **Giunzioni**, come le definisce l'ADR 0012 di solid-space (`offsetPolyline`),
    che corrisponde ad AutoCAD con `OFFSETGAPTYPE` = 0 (`autocad-offset.dxf`):
    - ogni segmento si sposta; un segmento curvo il cui raggio diventa zero o
      negativo sparisce;
    - due segmenti vicini si uniscono nel loro punto comune più vicino al
      vertice; se i loro prolungamenti non si incontrano e i segmenti di
      partenza sono consecutivi, con un arco di raggio pari alla distanza
      centrato nel vertice; altrimenti la copia non riesce;
    - un segmento che le giunzioni girano al contrario si toglie, e i vicini si
      uniscono di nuovo;
    - i segmenti ridotti esattamente a un punto si tolgono.

    Ogni segmento della copia tiene le larghezze del segmento da cui viene; un
    arco di giunzione ha larghezza 0, salvo la larghezza costante della
    polilinea (gruppo 43), che resta.
12. **Che cosa si garantisce** (il punto 6 dell'ADR 0012 di solid-space):
    - linee, archi e cerchi: sempre. Un arco o un cerchio che si annullerebbe dà
      il messaggio di AutoCAD e nessuna copia;
    - polilinee, nei **casi semplici**: nessun segmento sparisce o si toglie, e
      la copia esatta resta più lontana della tolleranza della regola (ADR
      0036) da se stessa e dalla polilinea di partenza. La copia c'è sempre, è
      quella esatta entro l'errore dichiarato da solid-space e non si
      interseca;
    - **negli altri casi**: o una copia senza autointersezioni che non incontra
      la polilinea di partenza, o nessuna copia con il messaggio di AutoCAD. Mai
      una copia sbagliata. Il risultato può differire da quello di AutoCAD: nel
      caso della U troppo stretta (`autocad-offset.dxf`, caso f) anche AutoCAD
      non crea nulla; nel caso g con distanza 5 solid-space non ha potuto
      confrontare i suoi passi con AutoCAD, e se differiscono vale la regola dei
      passi (lo segnala il pacchetto di riferimenti, e decide il responsabile).

### `FILLET` e `CHAMFER`

13. **`FILLET`.** Opzioni *Undo*, *Polyline*, *Radius*, *Trim* e *Multiple*,
    con `Shift` che vale raggio 0. Il comando lavora su linee, archi, cerchi e
    segmenti della stessa polilinea; il raccordo viene da `fillet` di
    solid-space, con le sue regole, che seguono `autocad-raccordi.dxf`:
    - ogni punto indica la parte della sua curva da tenere; il lato del
      raccordo si decide esattamente con `side()`;
    - le linee e gli archi si accorciano o si allungano fino ai punti di
      tangenza, con le posizioni che solid-space dà; i cerchi non si tagliano;
    - con raggio 0 le due entità si incontrano in uno spigolo;
    - due curve dritte parallele si raccordano con un semicerchio, qualunque sia
      il raggio;
    - l'arco di raccordo è il più corto dei due tra i punti di tangenza;
    - due segmenti della stessa polilinea ricevono un segmento curvo in mezzo,
      con il bulge che dà solid-space; *Polyline* raccorda tutti i vertici,
      segmento per segmento, e i segmenti troppo corti restano come sono e si
      contano nel messaggio;
    - con *Trim No* le entità non cambiano e si aggiunge solo l'arco;
    - un arco di raccordo che la regola delle tolleranze trova degenere (ADR
      0036, punto 5) non si aggiunge: il comando fa lo spigolo, come con
      raggio 0.

    Le risposte "non esiste", "infiniti raccordi" e "il punto non indica un
    lato" di solid-space diventano messaggi; nessuna entità cambia. Fuori: il
    raccordo tra una polilinea e un'altra entità.
14. **`CIRCLE` tangente-tangente-raggio** usa `tangentCircleNear` di
    solid-space: il cerchio con i punti di tangenza più vicini ai due punti
    indicati. Per una polilinea vale il segmento più vicino al punto.
15. **`CHAMFER`.** Opzioni *Undo*, *Polyline*, *Distance*, *Angle*, *Trim*,
    *mEthod* e *Multiple*. Lavora su linee e segmenti dritti di polilinee.
    - Lo spigolo è il punto comune delle due rette, deciso da solid-space
      (`intersect` con le curve prolungate); due rette parallele o coincidenti
      non hanno spigolo e danno il messaggio di AutoCAD.
    - Lo smusso lo costruisce Tecnigrafo (ADR 0011 di solid-space, punto 11): i
      suoi estremi stanno alle distanze date dallo spigolo, ciascuno sulla
      parte della sua linea che il punto indica, cioè dalla parte dello spigolo
      dove cade la proiezione del punto. Una distanza più lunga della parte da
      tenere dà il messaggio di AutoCAD.
16. **Proprietà dell'arco di raccordo e della linea di smusso:** quelle della
    prima entità scelta se le due hanno le stesse proprietà, altrimenti quelle
    correnti. I file di prova hanno tutto sul layer 0 e non lo dicono: la regola
    va confermata con una prova in AutoCAD LT prima del pacchetto di `FILLET`.

### `JOIN`

17. **Interattivo, più oggetti insieme**, come AutoCAD dal 2012
    (`autocad-unione.dxf`):
    - linee allineate diventano una linea, anche con spazi tra loro;
    - archi dello stesso cerchio diventano un arco, anche con spazi;
    - linee, archi e polilinee che si toccano agli estremi diventano una
      polilinea;
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
    quando nessuna si aggiunge. Linee e archi si uniscono tra loro con la stessa
    regola, confrontando ciascuno con la sorgente.
19. **Negli script** vale la sequenza vecchia (ADR 0032): prima l'oggetto
    sorgente, poi gli oggetti da unire, accettati secondo il tipo della sorgente.
    Una linea accetta linee allineate, un arco archi dello stesso cerchio (uno per
    clic, con l'opzione *cLose*), una polilinea linee, archi e polilinee contigue.
20. **Identità:** l'entità sorgente resta (ADR 0031, punto 7); con più oggetti
    insieme la sorgente è il primo scelto.

### Per tutti

21. **`Esc`** conserva le operazioni già completate e annulla solo quella in
    corso (ADR 0015).
22. **Testi dei messaggi.** I messaggi che vengono da AutoCAD (taglio senza
    bordo, offset impossibile, raccordo impossibile, unione non riuscita,
    larghezze perse) si scrivono con le parole della versione inglese. Quelli
    osservati nella prova sono in italiano: la versione inglese si controlla
    prima del pacchetto che li introduce.
23. **Dati fuori dal dominio di solid-space** (coordinate o raggi oltre 2^50,
    circa 1.1e15): nessun disegno reale ci arriva. Le curve che solid-space
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
  promette il risultato esatto solo nei casi semplici, e un messaggio onesto
  negli altri; Tecnigrafo non promette di più.
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
- Due domande restano aperte fino alla prova in AutoCAD LT: proprietà dell'arco
  di raccordo (punto 16) e testi inglesi dei messaggi (punto 22). Se la prova
  smentisce il punto 4 nel modo rapido, lo si corregge prima del pacchetto di
  `TRIM`.
- Le funzioni di solid-space si usano solo quando il loro pacchetto di
  solid-space è unito; i pacchetti di Tecnigrafo che le usano spostano il
  sottomodulo a quel commit (ADR 0030, punto 9).
