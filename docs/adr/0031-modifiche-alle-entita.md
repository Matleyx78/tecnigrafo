# ADR 0031: Modifiche alle entità: identità, dati conservati, trasformazioni

- **Stato:** Proposto
- **Data:** 2026-10-29

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
   | `TEXT` | punto d'inserimento e secondo punto si trasformano; l'altezza si moltiplica per il fattore di scala; la rotazione aumenta dell'angolo della similitudine; fattore di larghezza e obliquo non cambiano. La riflessione segue il punto 3 |

3. **Testo riflesso.** `φ` è l'angolo dell'asse di riflessione e `r` la rotazione
   del testo:
   - con `MIRRTEXT` = 1 il testo si specchia davvero: i due punti si riflettono,
     il segnale "all'indietro" (gruppo 71, bit 2) si inverte e la rotazione
     diventa `2φ − r + 180°`;
   - con `MIRRTEXT` = 0 (il valore predefinito, come in AutoCAD) il testo resta
     leggibile: il suo parallelogramma d'ingombro (ADR 0022, punto 5) si
     riflette, e il testo si riscrive dentro di esso con la stessa
     giustificazione, gli stessi segnali e la rotazione `2φ − r`, a cui si
     tolgono 180° quando cade in (90°, 270°]. Per *Aligned* e *Fit* i due punti
     riflessi si scambiano.

   Le regole sono ricavate da `autocad-specchiatura.dxf`. Con `MIRRTEXT` = 0 la
   posizione dipende dalle misure del font, che in Tecnigrafo sono quelle di
   Liberation Sans (ADR 0013): i riferimenti la calcolano con quelle misure, non
   la copiano da AutoCAD.
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
