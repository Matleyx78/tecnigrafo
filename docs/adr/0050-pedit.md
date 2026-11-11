# ADR 0050: `PEDIT`

- **Stato:** Accettato
- **Data:** 2026-11-11

## Contesto

`PEDIT` è uno dei comandi di AutoCAD che il responsabile usa di più
([#535](https://github.com/Matleyx78/tecnigrafo/issues/535)), soprattutto per
unire più oggetti in una polilinea. Il perimetro deciso il 2026-11-07, da
confermare nel piano: conversione di linee e archi in polilinea
(`PEDITACCEPT`), *Join* con distanza di tolleranza e tipo di unione *Extend*,
*Add*, *Both*, e le opzioni *Close*, *Open*, *Reverse*, *Ltype gen* e *Undo*.
Fuori *Width*, *Edit vertex*, *Fit*, *Spline* e *Decurve*: le larghezze non si
disegnano ancora (DECISIONI.md, 2026-10-29).

`JOIN` ([ADR 0035](0035-taglio-offset-raccordi-unione.md), 0.7-P22/P23) unisce
già entità che si toccano, con la regola delle tolleranze di solid-space; il
prolungamento di `EXTEND` usa le intersezioni di solid-space sulle curve estese.
Il piano doveva decidere se serve un ADR o un punto nuovo dell'ADR 0035, che è
accettato e non si modifica: serve questo.

## Decisione

1. **Perimetro.** Quello del 2026-11-07, più l'opzione ***Multiple***, che è il
   modo in cui si uniscono più oggetti senza sceglierne prima uno.
2. **Conversione.** Una linea o un arco scelti diventano una polilinea di un
   segmento, che prende handle nuovo, proprietà e dati dell'origine (ADR 0031,
   punto 7). Con `PEDITACCEPT` = 0 (predefinito) il comando chiede prima
   `Do you want to turn it into one? <Y>`; con 1 converte senza chiedere.
   `PEDITACCEPT` è anche un comando, come `MIRRTEXT`.
3. **Opzioni** di una polilinea: *Close* aggiunge il segmento di chiusura
   (dritto) e *Open* lo toglie; *Reverse* inverte l'ordine dei vertici (i bulge
   cambiano segno e passano al segmento giusto, le larghezze iniziale e finale
   si scambiano); *Ltype gen* accende o spegne la generazione continua del tipo
   di linea (70 bit 128); *Undo* annulla l'ultima opzione dentro il comando.
   La richiesta mostra solo le opzioni che Tecnigrafo ha; le altre parole
   chiave di AutoCAD danno il messaggio delle parole chiave non valide.
4. **Unione con distanza.** *Join* chiede gli oggetti; in *Multiple* chiede
   anche la distanza di tolleranza (`fuzz`, 0 per difetto) e il tipo
   (*Jointype*: *Extend* per difetto, *Add*, *Both*).
   - **Distanza 0:** come `JOIN`, con la stessa regola delle tolleranze.
   - **Coppie.** Partendo dalle due estremità della polilinea (o, in
     *Multiple*, di ogni polilinea nell'ordine della selezione), si unisce
     l'oggetto con l'estremo più vicino entro la distanza; a pari distanza vale
     l'ordine della selezione; si continua finché nessun estremo è entro la
     distanza.
   - ***Extend*:** i due segmenti che si affrontano si prolungano o si
     accorciano fino alla loro intersezione, calcolata da solid-space sulle
     curve estese (rette e cerchi interi), quella più vicina ai due estremi.
     Senza intersezione la coppia non si unisce.
   - ***Add*:** fra i due estremi si aggiunge un segmento dritto.
   - ***Both*:** *Extend* se c'è l'intersezione, altrimenti *Add*.
   - I segmenti aggiunti hanno larghezza zero. Quando l'ultima unione richiude
     la polilinea sul suo primo vertice, la polilinea diventa chiusa.
   - Messaggio finale come AutoCAD: `<n> segments added to polyline`.
5. **Annullamento.** Tutto il comando è un passo; *Undo* torna indietro di una
   opzione alla volta dentro il comando (ADR 0031, «Annullamento dentro un
   comando»).
6. **Messaggi e sequenze** dalla cronologia di AutoCAD LT, che il responsabile
   copia nel repository dei DXF di prova (piano, «Prerequisiti»), soprattutto per
   *Extend* con gli archi, che la documentazione di AutoCAD non descrive.

## Alternative considerate

- **Un punto nuovo dell'ADR 0035:** è accettato e non si modifica; un ADR breve
  ne riusa le regole.
- **Mostrare tutte le opzioni di AutoCAD e rispondere «non disponibile»:** gli
  script di AutoCAD che le usano non funzionerebbero comunque, e la richiesta
  prometterebbe ciò che non c'è.
- **Lasciare fuori *Multiple*:** è proprio il caso d'uso della #535.

## Conseguenze

- `core` ha un'unione con distanza accanto a `join`; il comando sta in `cmd`.
- Il catalogo di parità registra le opzioni rimaste fuori.
