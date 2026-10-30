# ADR 0031: Modifiche alle entità: identità, dati conservati, trasformazioni

- **Stato:** Accettato
- **Data:** 2026-10-29
- **Precisazione:** 2026-10-30 — testo riflesso (decisione del responsabile, #406).

## Contesto

Fino alla tappa 0.6 un comando aggiunge entità, le cancella o ne cambia le
proprietà. La tappa 0.7 ne cambia la geometria: le sposta, le ruota, le scala,
le specchia, le taglia, le prolunga, le scompone e le unisce. Servono regole su
tre punti che i comandi non devono decidere ciascuno per conto suo:

- **identità:** quando un'entità modificata conserva il suo handle e quando ne
  nasce una nuova;
- **dati conservati:** che cosa portano con sé le entità nuove dei dati che il
  modello non rappresenta (ADR 0018), senza perderli e senza creare riferimenti
  sbagliati;
- **trasformazioni:** come ogni tipo di entità della 0.4 si sposta, ruota, scala
  e specchia, testo compreso.

Il responsabile ha preparato in AutoCAD LT sei DXF di prova
([tappe/0.7/dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md)). Da questi file si
vedono già alcune scelte di AutoCAD:

- un cerchio tagliato da `TRIM` diventa un arco con **lo stesso handle**;
- un arco tagliato in due tiene l'handle sul primo pezzo, e il secondo ne
  riceve uno nuovo;
- `JOIN` tiene l'handle dell'oggetto sorgente;
- `EXPLODE` crea entità nuove.

Il file `autocad-specchiatura.dxf` mostra come `MIRROR` tratta i testi con
`MIRRTEXT` 0 e 1.

## Decisione

### Trasformazioni

1. **Solo similitudini.** `core` offre le trasformazioni che conservano le forme:
   traslazione, rotazione, scala uniforme con fattore maggiore di zero,
   riflessione rispetto a una retta, e le loro composizioni (`core::Similarity`).
   La scala non uniforme non c'è: arriverà, se servirà, con i blocchi della
   tappa 0.8.
2. **Come si trasforma ogni entità** (`core::transformed(entity, similarity,
   mirrorText)`):

   | Entità | Regola |
   |---|---|
   | `LINE` | gli estremi si trasformano |
   | `CIRCLE` | il centro si trasforma; il raggio si moltiplica per il fattore di scala |
   | `ARC` | centro e raggio come il cerchio; gli angoli ruotano con la similitudine; con una riflessione inizio e fine si scambiano, perché l'arco resti antiorario (ADR 0012, conseguenze) |
   | `LWPOLYLINE` | i vertici si trasformano; con una riflessione ogni bulge cambia segno; le larghezze (gruppi 40, 41, 43) si moltiplicano per il fattore di scala; la generazione continua del tipo di linea non cambia |
   | `TEXT` | punto d'inserimento e secondo punto si trasformano; l'altezza si moltiplica per il fattore di scala; la rotazione aumenta dell'angolo della similitudine; fattore di larghezza e obliquo non cambiano senza riflessione. La riflessione segue il punto 3 |

3. **Testo riflesso.** La riflessione deve rispettare la geometria del testo
   definita nell'ADR 0022, punti 2–5. `r` è la **direzione effettiva** prima
   della trasformazione: per *Aligned* e *Fit* non degeneri è la direzione da
   `P1 = position` a `P2 = alignmentPoint`, non il gruppo 50 conservato nel
   file. Per gli altri testi è la rotazione memorizzata.

   Per una riflessione pura, `φ` è l'angolo dell'asse e
   `α = normalize(2φ − r)`. `normalize` riporta in [0°, 360°). Per una
   similitudine che riflette, `p′ = s·R(θ)·F·p + t` del punto 1, si usa
   `α = normalize(θ − r)`; le lunghezze si moltiplicano per `s`.

   | Proprietà | `MIRRTEXT` = 1 (specchiato) | `MIRRTEXT` = 0 (leggibile) |
   |---|---|---|
   | Direzione effettiva risultante `β` | `normalize(α + 180°)` | `α`, meno 180° se `90° < α ≤ 270°`, poi normalizzata |
   | Segnale "all'indietro" (gruppo 71, bit 2) | invertito | conservato |
   | Segnale "capovolto" (gruppo 71, bit 4) | conservato | conservato |
   | Obliquo (gruppo 51) | conservato | di segno opposto |
   | Giustificazione | conservata | conservata |

   "Leggibile" non raddrizza un testo che era già all'indietro o capovolto:
   conserva quei segnali. Con `MIRRTEXT` = 0 il cambio di segno dell'obliquo
   è necessario per occupare il parallelogramma riflesso con la direzione
   prescritta e gli stessi segnali. La regola generale del punto 2 sul
   mantenimento dell'obliquo vale per le trasformazioni senza riflessione;
   per una riflessione prevale questa tabella.

   **Posizione e punti.** Il bersaglio è l'immagine del parallelogramma
   `textBox` originale nella similitudine. Si mantengono altezza e fattore di
   larghezza memorizzati (l'altezza moltiplicata per `s`); le dimensioni
   effettive di *Aligned* e *Fit* si ricavano ancora dall'ADR 0022.

   - Per *Aligned* e *Fit* non degeneri si ricostruiscono i due punti con
     distanza `s·|P2 − P1|` e direzione da `P1′` a `P2′` uguale a `β`.
     Si traslano entrambi della stessa quantità affinché il centro del
     parallelogramma risultante coincida con quello del bersaglio. Il
     centro è la media dei quattro vertici, non una misura dei glifi.
   - Per gli altri allineamenti si ricolloca il punto d'ancoraggio
     dell'ADR 0022, punto 2, con lo stesso criterio del centro. L'altro punto,
     che non determina l'ingombro, riceve solo la similitudine originale.
   - *Aligned* e *Fit* degeneri seguono il trattamento come *Left* in `P1`
     dell'ADR 0022, punto 3: si usa la rotazione memorizzata e si ricolloca
     `P1` con il criterio del centro. Se i punti erano coincidenti restano
     coincidenti (anche `P2` riceve la ricollocazione); se solo la larghezza
     del testo è nulla, l'altro punto riceve la sola similitudine.

   La forma e le dimensioni fissate sopra rendono il parallelogramma
   risultante uguale al bersaglio come regione, anche per testo obliquo o
   già specchiato. Il criterio del centro fissa la traslazione senza
   ambiguità e senza una tolleranza geometrica nuova. Con `MIRRTEXT` = 1
   ogni punto dei caratteri si trasforma esattamente con la similitudine;
   anche i quattro vertici di `textBox`, nel loro ordine, devono coincidere
   con le immagini di quelli originali. Con `MIRRTEXT` = 0 i vertici
   coincidono come insieme, ma il loro ordine può cambiare.

   Il gruppo 50 risultante si scrive uguale a `β` anche per *Aligned* e
   *Fit*: non sostituisce la ricostruzione dei punti, perché l'ADR 0022 lo
   conserva ma non lo usa per determinarne la direzione.

   **Perché non si scambiano sempre i punti.** Per un testo senza segnali e
   senza obliquo da `(0,5)` a `(10,5)`, la riflessione sull'asse X richiede
   `β = 0°` con `MIRRTEXT` = 0: lo scambio darebbe invece 180°. I punti
   mantengono l'ordine lungo X e la linea di base va ricollocata per
   conservare l'ingombro riflesso. Sull'asse Y occorre invece invertire la
   direzione dei punti riflessi. La ricostruzione sopra comprende entrambi
   i casi, i segnali preesistenti e gli assi obliqui.

   **Rapporto con AutoCAD.** `autocad-specchiatura.dxf` resta il riferimento
   per l'effetto visibile e per le scelte di orientamento. Con `MIRRTEXT` = 0
   le posizioni dipendono dalle misure di Liberation Sans (ADR 0013), non
   si copiano da AutoCAD. Anche con `MIRRTEXT` = 1 i punti grezzi di
   *Aligned* e *Fit* non si copiano: AutoCAD può conservare i punti riflessi
   insieme a un gruppo 50 opposto alla loro direzione (per esempio il testo
   *Fit* `29A`). Nel modello dell'ADR 0022 quella combinazione non produce
   la riflessione richiesta: i punti si ricostruiscono come sopra, senza
   cambiare la definizione di disegno dell'ADR 0022. L'inversione
   dell'obliquo con `MIRRTEXT` = 0 è una scelta esplicita di questa
   precisazione, ricavata dalle formule dell'ADR 0022; il file AutoCAD
   citato non contiene quel caso e non ne costituisce una verifica.
4. **Estrusione rovesciata.** La geometria di un'entità con estrusione (0,0,−1)
   è già nel sistema del disegno (ADR 0018), quindi si trasforma come le altre e
   il segnale resta.

### Identità e dati conservati

5. **Modifica sul posto.** Spostare, ruotare, scalare, specchiare senza copia,
   tagliare, prolungare e i grip cambiano la geometria di un'entità che resta la
   stessa: stesso handle, stessa posizione nell'ordine, stesse proprietà e stessi
   dati non gestiti (`Editor::replace`). Il tipo può cambiare solo da `CIRCLE` ad
   `ARC`, come fa `TRIM` in AutoCAD: le due entità hanno in comune la sottoclasse
   `AcDbCircle` e i suoi dati.
6. **Entità ricavate da un'altra.** Quando un comando crea entità nuove a partire
   da una che esiste (`COPY`, `MIRROR` senza cancellare, `OFFSET`, il secondo
   pezzo di un taglio, `EXPLODE`), le nuove:
   - ricevono un handle nuovo e vanno in fondo all'ordine;
   - prendono le proprietà e i dati non gestiti dell'origine (`Editor::addCopy`),
     **tranne i gruppi 102** (reattori e dizionario di estensione). Quei gruppi
     legano l'origine ad altri oggetti del file, e una copia li renderebbe
     sbagliati. L'origine li conserva.
7. **Unione.** `JOIN` conserva l'entità sorgente e le cancella le altre. Se il
   risultato cambia tipo (linee e archi che diventano una polilinea), nasce
   un'entità nuova con le regole del punto 6, a partire dalla sorgente.
8. **`EXPLODE`** di una polilinea:
   - crea linee e archi con le regole del punto 6;
   - perde le larghezze, con il messaggio di AutoCAD
     `Exploding this polyline has lost width information.` e
     `The UNDO command will restore it.`;
   - rifiuta, con un messaggio, le polilinee con quota o spessore (gruppi 38 e
     39), che le linee e gli archi rappresentano con gruppi diversi;
   - con l'estrusione rovesciata dà entità con l'estrusione predefinita, perché
     la geometria è già nel sistema del disegno.
9. **Polilinea tagliata.** Nei punti di taglio le larghezze si interpolano
   lungo il segmento, in proporzione alla lunghezza.
10. **Dove si taglia e come si costruisce.** Dove un'entità si taglia o fin dove
    si prolunga lo decidono le intersezioni di solid-space (ADR 0035); le entità
    che ne risultano (un arco più corto, una linea più lunga, una polilinea
    aperta) le costruisce Tecnigrafo, con i punti che solid-space restituisce
    come nuovi estremi, senza ricalcolarli.

### Annullamento dentro un comando

11. **Punti di ripristino.** L'opzione *Undo* di `COPY`, `TRIM`, `EXTEND`,
    `OFFSET`, `FILLET` e `CHAMFER` toglie l'ultima operazione senza chiudere il
    comando. L'editor offre punti di ripristino dentro la transazione aperta
    (`Editor::savepoint()`, `Editor::rollbackTo()`), così il comando non deve
    ricordare da sé che cosa ha fatto.

## Alternative considerate

- **Trasformazioni affini generiche:** un cerchio scalato in modo non uniforme
  diventa un'ellisse, che Tecnigrafo non ha. Rimandate ai blocchi.
- **Copiare anche i gruppi 102:** due entità sarebbero reattori dello stesso
  oggetto, e il dizionario di estensione sarebbe condiviso. AutoCAD non lo fa.
- **Copie senza dati non gestiti:** si perderebbero XDATA e gruppi che l'utente
  si aspetta di ritrovare, contro l'ADR 0018.
- **Ogni cambio di tipo come entità nuova, anche per il cerchio tagliato:** più
  semplice, ma diverso da AutoCAD, che tiene l'handle.
- **Testo riflesso sempre leggibile, senza `MIRRTEXT`:** gli script di AutoCAD
  che impostano `MIRRTEXT` darebbero un altro risultato.

## Conseguenze

- `core` riceve le trasformazioni, `edit` tre operazioni (`replace`, `addCopy`, i
  punti di ripristino), e i comandi le usano senza toccare i dati non gestiti.
- `MIRRTEXT` è un valore della sessione, come i valori ricordati dai comandi
  (ADR 0014), e vale 0 all'avvio. Il valore scritto nel DXF resta tra i dati
  non gestiti dell'intestazione: leggerlo e scriverlo è un lavoro a parte, da
  decidere se servirà.
- Il cambio di tipo da `CIRCLE` ad `ARC` è l'unico permesso da `replace`: un
  altro richiede un ADR.
- Le regole del testo con `MIRRTEXT` = 0 valgono con il font di Tecnigrafo: con
  lo stesso disegno, in AutoCAD il testo può cadere un poco più in là.
