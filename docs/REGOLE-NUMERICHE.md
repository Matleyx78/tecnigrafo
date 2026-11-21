# Regole numeriche

Le regole su pareggi, riduzioni e soglie che decidono un risultato quando i
numeri sono uguali, quasi uguali o fuori misura. Le ha prese il responsabile
una alla volta, dalla tappa 0.3 in poi, quasi tutte nelle tappe 0.7 e 0.8; qui
sono raccolte in un posto solo ([DECISIONI.md](DECISIONI.md), 2026-11-19, «Le
proposte di metodo della 0.8»).

- **Chi scrive una scheda** che confronta angoli, distanze o candidati rimanda
  alla regola di questo documento che la decide, invece di riscriverla.
- **Chi rivede il piano** controlla che la regola ci sia (punto 5 di
  [agenti/revisione.md](agenti/revisione.md), "Revisione del piano"):
  altrimenti è una [decisione].
- **Una regola nuova** si scrive qui, nella sezione giusta, con il rimando alla
  voce di [DECISIONI.md](DECISIONI.md) o al punto dell'ADR che la decide. Il
  registro resta la storia; questo documento è la regola com'è oggi.
- **Ogni regola vale dove dice la sua fonte**, non oltre: una regola scritta per
  il tracciamento non vale da sé per lo snap. Un caso nuovo che nessuna regola
  copre si tratta come ogni buco di una scheda: se è visibile lo decide il
  responsabile; se non lo è, chi realizza sceglie la risposta più semplice, la
  scrive nella scheda o nel commento dell'interfaccia, la fissa con un test e
  la segnala nella pull request
  ([agenti/pacchetto.md](agenti/pacchetto.md), "Buchi visibili e no").

## 1. Principi

1. **Decisioni esatte, costruzioni entro un errore** (solid-space). Le
   decisioni del kernel (il lato, se e quante volte due curve si incontrano, la
   coincidenza, quale diagnosi e in che ordine) sono esatte; le costruzioni
   (punti, raggi, centri, stazioni, lunghezze, bulge) valgono entro l'errore
   dichiarato, senza promettere quale `double` esce. Restano esatti i valori
   che il contratto copia o fissa per struttura (per esempio il centro
   conservato di un arco spostato, il raggio richiesto, il bulge 0 dei pezzi
   dritti); un valore esatto entro `kMaxCoordinate` non si restituisce oltre
   (se l'arrotondamento lo supererebbe, si restituisce il confine); le costanti
   angolari di `curves.hpp` sono i `double` più vicini ai valori esatti. Tutto
   è deterministico: stesso ingresso, stessi bit sui tre sistemi ([ADR 0013 di
   solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
   punto 1).
2. **Le soglie geometriche sono di solid-space.** Tecnigrafo non scrive soglie
   geometriche proprie per le decisioni nuove; dove una decisione di Tecnigrafo
   ne ha bisogno (pezzi degeneri, curva parallela, raccordo degenere,
   coincidenze di `JOIN`) usa la regola delle tolleranze di solid-space per
   l'unità del documento. Le decisioni esatte (si intersecano, da che parte, in
   che ordine) **non la usano mai**
   ([ADR 0036](adr/0036-tolleranze-di-tecnigrafo.md), punti 1 e 2). Le soglie
   che `core` ha già sono nell'ADR 0036, punto 7; le tolleranze in pixel nel
   punto 3.
3. **Fascia.** Quando un esito dipende solo da quale `double` una costruzione
   sceglie entro il suo errore dichiarato, quell'esito è libero dentro la
   **fascia**: l'insieme dei dati per cui due scelte lecite danno esiti diversi.
   Dentro la fascia l'esito resta deterministico, ma il contratto non lo fissa e
   nessun caso lo impone; ogni funzione che ha esiti così scrive la sua fascia
   nel suo commento (ADR 0013 di solid-space, punto 4;
   [agenti/revisione.md](agenti/revisione.md), "La fascia di una soglia").
4. **Dominio in alto.** Le funzioni di `geom2d` rifiutano con `outOfDomain` una
   coordinata, un raggio o il punto m di un segmento curvo di grandezza oltre
   `kMaxCoordinate` (2^50, circa 1.1e15, in valore assoluto: anche `-2^51` è
   fuori), e un `Arc` con `startAngle` o `startAngle + sweep` di grandezza
   oltre `kMaxTrigArgument`, anche se l'angolo finale riportato a un giro
   starebbe entro il limite. Nei controlli di ogni curva `outOfDomain` viene
   dopo `invalidValue` (NaN, infinito, raggio negativo) (`curves.hpp`,
   «Checking the data»). Le curve
   rifiutate non tagliano, non si raccordano e non si uniscono, e il comando lo
   dice con il messaggio dell'operazione non riuscita
   ([ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punto 23).
5. **Dominio in basso.** I limiti d'errore delle costruzioni valgono per dati
   di grandezza almeno 2^-50. Sotto, subnormali compresi, il contratto
   garantisce solo niente crash, nessuna eccezione oltre a `std::bad_alloc`,
   niente risultati fuori dominio e il determinismo. Le decisioni di `robust` e
   di `geom2d` restano esatte per ogni `double` finito (ADR 0013 di
   solid-space, punto 2).
6. **Ingressi fuori dall'uso reale** (angoli di migliaia di giri, coordinate o
   scale molti ordini di grandezza oltre ciò che si disegna, numeri al limite
   del `double`): non sono un buco visibile, né per chi rivede né per chi
   realizza. Basta che il programma non si arresti; poi vale la fascia
   dichiarata o una diagnosi ([DECISIONI.md](DECISIONI.md), 2026-11-12,
   «Ingressi fuori dall'uso reale»; [agenti/revisione.md](agenti/revisione.md),
   "Che cosa blocca una revisione").
7. **Valori scritti dall'utente.** Un raggio, una distanza o un fattore sono
   zero solo al confronto esatto con 0: il numero scritto è un dato, non un
   risultato da arrotondare. Raggio 0 in `FILLET` dà lo spigolo; distanza 0 in
   `OFFSET` e fattore 0 in `SCALE` si rifiutano (ADR 0036, punto 6).
8. **Degeneri.** Un pezzo che dopo un taglio, una curva parallela o un raccordo
   la regola trova degenere non si crea (ADR 0036, punto 5). Un pezzo di
   polilinea è degenere quando lo sono tutti i suoi segmenti
   ([DECISIONI.md](DECISIONI.md), 2026-11-01, «Buchi delle schede P15–P23: le
   decisioni», B4).

## 2. Angoli

1. **Direzioni e quarti di giro esatti.** `cardinalSinCos`
   (`tg/core/math.hpp`) dà `sinCos(angolo)`, ma una componente sotto `2^-48` in
   modulo diventa esattamente 0 e l'altra esattamente ±1, con il suo segno. La
   usano le direzioni delle famiglie dei retini (anche dopo `ROTATE` e `MIRROR`
   e per lo scostamento calcolato dai `.pat`) e `Similarity::matrix`, per
   rotazione e riflessione: `ROTATE` di un multiplo di 90° e `MIRROR` su un asse
   a un multiplo di 45° danno coordinate esatte, per gli angoli digitati e per
   quelli entro la fascia dichiarata nel commento di `cardinalSinCos`. Fuori da
   quella fascia, un angolo calcolato dà il risultato di `sinCos`
   ([DECISIONI.md](DECISIONI.md), 2026-11-12, «Direzioni cardinali esatte delle
   righe dei motivi» e «Quarti di giro esatti»;
   [0.8-PT1](tappe/0.8/PT1-trigonometria-1.md), punti 1 e 4).
2. **Angoli in gradi ridotti a un giro, prima della conversione.**
   `reducedDegreesToRadians`, con un argomento finito, riduce l'angolo
   a [0, 360) con `std::fmod` (esatto), aggiunge 360 a un resto negativo, porta a +0
   un risultato che arrotonda a 360 o è −0, e solo dopo converte in radianti.
   Ridurre dopo, in radianti, non basta: l'errore nasce nella conversione. Fra
   gli angoli che entrano già in gradi si applica a questi, e solo a questi:
   - gli angoli dei retini letti dal DXF (gruppi 52 e 53; il DXF si riscrive
     con l'angolo ridotto) e l'angolo digitato in `-HATCH`;
   - il valore di una richiesta `PromptKind::Angle` (l'angolo di `ROTATE`,
     quelli della sua opzione *Reference*, la rotazione di `TEXT`);
   - l'angolo di una coordinata polare `d<a`.

   ([DECISIONI.md](DECISIONI.md), 2026-11-12, «Angoli in gradi ridotti a un
   giro» e «Angoli digitati ridotti a un giro»; PT1, punti 1 e 5.)
3. **Angoli calcolati** (per esempio da due punti): non passano dalla
   riduzione; vale la fascia di `cardinalSinCos` (stesse voci). L'eccezione è la
   rotazione di `TEXT`, al punto seguente.
4. **Rotazione del comando `TEXT` in [0°, 360°).** Il valore che `TEXT`
   propone alla domanda successiva e quello che salva nell'entità (gruppo DXF
   50) stanno sempre in [0°, 360°), qualunque sia il percorso d'ingresso: anche
   l'angolo dato con un punto, convertito in gradi, passa da
   `reducedDegreesToRadians(radiansToDegrees(atan2(…)))`, così `-90`
   digitato e il punto a −90° danno gli stessi bit di `270`, e un giro intero
   dà 0°. Il testo della domanda segue lo stesso intervallo: quando
   l'arrotondamento a quattro decimali scrive `360`, la domanda mostra `<0>`,
   mentre Invio e il gruppo DXF 50 usano il valore memorizzato. Vale per il
   comando: le entità lette da un DXF tengono la loro rotazione
   ([DECISIONI.md](DECISIONI.md), 2026-11-13, «Rotazione di TEXT in [0°,
   360°)» e «Domanda di TEXT a 360°»; PT1, punto 5).
5. **Ampiezza di un `ARC` per solid-space.** L'ampiezza è `endAngle −
   startAngle`, a cui si aggiunge `kFullTurn` se non è positiva, calcolata in
   `double` in quest'ordine ([ADR 0035](adr/0035-taglio-offset-raccordi-unione.md),
   punto 1).
6. **Angoli dei nuovi estremi di un arco** tagliato, prolungato o raccordato:
   si ricavano dai punti di solid-space con `solid_space::robust::atan2` e si
   riportano in [0, 2π) (ADR 0035, punto 1).
7. **Asse di un'ellisse con rapporto 1.** Un cerchio ha l'asse su X positivo;
   altrimenti l'asse maggiore sta in [0°, 180°). Se il grip di un asse
   accorcia il maggiore sotto il minore, gli assi si scambiano, e a rapporto 1
   esatto vale l'asse su X positivo ([DECISIONI.md](DECISIONI.md), 2026-11-14,
   «Revisione delle interfacce PT4», P12 e P14, e «Secondo giro delle
   revisioni PT4 e PT5», P14).
8. **Pezzi di archi, cerchi e segmenti curvi** (`partOf`, `extendTo`):
   - **collasso** (B1): se i due angoli di un arco, o i due vertici di un
     segmento curvo, coincidono, il pezzo corto è vuoto (`Empty`) e quello
     lungo non rappresentabile (`NotRepresentable`), mai un cerchio; in una
     parte di polilinea con più segmenti, il tratto curvo collassato prende
     bulge 0 conservando i punti ricevuti ([DECISIONI.md](DECISIONI.md),
     2026-11-01, «Buchi delle schede P15–P23: le decisioni», B1; 2026-11-03,
     «Revisione P15: angoli invertiti e identificativo dello snap»);
   - **correzione angolare minima**: quando gli angoli calcolati contraddicono
     `PartCourse` o il contenimento del pezzo esatto nell'arco originale, vince
     la correzione che minimizza la somma degli spostamenti angolari circolari
     fra il collasso (B1) e il riporto di uno o di tutti e due gli angoli agli
     estremi originali, oppure, per un cerchio, al taglio 0/2π dal lato
     richiesto da `PartCourse`. Confronti esatti sui valori calcolati, senza
     soglie; a parità prevale l'inizio originale, poi quello del pezzo. Vale per
     `Arc`, cerchio con `Direct` e `AcrossStart`, segmenti curvi nei due versi
     ed `extendTo`; per il prolungamento l'intervallo consentito è il
     complemento dell'arco, con scelta fra prolungamento minimo e chiusura non
     rappresentabile, sempre preferendo l'inizio originale a parità
     ([DECISIONI.md](DECISIONI.md), 2026-11-03, «P15: correzione angolare
     minima, decisione C completa»);
   - **fascia al taglio 0/2π**: quando i *due* posti esatti di un cerchio
     stanno entro l'errore dichiarato dal taglio, da lati opposti, l'esito
     della correzione è libero e deterministico, con `Direct` e con
     `AcrossStart` ([DECISIONI.md](DECISIONI.md), 2026-11-03, «P15, terzo
     verbale: fascia del cerchio al taglio 0/2π»).

## 3. Distanze e punti indicati

1. **Al quadrato, nel tracciamento.** Il tracciamento degli snap confronta le
   distanze dal puntatore al quadrato (`dx*dx + dy*dy` in `double`), senza
   radice; a quadrati uguali valgono i pareggi del tracciamento (sezione 4)
   ([DECISIONI.md](DECISIONI.md), 2026-11-14, «Distanze confrontate al
   quadrato»; [0.8-PT34](tappe/0.8/PT34-otrack-cmd.md), punto 3). Fuori dal
   tracciamento la regola non è decisa: una scheda che confronta distanze la
   cita solo se il responsabile la estende.
2. **Clic sul taglio.** In `TRIM`, se il punto del clic, portato sull'oggetto
   con la proiezione di Tecnigrafo, cade esattamente su un taglio, si toglie il
   pezzo che segue (ADR 0035, punto 5).
3. **Lato di `OFFSET`.** Per una polilinea vale il segmento più vicino al punto
   indicato, con la proiezione di Tecnigrafo, e a pari distanza il primo. Un
   punto esattamente sulla retta o sul cerchio di quel segmento non indica un
   lato: nessuna copia, con il messaggio di AutoCAD (ADR 0035, punto 10).
4. **Distanze di `CHAMFER`.** Una distanza di smusso, o la lunghezza, più lunga
   della parte tenuta della sua linea dà `DistanceTooLarge` (`Chamfer distance
   too large.`); con una distanza zero e l'altra troppo lunga prevale `None`,
   in tutti e due gli ordini. Con *Angle*: prima si controlla la lunghezza
   sulla prima linea, e se supera la parte tenuta l'esito è `DistanceTooLarge`,
   anche se la retta dello smusso è parallela alla seconda o la incontra nello
   spigolo o dalla parte opposta; superato quel controllo, una retta parallela
   alla seconda linea o un incontro nello spigolo o dalla parte opposta dà
   `None`, e un incontro dalla parte tenuta della seconda linea ma oltre la
   fine di quella parte dà `DistanceTooLarge`
   ([0.7-P21](tappe/0.7/P21-chamfer.md), *Angle*; [DECISIONI.md](DECISIONI.md),
   2026-11-05, «Esiti di `FILLET` e `CHAMFER`
   non riusciti», e 2026-11-06, «P21, precedenza della prima lunghezza in
   Angle»; ADR 0035, punto 15).
5. **Unione con distanza di `PEDIT`.** Si parte dalla fine della polilinea,
   poi dall'inizio. A ogni passo si attacca, fra gli oggetti
   non usati, quello con un estremo alla distanza minima ≤ `fuzz`
   dall'estremità della polilinea; a pari distanza il primo in `others`; un
   oggetto con i due estremi alla stessa distanza si attacca dal suo inizio. Un
   oggetto che coincide secondo la regola del documento passa prima della
   graduatoria per `fuzz`, anche quando `fuzz` è più piccolo della tolleranza.
   Con `fuzz` uguale a 0 vale la regola di `JOIN`
   ([DECISIONI.md](DECISIONI.md), 2026-11-14, «Revisione delle interfacce
   PT5», B21; [0.8-PT32](tappe/0.8/PT32-unione-con-distanza.md), punto 3).

## 4. Pareggi fra candidati

### Selezione e snap a oggetto

1. **Clic:** a parità di distanza dal clic vince l'entità con l'id più alto
   ([ADR 0008](adr/0008-selezione-e-snap.md), punto 2).
2. **Snap a oggetto:** estremo, intersezione, medio, quadrante, perpendicolare
   e tangente entro l'apertura competono, e vince il più vicino al puntatore; a
   pari distanza vale quest'ordine, e a pari distanza e tipo l'id più alto. Se
   nessuno è entro l'apertura, valgono centro e inserimento dell'entità su cui
   passa il puntatore ([ADR 0033](adr/0033-nuovi-snap-e-tracciamento-polare.md),
   punto 5; ADR 0008, punto 4).
3. **Identificativo di un'intersezione:** il punto di una coppia porta il
   maggiore dei due identificativi; un'autointersezione porta quello della
   polilinea ([DECISIONI.md](DECISIONI.md), 2026-11-03, «Revisione P15: angoli
   invertiti e identificativo dello snap»).

### Orto e polare

1. **Orto:** con l'orto attivo e un punto base del comando in corso, il punto
   va sull'orizzontale o sulla verticale passante per il punto base, quella più
   vicina alla direzione del puntatore; a 45° esatti, l'orizzontale (ADR 0008,
   punto 5).
2. **Polare:** con un punto base, le semirette escono da quel punto, nelle
   direzioni multiple di `POLARANG` misurate dall'asse X del disegno;
   `POLARANG` accetta un valore maggiore di 0 e al più 180. Entro l'apertura da
   due semirette vince la più vicina (ADR 0033, punti 8 e 9); a pari distanza
   vince la semiretta **in senso orario**: base `(0,0)`, passo 90° e puntatore
   `(5,5)` danno `(5,0)`, come fa il programma dalla tappa 0.7 (`polarPoint` in
   `cmd/src/drawing_aids.cpp`, test `HalfwayBetweenTwoRaysTheClockwiseOneWins`;
   [DECISIONI.md](DECISIONI.md), 2026-11-21, «Pareggio fra due semirette del
   polare»).

### Tracciamento degli snap

La priorità è: snap a oggetto, poi incrocio di due guide, poi una guida, poi il
polare, poi l'orto, poi il puntatore così com'è
([ADR 0049](adr/0049-tracciamento-degli-snap-a-oggetto.md), punto 5). Fra più
guide vince la più vicina; il puntatore entro l'apertura da due guide di
**punti diversi** va al loro incrocio, mentre l'orizzontale e la verticale
dello stesso punto non fanno incrocio (ADR 0049, punto 4). A pari distanza dal
puntatore (confrontata al quadrato, sezione 3):

1. **guide di punti diversi:** vince la guida del punto acquisito più di
   recente;
2. **orizzontale e verticale dello stesso punto** (anche con un solo punto
   acquisito): vince l'orizzontale, come l'orto a 45°
   ([DECISIONI.md](DECISIONI.md), 2026-11-13, «Pareggio fra le guide dello
   stesso punto»);
3. **due incroci di guide di punti diversi:** vince quello che usa la guida
   orizzontale del punto più recente; se l'orizzontale è dello stesso punto,
   quello che usa la verticale del punto più recente
   ([DECISIONI.md](DECISIONI.md), 2026-11-13, «Pareggio fra due incroci»);
4. **incrocio di due guide e incrocio di una guida con la semiretta polare:**
   vince il primo, come nella priorità ([DECISIONI.md](DECISIONI.md),
   2026-11-13, «Pareggio fra un incrocio di guide e uno con il polare»);
5. **due incroci con la stessa semiretta polare**, ognuno con una sola guida
   acquisita: vince quello la cui guida è del punto acquisito più di recente
   ([DECISIONI.md](DECISIONI.md), 2026-11-14, «Pareggio fra due incroci con il
   polare»).

(Tutti in [0.8-PT34](tappe/0.8/PT34-otrack-cmd.md), punto 3.)

### `JOIN`

1. **Ordine della catena:** parte dalla sorgente e cresce prima dalla sua
   fine, poi dal suo inizio; a ogni passo si aggiunge la prima entità,
   nell'ordine in cui sono state scelte, che ha un estremo coincidente con
   l'estremo libero della catena; ci si ferma quando nessuna si aggiunge o la
   catena si chiude (ADR 0035, punto 18).
2. **Verso:** se coincidono tutti e due gli estremi della candidata, vince il
   suo inizio, che diventa il vertice comune: dalla fine si aggiunge com'è,
   dall'inizio si gira. Con un solo estremo coincidente, il vertice comune è
   quello e la candidata si orienta allo stesso modo
   ([DECISIONI.md](DECISIONI.md), 2026-11-02, «Buchi della scheda P22: le
   decisioni», punto 1).
3. **Archi dello stesso cerchio:** ogni arco si copre in senso antiorario a
   partire dall'inizio della sorgente, e l'arco che risulta va dall'inizio della
   sorgente al più lontano dei loro estremi. Prima di tutto almeno un altro arco
   deve unirsi: con la sola sorgente niente si unisce, anche quando le regole
   seguenti darebbero il cerchio. Se il più lontano arriva al giro intero o
   oltre, **oppure** l'angolo finale tornerebbe uguale a quello iniziale, gli
   archi formano un cerchio, con la domanda di conversione. *cLose* di
   `joinToSource` è l'eccezione: dà sempre il cerchio, anche con la sola
   sorgente, senza domanda (ADR 0035, punti 17 e 19;
   [DECISIONI.md](DECISIONI.md), 2026-11-02, «B6 e B11 riviste dopo le prove
   in AutoCAD» e «Buchi della scheda P22: le decisioni», punto 4;
   [0.7-P22](tappe/0.7/P22-unione-core.md)).
4. **Segmento dritto o curvo:** `isStraight` decide sul segmento com'è nella
   sua entità, con gli estremi e il bulge con cui entra nella catena (girato se
   l'entità si gira), prima dello spostamento sul vertice comune, anche alla
   chiusura; un `ARC` si tratta come il suo segmento. Quando costruzioni lecite
   degli estremi e del bulge di un `ARC` danno frecce dai due lati di epsilon,
   l'esito è libero nella fascia ([DECISIONI.md](DECISIONI.md), 2026-11-04,
   «P22: `isStraight` sul segmento d'ingresso»).

### Quote

1. **Fuori dominio:** una quota con valori fuori dominio si conserva e non si
   rigenera (B07).
2. **Misura zero:** una lineare di misura zero ha il testo `0`; un'allineata
   con le origini uguali non si rigenera (B08).
3. **Angolari:** un punto su una semiretta dà l'angolo che segue in senso
   antiorario, e nel vertice la quota non si rigenera (B13); `DIMLFAC` e
   `DIMRND` non si applicano agli angoli (B15).
4. **Radiali e diametrali:** il testo sulla curva conta come dentro; il
   pareggio della diametrale va a `point15` (B16). Con `DIMCEN` negativo e un
   raggio fino a `2·|DIMCEN|` niente linee d'asse, solo la croce
   (secondo giro, B16).

([DECISIONI.md](DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT5» e
«Secondo giro delle revisioni PT4 e PT5».)

## 5. Che cosa non sta qui

- Le tolleranze dei casi di verifica (`1e-9` assoluto e l'errore dichiarato da
  solid-space): [ADR 0036](adr/0036-tolleranze-di-tecnigrafo.md), punto 4.
- Le tolleranze in pixel (clic, apertura dello snap, grip, polare,
  trascinamento): ADR 0036, punto 3.
- Le regole numeriche interne di solid-space (predicati, errori delle
  costruzioni, fasce di ogni funzione): il contratto di solid-space, nelle
  intestazioni e in
  [CONTRATTI.md](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md).
