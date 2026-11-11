# ADR 0049: Tracciamento degli snap a oggetto

- **Stato:** Accettato
- **Data:** 2026-11-11

## Contesto

Il responsabile ha deciso il 2026-11-11 ([#653](https://github.com/Matleyx78/tecnigrafo/issues/653))
che la tappa 0.8 porta il tracciamento degli snap a oggetto di AutoCAD (`OTRACK`,
`F11`): acquisizione di un punto di snap fermandosi sopra, guide orizzontali e
verticali dai punti acquisiti, aggancio alla guida e all'incrocio di due guide,
distanza scritta lungo una guida; spento in un disegno nuovo; negli script una
direttiva nuova per acquisire, così `!move` non cambia significato. Restano fuori
le guide polari dai punti acquisiti (`POLARMODE`), `TRACKPATH` e gli snap
*Prolungamento* e *Parallelo*.

Il runner ha già lo snap a oggetto ([ADR 0008](0008-selezione-e-snap.md),
[0033](0033-nuovi-snap-e-tracciamento-polare.md)) e il tracciamento polare, con
l'apertura di 10 pixel.

## Decisione

1. **Stato.** `DrawingAids` ha il tracciamento, spento per difetto e in un
   disegno nuovo, come orto e polare. `F11` lo accende e lo spegne con i
   messaggi `<Object Snap Tracking on>` e `<Object Snap Tracking off>`. Funziona
   solo con lo snap a oggetto acceso, come in AutoCAD.
2. **Acquisizione.** Durante una richiesta di punto, fermarsi su un punto di
   snap lo **acquisisce**; fermarsi di nuovo su un punto acquisito lo
   **rilascia**. Nel programma «fermarsi» è restare fermi per 500 ms; negli
   script è la direttiva del punto 7. Al più **7 punti** acquisiti: l'ottavo
   toglie il più vecchio. I punti acquisiti si azzerano quando la richiesta di
   punto riceve un punto o finisce.
3. **Guide.** Da ogni punto acquisito partono una guida orizzontale e una
   verticale, rette intere nel sistema del disegno.
4. **Aggancio.** Con il puntatore entro l'apertura (10 pixel) da una guida, il
   punto si proietta sulla guida; entro l'apertura da due guide di punti
   diversi, va al loro **incrocio**, che è esatto: la X dell'una e la Y
   dell'altra. Fra due guide vince la più vicina. Con il polare acceso conta
   come guida anche la semiretta polare dal punto base, e il suo incrocio con
   una guida del tracciamento si calcola con l'intersezione di solid-space già
   nel contratto (`geom2d::intersect` sulle rette estese).
5. **Priorità.** Snap a oggetto, poi incrocio di due guide, poi una guida, poi
   il polare, poi l'orto, poi il puntatore così com'è.
6. **Distanza scritta.** Con una guida attiva, una distanza scritta da sola
   (ADR 0014) mette il punto a quella distanza dal punto acquisito, lungo la
   guida, verso il puntatore, come oggi con il polare.
7. **Script.** `!hover x,y` porta il puntatore nel punto e lo ferma lì quanto
   basta per acquisire (livelli 1 e 2); `!move` non acquisisce mai.
   `!otrack on` e `!otrack off` accendono e spengono il tracciamento senza
   scrivere nella cronologia, come `!polar`. Al livello 2 anche `F11` con
   `!key`.
8. **Rapporto.** Righe nuove, come `pointer:` e `active snap:`: lo stato del
   tracciamento, i punti acquisiti in ordine e la guida o l'incrocio che
   aggancia (formato in [verifica/rapporto.md](../verifica/rapporto.md)).
9. **Interfaccia.** L'interruttore nella barra di stato con la sua icona, `F11`,
   il segno `+` sui punti acquisiti, le guide tratteggiate come quella polare e
   il suggerimento `Endpoint: <distanza> < <angolo>°` vicino al puntatore.
   Provati con i test Qt e la prova manuale.

## Alternative considerate

- **Acquisire con `!move` su un punto di snap:** cambierebbe il significato degli
  script che esistono (decisione del 2026-11-11).
- **Punti acquisiti che restano fino alla fine del comando:** in AutoCAD si
  azzerano quando si indica un punto; il responsabile lo conferma in AutoCAD LT
  (piano, «Prerequisiti»).
- **Incrocio con la guida polare fuori dalla 0.8:** è il caso più comune in
  AutoCAD (un punto allineato a un altro e a 45° dal precedente); costa una
  chiamata a solid-space che c'è già.

## Conseguenze

- Il runner del disegno ha uno stato in più per richiesta di punto; il
  protocollo dei comandi ([ADR 0041](0041-documenti-e-ambienti.md), punto 5) non
  cambia.
- Il catalogo di parità registra *Prolungamento*, *Parallelo*, `POLARMODE` e
  `TRACKPATH` come ancora mancanti.
