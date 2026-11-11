# ADR 0048: Retini

- **Stato:** Accettato
- **Data:** 2026-11-11

## Contesto

La tappa 0.8 porta i retini con contorno scelto o rilevato e alcuni motivi
standard (`ANSI31`, `SOLID`, `NET`), anche in DXF. Decidere quali curve chiudono
una regione, quali isole contiene e quali tratti di una riga stanno dentro sono
decisioni geometriche: spettano a solid-space ([ADR 0030](0030-solid-space-kernel-di-tecnigrafo.md),
punto 2). Le due richieste nascono con il piano della tappa (decisione del
2026-11-07; testi preparati nella [#576](https://github.com/Matleyx78/tecnigrafo/issues/576)).

## Decisione

### 1. Il modello

- `core::Hatch`: nome del motivo (2), riempimento pieno (70), associatività (71,
  conservata e non usata), stile delle isole (75: normale, solo esterno,
  ignora), tipo di motivo (76), angolo (52), scala o passo (41), doppio (77), le
  righe del motivo (78 e 53, 43, 44, 45, 46, 79, 49), i punti interni (98 e
  10/20), e i **contorni** (91): ognuno con i segnali (92) e la forma, polilinea
  (72, 73, 93, vertici con bulge) o lati (93; linee e archi). Gli oggetti di
  contorno associati (97, 330) e il resto stanno in `extras`.
- Restano non gestiti (ADR 0044, punto 4) i retini con lati d'ellisse o spline e
  quelli con sfumatura (450 e seguenti).
- Il contorno **non si disegna**: come in AutoCAD, si vede solo se è
  un'entità a sé.

### 2. Motivi

- Un retino letto si disegna con le **righe del motivo che porta nel file**
  (gruppi 53–49): qualunque motivo, non solo i tre della scheda.
- Per i retini nuovi Tecnigrafo conosce `ANSI31`, `NET` e `SOLID`, con le
  definizioni di `acadiso.pat` in un disegno metrico (`$MEASUREMENT` = 1) e di
  `acad.pat` negli altri. Sono numeri di uno standard; le scrive la scheda.
- I tratti di una riga si misurano dall'origine della sua famiglia (gruppi
  43/44), come in AutoCAD.

### 3. Geometria generata

- `SOLID`: una regione piena (`FilledRegion`, ADR 0044) con gli anelli dei
  contorni. Con lo stile *normale* la regola pari-dispari della regione
  riempie già le isole giuste; con *solo esterno* e *ignora* gli anelli da
  usare li dice solid-space (punto 5).
- Motivo: le righe di ogni famiglia, infinite nel piano, **tagliate sulla
  regione** da solid-space: restano i tratti dentro la regione e fuori dalle
  isole, poi Tecnigrafo applica i tratteggi.
- Prima che solid-space dia il taglio, un retino a motivo letto da un file si
  conserva e si trasforma, ma non si disegna.

### 4. Comportamento

- `MOVE`, `COPY`, `ROTATE`, `SCALE` e `MIRROR` trasformano contorni, punti
  interni, angolo, scala e origini delle righe; le righe si ricalcolano.
  Associatività: nessuna, il retino non segue i contorni.
- **Selezione:** un clic su una riga del motivo o dentro una regione piena; la
  finestra usa la geometria generata. **Snap:** nessuno (in AutoCAD
  `OSNAPHATCH` = 0). **Grip:** nessuno nella 0.8. `EXPLODE` lo rifiuta.

### 5. Che cosa si chiede a solid-space

Due richieste, aperte con il piano:

1. **Contorni e isole:** dato un insieme di curve del piano e un punto, il
   contorno chiuso più piccolo che lo contiene e le isole dentro, con una
   diagnosi quando la regione non è chiusa; dato un insieme di contorni già
   chiusi (i contorni scelti), quali stanno dentro quali.
2. **Parti di una curva dentro una regione:** quali tratti di una riga stanno
   dentro una regione con le sue isole, anche quando la riga passa per un
   vertice, è tangente a un arco o corre lungo un lato.

Tecnigrafo tiene: la scelta delle curve candidate (tutte le curve gestite del
disegno, non solo quelle in vista, così il risultato non dipende dalla vista),
le unità, la traduzione delle entità, il motivo e i tratteggi.

### 6. Comando

`-HATCH` (alias `HATCH`) nella forma da riga di comando di AutoCAD: punto
interno (si possono indicare più punti; `Invio` termina), *Properties* (nome del
motivo, scala, angolo) e *Select objects* (contorni scelti fra cerchi e
polilinee chiuse). Le altre opzioni (*draW boundary*, *remove Boundaries*,
*Advanced*, *DRaw order*, *Origin*, colore e trasparenza) restano fuori.

## Alternative considerate

- **Rilevare i contorni in Tecnigrafo con tolleranze proprie:** è proprio ciò
  che il kernel deve evitare (ADR 0030).
- **Disegnare le righe ritagliate dal pittore** (`QPainter` con un'area di
  ritaglio): si vedrebbero, ma selezione, rapporto e casi di verifica non
  avrebbero geometria.
- **Curve candidate solo in vista**, come AutoCAD: il risultato dipenderebbe
  dallo zoom, anche negli script.

## Conseguenze

- I pacchetti che tagliano le righe e rilevano i contorni hanno la scheda dopo
  il contratto di solid-space.
- Il rischio della scheda (topologia dei contorni) passa a solid-space, che lo
  isola nelle sue richieste.
