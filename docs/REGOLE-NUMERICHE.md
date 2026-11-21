# Regole numeriche

Le regole su pareggi, riduzioni e soglie che decidono un risultato quando i
numeri sono uguali, quasi uguali o fuori misura. Le ha prese il responsabile
una alla volta, quasi tutte nella tappa 0.8; qui sono raccolte in un posto
solo ([DECISIONI.md](DECISIONI.md), 2026-11-19).

- **Chi scrive una scheda** che confronta angoli, distanze o candidati rimanda
  alla regola di questo documento che la decide, invece di riscriverla.
- **Chi rivede il piano** controlla che la regola ci sia (punto 5 di
  [agenti/revisione.md](agenti/revisione.md), "Revisione del piano"):
  altrimenti è una [decisione].
- **Una regola nuova** si scrive qui, nella sezione giusta, con la data e il
  rimando alla voce di [DECISIONI.md](DECISIONI.md) che la racconta. Il
  registro resta la storia; questo documento è la regola com'è oggi.

## 1. Principi

1. **Decisioni esatte, costruzioni entro un errore.** Le decisioni (si
   intersecano, da che parte, in che ordine, quale candidato vince) sono
   esatte; i valori costruiti (punti, estremi, archi) valgono entro l'errore
   che solid-space dichiara, senza un arrotondamento prescritto («il `double`
   più vicino» o «pareggio al pari» non si promettono più). Il risultato è
   deterministico, gli stessi bit sui tre sistemi
   ([ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
   punti 1 e 2).
2. **Le soglie geometriche sono di solid-space.** Tecnigrafo non ne scrive di
   proprie per le decisioni nuove: usa la regola del documento, che dipende
   dall'unità del disegno ([ADR 0036](adr/0036-tolleranze-di-tecnigrafo.md),
   punti 1 e 2). Le soglie che `core` ha già sono elencate nell'ADR 0036,
   punto 7; le tolleranze in pixel nel punto 3.
3. **Fascia di una soglia.** Un caso che sta entro l'errore dichiarato da una
   soglia ha un esito libero dentro la fascia: nessun caso di verifica lo
   impone, e non serve una decisione (ADR 0013 di solid-space, punto 4;
   [agenti/revisione.md](agenti/revisione.md), "La fascia di una soglia").
4. **Dominio.** Oltre 2^50 (circa 1.1e15) i dati sono fuori dal dominio di
   solid-space: le curve si rifiutano e il comando lo dice
   ([ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punto 23). Per
   dati di grandezza sotto 2^-50 le costruzioni garantiscono solo niente
   crash, niente risultati fuori dominio e il determinismo, senza limite
   d'errore; le decisioni restano esatte per ogni `double` finito (ADR 0013 di
   solid-space, punto 2).
5. **Ingressi fuori dall'uso reale** (angoli di migliaia di giri, coordinate
   o scale molti ordini di grandezza oltre ciò che si disegna, numeri al limite
   del `double`): non sono un buco visibile. Basta che il programma non si
   arresti; poi vale la fascia dichiarata o una diagnosi
   ([DECISIONI.md](DECISIONI.md), 2026-11-12, «Ingressi fuori dall'uso
   reale»).
6. **Valori scritti dall'utente.** Lo zero di un raggio, una distanza o un
   fattore si riconosce con il confronto esatto con 0: il numero scritto è un
   dato, non un risultato da arrotondare (ADR 0036, punto 6).
7. **Degeneri.** Un pezzo che la regola trova degenere dopo un taglio, una
   curva parallela o un raccordo non si crea (ADR 0036, punto 5).

## 2. Angoli

1. **Quarti di giro esatti.** Seno e coseno di un angolo entro `2^-48` da un
   multiplo di 90° valgono esattamente 0 e ±1 (`cardinalSinCos` in
   `tg/core/math.hpp`). Li usano tutte le trasformazioni
   (`Similarity::matrix`): `ROTATE` e `MIRROR` di un multiplo di 90° danno
   coordinate esatte. Oltre la soglia vale la fascia dichiarata nel commento
   della funzione ([DECISIONI.md](DECISIONI.md), 2026-11-12, «Quarti di giro
   esatti»; [0.8-PT1](tappe/0.8/PT1-trigonometria-1.md), punto 4).
2. **Angoli in gradi ridotti a un giro, prima della conversione.** Un angolo
   in gradi che entra nel programma si riduce con `std::fmod(gradi, 360)`
   (esatto) e solo dopo si converte in radianti (`reducedDegreesToRadians`).
   Vale per gli angoli letti dal DXF (gruppi 52 e 53 dei retini; il DXF si
   riscrive con l'angolo ridotto), per l'angolo digitato in `-HATCH`, per ogni
   richiesta `PromptKind::Angle` (`ROTATE` e la sua opzione *Reference*, la
   rotazione di `TEXT`) e per l'angolo delle coordinate polari. Ridurre dopo,
   in radianti, non basta: l'errore nasce nella conversione
   ([DECISIONI.md](DECISIONI.md), 2026-11-12, «Angoli in gradi ridotti a un
   giro» e «Angoli digitati ridotti a un giro»).
3. **Angoli calcolati** (per esempio da due punti): niente riduzione in
   gradi; vale la fascia di `cardinalSinCos` (stesse voci).
4. **Rotazione in [0°, 360°).** Il valore proposto e quello salvato di una
   rotazione stanno sempre in [0°, 360°), qualunque sia il percorso d'ingresso:
   `-90` digitato e il punto `(0,-10)` danno tutti e due 270°, un giro intero
   dà 0°. Se il testo della domanda arrotonda a `360`, mostra `<0>`; il valore
   memorizzato non cambia ([DECISIONI.md](DECISIONI.md), 2026-11-13,
   «Rotazione di TEXT in [0°, 360°)» e «Domanda di TEXT a 360°»;
   [0.8-PT1](tappe/0.8/PT1-trigonometria-1.md), punto 5).
5. **Asse di un cerchio come ellisse.** Un cerchio ha l'asse su X positivo;
   altrimenti l'asse maggiore sta in [0°, 180°) ([DECISIONI.md](DECISIONI.md),
   2026-11-14, «Revisione delle interfacce PT4», P12).
6. **Correzione angolare minima** dei pezzi di archi e cerchi (`partOf`,
   `extendTo`). Quando gli angoli calcolati contraddicono il verso o il
   contenimento nell'arco originale, vince la correzione che minimizza la
   somma degli spostamenti angolari circolari, con confronti esatti senza
   soglie; a parità prevale l'inizio originale, poi quello del pezzo. Al
   taglio 0/2π di un cerchio, entro l'errore dichiarato, l'esito è libero
   nella fascia ([DECISIONI.md](DECISIONI.md), 2026-11-03, «P15: correzione
   angolare minima» e «P15, terzo verbale»).

## 3. Distanze

1. **Al quadrato.** Il tracciamento confronta le distanze al quadrato
   (`dx*dx + dy*dy`), senza radice: dopo la radice due distanze diverse
   possono dare lo stesso `double`. A quadrati uguali valgono i pareggi della
   sezione 4 ([DECISIONI.md](DECISIONI.md), 2026-11-14, «Distanze confrontate
   al quadrato»). Chi confronta distanze in un caso nuovo fa lo stesso.
2. **Estremi alla stessa distanza.** Quando due estremi di una curva sono
   alla stessa distanza dal punto indicato, vince l'inizio; la coincidenza si
   decide prima della graduatoria per `fuzz` ([DECISIONI.md](DECISIONI.md),
   2026-11-14, «Revisione delle interfacce PT5», B21).

## 4. Pareggi fra candidati

### Snap a oggetto

1. Vince il punto più vicino al puntatore; a pari distanza l'ordine è
   estremo, intersezione, medio, quadrante, perpendicolare, tangente; centro e
   inserimento solo se nessun altro è entro l'apertura
   ([ADR 0033](adr/0033-nuovi-snap-e-tracciamento-polare.md), punto 5).
2. Il punto d'intersezione di una coppia porta il maggiore dei due
   identificativi; un'autointersezione porta quello della polilinea
   ([DECISIONI.md](DECISIONI.md), 2026-11-03, «Revisione P15»).

### Orto, polare e tracciamento

1. **Priorità:** snap a oggetto, poi incrocio di due guide, poi una guida, poi
   il polare, poi l'orto, poi il puntatore così com'è
   ([ADR 0049](adr/0049-tracciamento-degli-snap-a-oggetto.md), punto 5).
2. **Orto a 45° esatti:** vince l'orizzontale
   ([ADR 0008](adr/0008-selezione-e-snap.md)).
3. **Guide dello stesso punto:** a pari distanza fra l'orizzontale e la
   verticale di uno stesso punto acquisito vince l'**orizzontale**
   ([DECISIONI.md](DECISIONI.md), 2026-11-13, «Pareggio fra le guide dello
   stesso punto»).
4. **Guide di punti diversi:** vince la guida del punto acquisito **più di
   recente** ([0.8-PT34](tappe/0.8/PT34-otrack-cmd.md), punto 3).
5. **Due incroci di guide:** vince l'incrocio con la guida orizzontale del
   punto più recente; a pari orizzontale, quello con la verticale del punto
   più recente ([DECISIONI.md](DECISIONI.md), 2026-11-13, «Pareggio fra due
   incroci»).
6. **Incrocio di guide contro incrocio con il polare:** vince l'incrocio delle
   due guide acquisite ([DECISIONI.md](DECISIONI.md), 2026-11-13, «Pareggio
   fra un incrocio di guide e uno con il polare»).
7. **Due incroci con la stessa semiretta polare:** vince quello con la guida
   del punto acquisito più di recente ([DECISIONI.md](DECISIONI.md),
   2026-11-14, «Pareggio fra due incroci con il polare»).

### Quote

1. **Testo sulla curva** di una quota radiale o diametrale: conta come dentro;
   il pareggio della diametrale va a `point15` ([DECISIONI.md](DECISIONI.md),
   2026-11-14, «Revisione delle interfacce PT5», B16).
2. **Punto su una semiretta** di una quota angolare: l'angolo è quello che
   segue in senso antiorario; nel vertice la quota non si rigenera (stessa
   voce, B13).

## 5. Che cosa non sta qui

- Le tolleranze dei casi di verifica (`1e-9` assoluto e l'errore dichiarato da
  solid-space): [ADR 0036](adr/0036-tolleranze-di-tecnigrafo.md), punto 4.
- Le tolleranze in pixel (clic, apertura dello snap, grip, polare,
  trascinamento): ADR 0036, punto 3.
- Le regole numeriche interne di solid-space (predicati, errori delle
  costruzioni): il contratto di solid-space, nelle intestazioni e in
  [CONTRATTI.md](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md).
