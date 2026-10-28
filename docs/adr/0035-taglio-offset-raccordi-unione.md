# ADR 0035: Taglio, prolungamento, offset, raccordi, smussi e unione

- **Stato:** Proposto
- **Data:** 2026-10-28

## Contesto

`TRIM`, `EXTEND`, `OFFSET`, `FILLET`, `CHAMFER` e `JOIN` sono i comandi più
difficili della tappa 0.7. La loro geometria la decide solid-space (ADR 0030),
con le richieste [#326](https://github.com/Matleyx78/tecnigrafo/issues/326)
(intersezioni), [#327](https://github.com/Matleyx78/tecnigrafo/issues/327)
(raccordi), [#328](https://github.com/Matleyx78/tecnigrafo/issues/328) (offset) e
[#329](https://github.com/Matleyx78/tecnigrafo/issues/329) (tolleranze). Restano a
Tecnigrafo il comportamento dei comandi (domande, opzioni, che cosa si tiene e
che cosa si toglie, messaggi) e che cosa si garantisce a chi li usa. La scheda
della tappa chiede di scriverlo, perché `OFFSET` e `TRIM` su polilinee complesse
sono notoriamente difficili.

I DXF di prova preparati in AutoCAD LT
([tappe/0.7/dxf-di-prova.md](../tappe/0.7/dxf-di-prova.md)) fissano il
comportamento di AutoCAD in una trentina di casi. Mostrano anche che negli script
alcuni di questi comandi usano sequenze vecchie (ADR 0032).

## Decisione

### `TRIM` ed `EXTEND`

1. **Due modi, come AutoCAD**, scelti con `TRIMEXTENDMODE` (valore della
   sessione, 1 all'avvio) o con l'opzione *mOde* dentro il comando:
   - **rapido** (1): tutte le curve visibili fanno da bordo; non si scelgono
     bordi;
   - **standard** (0): prima si scelgono i bordi (`Select objects or <select
     all>:`, e Invio li sceglie tutti), poi gli oggetti da tagliare o prolungare.

   Negli script vale sempre il modo standard (ADR 0032, punto 8).
2. **Bordi.** Fanno da bordo linee, archi, cerchi e polilinee sui layer visibili,
   anche bloccati; il testo no. **Una polilinea fa da bordo anche a se stessa, e
   i suoi vertici delimitano i pezzi**: AutoCAD taglia un rettangolo da un vertice
   all'altro anche se una linea lo attraversa più lontano (`autocad-taglio.dxf`,
   casi d ed e).
3. **`TRIM`** toglie il pezzo dell'oggetto scelto compreso tra i due bordi più
   vicini al punto del clic.
   - Un cerchio tagliato diventa un arco con lo stesso handle (ADR 0031, punto 5).
   - Un arco o una linea tagliati in mezzo diventano due entità: il primo pezzo
     tiene l'handle.
   - Una polilinea chiusa tagliata diventa aperta.
   - Nel modo rapido un oggetto senza bordi si cancella; nel modo standard
     compare il messaggio di AutoCAD che non interseca il bordo.
   - Il testo non si taglia.
4. **`EXTEND`** prolunga l'estremo più vicino al clic fino al primo bordo che
   incontra: le linee lungo la retta, gli archi lungo il loro cerchio, le
   polilinee aperte con il loro primo o ultimo segmento. Cerchi, polilinee
   chiuse e testo non si prolungano. Senza un bordo da raggiungere, l'entità
   resta com'è.
5. **`Shift`.** Con `Shift` premuto il clic fa l'altro comando: in `TRIM`
   prolunga, in `EXTEND` taglia.
6. **Opzioni:** *mOde* e *Undo*. Restano fuori *cuTting edges*/*Boundary edges*,
   *Crossing*, *Fence*, *Project*, *Edge* ed *eRase*, che quindi non compaiono
   nelle domande. Un clic nel vuoto non apre un rettangolo (ADR 0032, punto 1).

### `OFFSET`

7. **Domande e opzioni come AutoCAD:** distanza o *Through*, poi *Erase* e
   *Layer* (*Source* o *Current*); nel ciclo *Exit*, *Multiple* e *Undo*. La
   copia è un'entità nuova (ADR 0031, punto 6); con *Layer Current* prende il
   layer corrente.
8. **Giunzioni** come AutoCAD con `OFFSETGAPTYPE` = 0 (`autocad-offset.dxf`):
   - due segmenti consecutivi si prolungano fino a incontrarsi;
   - se i prolungamenti non si incontrano, la giunzione è un arco di raggio pari
     alla distanza, con il centro nel vertice;
   - un segmento curvo che con quella distanza si annulla sparisce, e le giunzioni
     accanto si chiudono con lo stesso arco.
9. **Che cosa si garantisce:**
   - linee, archi e cerchi: sempre. Un arco o un cerchio che si annullerebbe dà il
     messaggio di AutoCAD e nessuna copia;
   - polilinee, nei **casi semplici**: nessun segmento sparisce e il risultato non
     si interseca. Il risultato è sempre quello di AutoCAD, senza autointersezioni;
   - **negli altri casi**: o un risultato senza autointersezioni, o nessuna copia
     con un messaggio. Mai una copia sbagliata. Nel caso della U troppo stretta
     (`autocad-offset.dxf`, caso f) anche AutoCAD non crea nulla.

### `FILLET` e `CHAMFER`

10. **`FILLET`.** Opzioni *Undo*, *Polyline*, *Radius*, *Trim* e *Multiple*, con
    `Shift` che vale raggio 0. Il comando lavora su linee, archi, cerchi e
    segmenti di polilinee; i risultati seguono `autocad-raccordi.dxf`:
    - le linee e gli archi si accorciano o si allungano fino all'arco di
      raccordo; i cerchi non si tagliano;
    - con raggio 0 le due entità si incontrano in uno spigolo;
    - due linee parallele si raccordano con un semicerchio, qualunque sia il
      raggio;
    - due segmenti della stessa polilinea ricevono un segmento curvo in mezzo;
      *Polyline* raccorda tutti i vertici;
    - con *Trim No* le entità non cambiano e si aggiunge solo l'arco.

    Fuori: raccordi tra entità diverse da queste e il raccordo di una polilinea
    con un'altra entità.
11. **`CHAMFER`.** Opzioni *Undo*, *Polyline*, *Distance*, *Angle*, *Trim*,
    *mEthod* e *Multiple*. Lavora su linee e segmenti dritti di polilinee.
12. **Proprietà dell'arco di raccordo e della linea di smusso:** quelle della
    prima entità scelta se le due hanno le stesse proprietà, altrimenti quelle
    correnti. I file di prova hanno tutto sul layer 0 e non lo dicono: la regola
    va confermata con una prova in AutoCAD LT prima del pacchetto dei comandi.

### `JOIN`

13. **Interattivo, più oggetti insieme**, come AutoCAD dal 2012
    (`autocad-unione.dxf`):
    - linee allineate diventano una linea, anche con spazi tra loro;
    - archi dello stesso cerchio diventano un arco, anche con spazi;
    - linee, archi e polilinee che si toccano agli estremi diventano una
      polilinea;
    - due estremi distanti 1e-10 si considerano coincidenti, e il vertice comune
      prende il punto dell'entità che segue. Il confine esatto lo dà la regola
      della richiesta #329;
    - entità che non si toccano non si uniscono, e compare il messaggio
      `0 objects joined, <n> objects discarded from the operation.`
14. **Negli script** vale la sequenza vecchia (ADR 0032): prima l'oggetto
    sorgente, poi gli oggetti da unire, accettati secondo il tipo della sorgente.
    Una linea accetta linee allineate, un arco archi dello stesso cerchio (uno per
    clic, con l'opzione *cLose*), una polilinea linee, archi e polilinee contigue.
15. **Identità:** l'entità sorgente resta (ADR 0031, punto 7); con più oggetti
    insieme la sorgente è il primo scelto.

### Per tutti

16. **`Esc`** conserva le operazioni già completate e annulla solo quella in
    corso (ADR 0015).
17. **Testi dei messaggi.** I messaggi che vengono da AutoCAD (taglio senza
    bordo, offset impossibile, unione non riuscita, larghezze perse) si scrivono
    con le parole della versione inglese. Quelli osservati nella prova sono in
    italiano: la versione inglese si controlla prima del pacchetto che li
    introduce.

## Alternative considerate

- **Solo il modo rapido:** gli script `.scr` di AutoCAD con `TRIM` o `EXTEND`
  non funzionerebbero in Tecnigrafo (ADR 0011, punto 3).
- **La polilinea che non fa da bordo a se stessa:** più intuitivo, ma diverso da
  AutoCAD, e uno script darebbe un altro disegno.
- **`OFFSET` che promette il risultato di AutoCAD in tutti i casi:** non si può
  garantire senza sapere che cosa darà solid-space; meglio una promessa piccola e
  vera, e un messaggio onesto negli altri casi.
- **`FILLET` e `CHAMFER` anche su archi di polilinee e su entità miste con
  polilinee:** rimandati, perché AutoCAD stesso li tratta caso per caso.

## Conseguenze

- I comandi stanno in `cmd`; la geometria nelle facciate di `core` sopra
  solid-space (intersezioni, curve parallele, raccordi, unione).
- Il criterio di chiusura su `OFFSET` si legge con il punto 9.
- Due domande restano aperte fino alla prova in AutoCAD LT: proprietà dell'arco
  di raccordo (punto 12) e testi inglesi dei messaggi (punto 17). Se la prova
  smentisce il punto 2 nel modo rapido, lo si corregge prima del pacchetto di
  `TRIM`.
