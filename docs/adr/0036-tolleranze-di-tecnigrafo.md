# ADR 0036: Tolleranze del lato di Tecnigrafo

- **Stato:** Accettato
- **Data:** 2026-10-29
- **Corretto:** 2026-11-03 — operazione snellimento
  ([#478](https://github.com/Matleyx78/tecnigrafo/issues/478);
  [DECISIONI.md](../DECISIONI.md), stessa data): dove questo ADR descriveva il
  contratto di solid-space (la regola, la grandezza M, gli errori delle
  costruzioni e `isDegenerate`, nel contesto e nei punti 4 e 5), rimanda alle
  sue intestazioni e ai suoi ADR. Nel punto 5 il rimando sostituisce solo la
  descrizione. Dove la descrizione era già diversa dal contratto vale il
  contratto: la grandezza M (`magnitude()`) e il dominio degli errori (ADR
  0013 di solid-space, punto 2) nel contesto; nel punto 4 l'errore di un
  valore costruito, che dipende anche dalla sua grandezza, e gli esiti liberi
  nelle fasce, che un caso non impone (ADR 0013, punti 4 e 5; revisione della
  [#479](https://github.com/Matleyx78/tecnigrafo/pull/479), R479-1 e R479-2).

## Contesto

L'ADR 0030 (punto 2) divide le tolleranze in due parti. La politica delle
tolleranze geometriche (quando due punti coincidono, quando due curve sono
allineate o un pezzo è degenere) spetta a solid-space. Restano a Tecnigrafo le
tolleranze in pixel, quelle con cui i casi di verifica confrontano i numeri, il
trattamento dei valori che l'utente scrive e la traduzione delle unità del
disegno.

solid-space ha risposto alla richiesta
[Matleyx78/solid-space#23](https://github.com/Matleyx78/solid-space/issues/23)
con l'[ADR 0009 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0009-regola-delle-tolleranze-del-piano.md):
una regola sola, `epsilon(M)`, scritta in `robust/tolerance.hpp`
(`TolerancePolicy`), con la grandezza M dei dati di `magnitude()` in
`geom2d/curves.hpp`. Tecnigrafo le dà la lunghezza della sua unità in metri,
ricavata da `INSUNITS`. Le decisioni esatte di solid-space non usano mai la
regola, che si usa solo con le funzioni che la nominano (ADR 0009 di
solid-space, punto 1). Gli errori delle costruzioni sono le costanti delle sue
intestazioni (`kRelativePointError` e `kRelativeStationError` in `curves.hpp`,
`kRelativeArcError` in `tangency.hpp`, `kRelativeOffsetError` in `offset.hpp`),
con il dominio dell'[ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
punto 2. Le intestazioni valgono nella versione fissata dal sottomodulo (ADR
0030, punto 9).

Oggi le regole di Tecnigrafo sono sparse:

- in pixel, negli ADR 0008, 0033 e 0034: scelta 3, snap 10, grip 10, polare 10;
- nel confronto dei casi, `1e-9` assoluto, cambiabile con `!tolerance` (ADR
  0011, punto 6);
- nelle soglie di `core` (ADR 0030, conseguenze).

La scheda della tappa 0.7 chiede di raccoglierle in un ADR, dopo la valutazione
di solid-space, che ora c'è.

## Decisione

1. **L'unità del disegno in metri.** Tecnigrafo traduce `INSUNITS` (ADR 0021)
   nella lunghezza dell'unità in metri, il `double` più vicino al valore
   esatto, e la dà a solid-space (`TolerancePolicy::forUnit`). `INSUNITS` 0,
   senza unità, vale come i millimetri, come già fa Tecnigrafo e come dice
   l'ADR 0009 di solid-space.

   | `INSUNITS` | Unità | Metri |
   |---|---|---|
   | 0 | senza unità (come i millimetri) | 0.001 |
   | 1 | pollici | 0.0254 |
   | 2 | piedi | 0.3048 |
   | 3 | miglia | 1609.344 |
   | 4 | millimetri | 0.001 |
   | 5 | centimetri | 0.01 |
   | 6 | metri | 1 |
   | 7 | chilometri | 1000 |
   | 8 | micropollici | 2.54e-8 |
   | 9 | mil | 2.54e-5 |
   | 10 | iarde | 0.9144 |
   | 11 | ångström | 1e-10 |
   | 12 | nanometri | 1e-9 |
   | 13 | micron | 1e-6 |
   | 14 | decimetri | 0.1 |
   | 15 | decametri | 10 |
   | 16 | ettometri | 100 |
   | 17 | gigametri | 1e9 |
   | 18 | unità astronomiche | 149597870700 |
   | 19 | anni luce | 9460730472580800 |
   | 20 | parsec | 3.0856775814913673e16 |
   | 21 | piedi topografici US | 1200/3937 |
   | 22 | pollici topografici US | 100/3937 |
   | 23 | iarde topografiche US | 3600/3937 |
   | 24 | miglia topografiche US | 6336000/3937 |

   Unità astronomica e anno luce sono le definizioni esatte dell'IAU; il parsec
   è 648000/π unità astronomiche; le unità topografiche US sono le frazioni
   esatte della loro definizione. Una regola vale per il documento con le unità
   che ha nel momento in cui il comando la usa: cambiare `INSUNITS` cambia le
   decisioni che seguono, non le entità già disegnate.
2. **La regola è quella di solid-space.** Tecnigrafo non scrive soglie
   geometriche proprie per le decisioni nuove. Nella tappa 0.7 la regola
   serve a:
   - togliere i pezzi degeneri dopo `TRIM` (ADR 0035, punto 5);
   - il controllo finale della curva parallela delle polilinee (`OFFSET`, ADR
     0035, punti 11 e 12);
   - riconoscere un arco di raccordo degenere (`FILLET`, ADR 0035, punto 13);
   - le coincidenze, gli allineamenti e lo stesso cerchio di `JOIN` (ADR 0035,
     punti 17 e 18).

   Le decisioni esatte (si intersecano, da che parte, in che ordine) non la
   usano mai.
3. **Pixel.** Le tolleranze in pixel si raccolgono qui, con i loro valori e il
   loro scopo:

   | Tolleranza | Pixel | Dove |
   |---|---|---|
   | scelta di un'entità con un clic | 3 | ADR 0008 |
   | apertura dello snap | 10 | ADR 0008, 0033 |
   | lato del quadrato dei grip | 10 | ADR 0034 |
   | aggancio del polare | 10 | ADR 0033 |
   | trascinamento che chiude la finestra di selezione | 5 | tappa 0.7, idea #61 |

   Si cambiano solo con un ADR, perché i casi di verifica dipendono da loro
   attraverso `!view`. Non sono tolleranze geometriche: dicono che cosa ha
   indicato l'utente sullo schermo.
4. **Confronto dei casi.** Il confronto assoluto `1e-9` resta per tutto ciò che
   Tecnigrafo calcola da sé.
   - Per i numeri costruiti da solid-space (punti d'intersezione e di tangenza,
     estremi tagliati, archi di raccordo, curve parallele) vale l'errore che
     solid-space dichiara, con le formule delle costanti del contesto: dipende
     dalla grandezza G, la maggiore fra la grandezza M dei dati e quella dei
     valori costruiti che il caso confronta (un'intersezione può cadere molto
     più lontano dei dati), e sotto 2^-50 non c'è (ADR 0013 di solid-space,
     punto 2). Fino a G = 7e4 circa il valore predefinito basta già
     (2^-46 · 7e4 ≈ 9.9e-10); sopra, il caso imposta `!tolerance absolute`
     all'errore dichiarato, arrotondato per eccesso alla prima cifra (per
     G = 1e6: `!tolerance absolute 2e-8`).
   - Le decisioni (`yes`/`no`, posizioni, lati, esiti) si confrontano esatte,
     tranne gli esiti che il contratto lascia liberi in una fascia: un caso
     non mette i suoi dati in una fascia, come l'archivio dei casi di
     solid-space (ADR 0013 di solid-space, punti 4 e 5).
   - I valori geometrici attesi si prendono dall'archivio dei casi di
     solid-space, citando il caso, quando il caso c'è (ADR 0030, punto 14).
5. **Risultati degeneri.** Un pezzo che dopo un taglio, una curva parallela o un
   raccordo la regola trova degenere (`isDegenerate` di
   `geom2d/coincidence.hpp`) non si crea. Che cosa fa il comando al suo posto lo
   dice l'ADR 0035 per ciascuno.
6. **Valori scritti dall'utente.** Un raggio, una distanza o un fattore uguali a
   0, o non finiti, si trattano come in AutoCAD:
   - raggio 0 in `FILLET`: spigolo;
   - distanza 0 in `OFFSET`: rifiutata;
   - fattore 0 in `SCALE`: rifiutato.

   Il confine tra "zero" e "piccolo" per i valori scritti è il confronto esatto
   con 0: il numero scritto dall'utente è un dato, non un risultato da
   arrotondare.
7. **Soglie di `core`.** Le soglie che `core` ha oggi, e che cosa ne è:

   | Soglia | Dove | Che cosa decide | Nella 0.7 |
   |---|---|---|---|
   | `1e-9 · L²` | `arc_geometry.cpp` (`areCollinear`, ADR 0012, punto 4) | tre punti allineati: niente arco o cerchio per tre punti, grip non valido | **resta**; la sostituisce la regola (i due segmenti a–b e b–c allineati per `areAligned`, o uno dei due degenere) con un pacchetto proprio dopo la tappa 0.7, perché cambia i casi limite di `ARC` e `CIRCLE` per tre punti scritti nella 0.4 |
   | `1e-12` sul bulge | `polyline.cpp` (`PolylineSegment::isArc`) | un segmento di polilinea si disegna e si misura come dritto | **resta**: protegge i calcoli di Tecnigrafo (raggio enorme di un arco quasi dritto); la facciata passa a solid-space quei segmenti con bulge 0 (ADR 0035, punto 1), così si decide sulla curva che si disegna |
   | `32 ε` relativo | `arc_geometry.cpp`, `proximity.cpp` (`containsAngle`, `inRange`, `overlaps`) | un angolo o una proiezione cade dentro un intervallo, nei calcoli di distanza e di snap di Tecnigrafo | **resta**: margine di arrotondamento dei calcoli propri di Tecnigrafo (proiezione, distanza), che l'ADR 0010 di solid-space, punto 9, lascia a lui |
   | `32 ε` relativo | `document_query.cpp` (area di ricerca) | quali entità l'indice spaziale propone | **resta**: allarga la ricerca, poi decide la geometria |
   | `8 ε` | `dashes.cpp`, `lineweight.cpp` | tratti e spessori disegnati | **resta**: disegno |

   Un pacchetto che sostituisce una soglia non deve cambiare i casi di verifica
   esistenti; se li cambia, si ferma e decide il responsabile.

## Alternative considerate

- **Scrivere l'ADR prima della valutazione di solid-space:** fisserebbe numeri
  che solid-space avrebbe potuto cambiare. La scheda della tappa lo esclude.
- **Lasciare le tolleranze sparse negli ADR:** chi scrive un caso di verifica non
  sa dove trovarle.
- **`INSUNITS` 0 come metri, o rifiutato:** un disegno senza unità diventerebbe
  mille volte più tollerante di uno in millimetri, oppure i comandi della 0.7
  non funzionerebbero su un DXF senza unità; Tecnigrafo tratta già 0 come
  millimetri.
- **Sostituire tutte le soglie di `core` nella 0.7:** cambierebbe casi di
  verifica già chiusi della 0.4, in una tappa già lunga; si fa una per volta.
- **Passare a solid-space i segmenti quasi dritti con il loro bulge:**
  solid-space deciderebbe su un arco che Tecnigrafo disegna e misura come una
  linea, e due parti del programma vedrebbero curve diverse.
- **Una tolleranza dei casi relativa a ciascun numero (`!tolerance relative`):**
  l'errore di solid-space è relativo alla grandezza dei dati, non al numero: una
  coordinata vicina a zero in un disegno a coordinate di 1e6 ha lo stesso
  errore assoluto delle altre.

## Conseguenze

- `core` riceve la traduzione di `INSUNITS` e la regola del documento, nella
  facciata di solid-space; le funzioni che la usano la ricevono come argomento.
- Le domande al modello che usano la regola hanno l'argomento `units=`, con 4
  (millimetri, come un disegno nuovo) se manca.
- Le soglie di `core` restano come sono nella 0.7; la soglia dei tre punti
  allineati si sostituisce dopo, con un pacchetto proprio.
- Se il pavimento di un nanometro si rivelasse troppo grande o troppo piccolo per
  un uso reale, è una modifica del contratto di solid-space, con l'etichetta
  `contratto` (ADR 0009 di solid-space, conseguenze).
