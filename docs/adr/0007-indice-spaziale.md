# ADR 0007: Indice spaziale

- **Stato:** Accettato
- **Data:** 2026-10-16

## Contesto

Dalla tappa 0.3 il programma deve rispondere in fretta a due domande sul disegno:
quali entità sono vicine a un punto (clic di selezione, snap a ogni movimento del
puntatore) e quali stanno in un rettangolo (selezione a finestra e a
intersezione). Il criterio della tappa è una selezione a finestra su 100.000
polilinee in meno di 100 ms; lo snap, che si ripete a ogni movimento del mouse,
deve costare molto meno.

Oggi l'unico modo è percorrere tutte le entità e calcolarne il riquadro, che per
le polilinee con archi richiede seni e coseni. L'ADR 0004 aveva previsto che con
molte entità il `std::variant` andasse affiancato da un indice spaziale.

I disegni tecnici hanno caratteristiche che pesano sulla scelta:

- entità di dimensioni diversissime nello stesso disegno (un dettaglio di un
  millimetro accanto a una linea di un chilometro);
- coordinate che possono essere molto grandi (planimetrie in coordinate
  geografiche) e un'estensione che non si conosce in anticipo;
- modifiche continue: ogni comando aggiunge e toglie entità, e annulla e ripeti le
  rimettono.

## Decisione

### 1. Un R-tree dinamico in `core`

`core::SpatialIndex` è un R-tree (Guttman, 1984) di riquadri allineati agli assi,
ciascuno associato a un `EntityId`:

- divisione dei nodi **quadratica**, al massimo **16** e almeno **6** elementi per
  nodo (circa il 40%, come suggerito in letteratura);
- cancellazione con *condense tree*: gli elementi dei nodi rimasti sotto il
  minimo si reinseriscono;
- nodi in un unico vettore, collegati da indici e non da puntatori: l'indice si
  copia e si sposta con la semantica predefinita, e non c'è un'allocazione per
  nodo;
- interfaccia minima: inserire, togliere, cercare per rettangolo, riquadro totale.
  L'implementazione si può sostituire (per esempio con un R*-tree) senza toccare
  chi la usa.

L'indice è scritto nel progetto, in C++20 puro, senza librerie esterne.

### 2. Il documento possiede l'indice

`core::Document` contiene l'indice e lo aggiorna dentro `add`, `remove` e
`restore`. Poiché ogni modifica passa da lì (e, dall'ADR 0005, da `edit::Editor`),
l'indice non può divergere dal contenuto del disegno. Le entità con riquadro vuoto
(una polilinea senza vertici) restano nel documento ma non nell'indice.

Con l'occasione `Document` tiene anche una tabella da `EntityId` alla posizione
nell'elenco, così `find` diventa a tempo costante; `extents()` usa il riquadro
totale dell'indice.

### 3. L'indice dà candidati, la geometria decide

L'indice lavora solo sui riquadri. Le interrogazioni del disegno (scelta di
un'entità, selezione con rettangolo, snap; ADR 0008) chiedono all'indice i
candidati e poi applicano la geometria esatta di ogni entità. L'ordine dei
risultati dell'indice non è definito: chi ha bisogno di un ordine lo stabilisce
da sé.

### 4. Le prestazioni si misurano

Un programma di misura (`tg_bench`, fuori da `ctest`, perché in CI i tempi non sono
affidabili) crea 100.000 polilinee e misura selezione, scelta, snap e
cancellazione in build Release. I risultati vanno nelle note di rilascio.

## Alternative considerate

- **Nessun indice, scansione lineare.** La più semplice, e per una sola selezione
  a finestra probabilmente rientrerebbe nei 100 ms; ma ogni movimento del puntatore
  con lo snap percorrerebbe tutto il disegno, e il costo cresce con ogni tappa.
- **Griglia uniforme.** Facile da scrivere, ma la dimensione delle celle va scelta
  in anticipo: con entità di grandezze molto diverse le grandi occupano migliaia di
  celle e le piccole si ammassano in poche. Con coordinate grandi e un'estensione
  ignota la griglia va ricostruita.
- **Quadtree.** Richiede limiti iniziali e un trattamento delle entità a cavallo
  delle celle; le varianti "larghe" risolvono, ma con più parametri da tarare.
- **R*-tree.** Alberi di qualità migliore, ma reinserimenti forzati e una divisione
  più complessa. Si potrà adottare in seguito se le misure lo chiedono, senza
  cambiare interfaccia.
- **Libreria esterna (Boost.Geometry).** Collaudata, ma una dipendenza pesante per
  un solo componente, contro l'ADR 0002.

## Conseguenze

- Selezione e snap costano in proporzione alle entità vicine, non a tutto il
  disegno.
- Ogni aggiunta e rimozione aggiorna l'indice; la memoria cresce di qualche decina
  di byte per entità.
- L'indice va provato con cura: i test lo confrontano con la ricerca esaustiva su
  migliaia di riquadri casuali e ne controllano gli invarianti.
- `Document::remove` resta lineare, perché conserva l'ordine delle entità (che è
  l'ordine del disegno e del DXF). La cancellazione di moltissime entità in un
  colpo si misura con `tg_bench`; un eventuale rimedio va nella tappa 1.0.
- Il disegno della scena potrà usare l'indice per disegnare solo le entità
  visibili (tappa 1.0).
- Ogni nuova entità (tappa 0.4) entra nell'indice tramite il suo riquadro, senza
  altro lavoro.
