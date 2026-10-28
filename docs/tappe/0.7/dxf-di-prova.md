# DXF di prova della tappa 0.7

Sei disegni preparati dal responsabile in AutoCAD LT il 2026-10-28, per avere
riferimenti indipendenti da Tecnigrafo sui comandi di modifica. Ogni disegno è
fatto di figure di partenza e dei comandi eseguiti su di esse. Chi scrive i
riferimenti conosce le figure di partenza da questo documento e dagli script, e
legge il risultato di AutoCAD con l'oracolo ezdxf (ADR 0020). I valori attesi
si copiano dal log dell'oracolo, non si calcolano a mano e non si ricavano da
Tecnigrafo.

## I file

Repository [Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file),
licenza CC0, commit
[`4ad9fff`](https://github.com/Matleyx78/dxf-test-file/commit/4ad9fff8a88e65561f3998a56a95adf921b58792).
Tutti in DXF 2018 (`AC1032`), in millimetri (`$INSUNITS` 4, `$MEASUREMENT` 1).

| File | SHA-256 | Riferimenti |
|---|---|---|
| `autocad-specchiatura.dxf` | `5f176996ad6fdd9b8e59291ba0125236afb74574099caba3f5434b8f0011b051` | R4, R6 |
| `autocad-taglio.dxf` | `240fe3ae91e9837ff65865d42aa30e45b5abf242baf8e01e3dd5aed701ed8973` | R16 |
| `autocad-offset.dxf` | `8ffddd1f67c4ea365fb01483edcc948c08aa583136849f8dc071916d9e6d8189` | R17, R18 |
| `autocad-raccordi.dxf` | `aecc6725a0d5b0b76c11eef4c9f93613e7beda91c0a2a225496ea8899118e842` | R19, R20, R21 |
| `autocad-grip.dxf` | `864d277c2be7cd9c58491f140417d5cb9be3e1206fc02b3737a207ac3c3521ed` | R9, R10 |
| `autocad-unione.dxf` | `4df5f4490dc442874c7561e4304e78263bd73a01abcfae5618de15193701dec7` | R22, R23 |

Gli script eseguiti stanno in [dxf-di-prova/](dxf-di-prova/). Ogni disegno è
partito da un disegno nuovo, con snap, orto e input dinamico spenti,
`OSNAPCOORD` 1, `TRIMEXTENDMODE` 1, `TRIMMODE` 1 e `MIRRTEXT` 0 (le prime righe
di ogni script). Negli script AutoCAD usa le sequenze vecchie di alcuni comandi
(ADR 0032): per questo gli script impostano raggi e distanze con le variabili
di sistema, e fanno `TRIM` ed `EXTEND` nel modo standard con tutti gli oggetti
come bordi.

## Che cosa contiene ogni file

Le coordinate sono quelle scritte negli script; qui si riassumono i casi e i
passi fatti a mano.

### `autocad-specchiatura.dxf`

- **Script:** `1a-specchiatura-oggetti.scr`, poi a mano su `Tg10` obliquo 15 e
  fattore di larghezza 0.8, poi `1b-specchiatura-comandi.scr`.
- **Figure:** dieci testi `Tg1`…`Tg10` alti 2.5 con giustificazioni sinistra,
  `C`, `R`, `M`, `TL`, `BR`, `A` (0,60–20,62), `F` (0,70–20,70), rotazione 30
  (`Tg9`), obliquo e larghezza (`Tg10`); l'arco di centro 0,−20, raggio 10, da 0°
  a 120°; la polilinea 0,−40 → 20,−40 → arco tangente fino a 30,−50 → 40,−50, con
  larghezza globale 0.5.
- **Comandi:** sei `MIRROR` degli stessi dodici oggetti, senza cancellare, ognuno
  con il suo asse e il suo `MIRRTEXT`:

  | Specchiatura | `MIRRTEXT` | Asse |
  |---|---|---|
  | M1 | 0 | da 100,−100 a 100,200 |
  | M2 | 1 | da −100,−100 a −100,200 |
  | M3 | 0 | da −100,−100 a 200,−100 |
  | M4 | 1 | da −100,200 a 200,200 |
  | M5 | 0 | da 300,0 a 400,100 |
  | M6 | 1 | da −300,0 a −400,100 |

- **Note per i riferimenti:**
  - con `MIRRTEXT` 1 i punti dei testi sono riflessi esatti: si copiano;
  - con `MIRRTEXT` 0 la rotazione e i segnali si copiano, ma la posizione dipende
    dalle misure del font di AutoCAD. I riferimenti la calcolano con quelle di
    Liberation Sans (ADR 0031, punto 3; domande `font-metrics` e `text-width`
    sono di Tecnigrafo e non si usano: le misure si prendono dal file del font,
    come per i riferimenti della 0.4).

### `autocad-taglio.dxf`

- **Script:** `2-taglio.scr`, tutto nel modo standard con Invio a
  `<select all>`.
- **`TRIM`**, punto del clic tra parentesi:
  - a: croce (15,0);
  - b: cerchio e linea (100,10);
  - c: arco tra due linee (200,10);
  - d: polilinea con arco (350,0);
  - e: rettangolo chiuso (420,0);
  - g: linea tangente a un cerchio (602,10);
  - h: polilinea che si interseca (703,3).

  Il caso f (oggetto senza intersezioni) e il caso m (layer bloccato) sono stati
  tolti dallo script.
- **`EXTEND`:**
  - i: linea (8,200);
  - j: arco (100.8716,209.9619);
  - k: polilinea (210,204);
  - l: linea verso un cerchio (304,200).
- **Che cosa mostra:**
  - il cerchio tagliato diventa un arco con lo stesso handle;
  - l'arco tagliato in due tiene l'handle sul primo pezzo;
  - **una polilinea fa da bordo a se stessa nei suoi vertici**: nei casi d ed e
    il taglio va da vertice a vertice (ADR 0035, punto 2).

### `autocad-offset.dxf`

- **Script:** `3-offset.scr`.
- **Casi** (distanza, lato):
  - a: linea, 5;
  - b: cerchio, 3 dentro e 3 fuori;
  - c: arco, 4 fuori e 4 dentro;
  - d: rettangolo 100×50, 10 dentro e 10 fuori;
  - e: asola, 4 dentro e 4 fuori;
  - f: U larga 6, 2 dentro (riuscito) e 4 dentro (nessun risultato);
  - g: conca con un arco di raggio 3, 2 (arco di raggio 1 e giunzioni ad arco) e
    5 (l'arco sparisce);
  - h: rettangolo a coordinate 1e6, 10 dentro.
- **Che cosa mostra:** le giunzioni ad arco di raggio pari alla distanza con il
  centro nel vertice, dove i prolungamenti non si incontrano (ADR 0035, punto 8).

### `autocad-raccordi.dxf`

- **Script:** `4-raccordi.scr`.
- **`FILLET`:**
  - a: angolo retto, raggio 10;
  - b: linee che non si toccano, raggio 5;
  - c: parallele;
  - d: cerchio e linea, raggio 3;
  - e: arco e linea, raggio 3;
  - f: raggio 0;
  - g: *Polyline* su un rettangolo, raggio 5;
  - h: due segmenti della stessa polilinea, raggio 5;
  - i: `TRIMMODE` 0, raggio 5.
- **`CHAMFER`:**
  - j: distanze 5 e 8;
  - k: lunghezza 6 e angolo 30;
  - l: *Polyline* con distanze 3 e 3.
- **`CIRCLE` *Ttr*:**
  - m: due linee, raggio 8;
  - n: cerchio e linea, raggio 6.

  Riusciti con i punti scritti.

### `autocad-grip.dxf`

- **Script:** `5-grip-oggetti.scr` per le figure; i grip spostati a mano, con le
  coordinate assolute:

  | Oggetto | Grip | Nuovo punto |
  |---|---|---|
  | arco centro 0,0 (0°–90°) | inizio 10,0 | 12,0 |
  | arco centro 30,0 | fine 30,10 | 30,12 |
  | arco centro 60,0 | medio 67.0711,7.0711 | 69,9 |
  | arco centro 90,0 | centro | 95,5 |
  | cerchio 150,0 r10 | quadrante 160,0 | 165,0 |
  | polilinea 200,0 → 220,0 → arco di centro 230,0 → 240,0 → 260,0 | vertice 220,0 | 220,5 |
  | la stessa spostata di 100 | medio del primo segmento 310,0 | 310,5 |
  | la stessa spostata di 200 | medio dell'arco 430,−10 | 430,−15 |
  | linea 500,0–520,0 | medio 510,0 | 515,5 |
  | linea 500,20–520,20 | estremo 520,20 | 525,25 |
  | testo `Tg1` a 600,0 | inserimento | 605,5 |
  | testo `Tg2` *Aligned* 600,20–620,20 | secondo punto | 625,25 |

- **Che cosa mostra:** l'arco e i segmenti curvi delle polilinee passano per il
  loro medio di prima quando si sposta un estremo o un vertice (ADR 0034,
  punto 4).

### `autocad-unione.dxf`

- **Script:** `6-unione.scr` fino al caso c (con il `JOIN` degli script: oggetto
  sorgente, poi gli altri). La versione eseguita si era fermata sul secondo
  arco del caso c, che il responsabile ha scelto a mano con il punto
  75,8.6603; quella qui è corretta così. Il resto è stato fatto a mano, con le
  stesse figure dello script e con il `JOIN` interattivo:
  - e1: linee allineate con spazio 1e-10, due clic;
  - f: polilinea più linea, finestra;
  - d: linea, arco, linea, in due `JOIN` (due oggetti, poi il terzo);
  - e2: estremi a 1e-10 non allineati, finestra;
  - g: non contigue, finestra, non unite.

  Poi `EXPLODE` della polilinea larga a 300,0.
- **Casi a, b, c:** unione con il `JOIN` degli script, che serve ai riferimenti
  della sequenza degli script (R23 con `!scr on`).
- **Messaggi osservati** (in italiano):
  - «1 linea unita all'origine»;
  - «2 linee unite in 1 linea»;
  - «2 oggetti convertiti in 1 polilinea»;
  - «1 segmento unito in 1 polilinea»;
  - «0 oggetti uniti, 2 oggetti scartati dall'operazione»;
  - «L'esplosione di questa polilinea causa la perdita di informazioni sulla
    larghezza».

## Che cosa resta da controllare in AutoCAD LT

Prima dei pacchetti indicati, una prova a mano:

1. **`TRIM` in modo rapido su un rettangolo attraversato da una linea:** il
   taglio va da vertice a vertice come nel modo standard? Serve a P16 (ADR 0035,
   punto 2).
2. **`FILLET` tra due linee su layer diversi:** su quale layer va l'arco? Serve a
   P20 (ADR 0035, punto 12).
3. **I testi inglesi dei messaggi osservati in italiano**, se c'è un AutoCAD in
   inglese a disposizione; altrimenti dalla documentazione di AutoCAD. Servono a
   P5, P16, P18, P20, P21 e P23 (ADR 0035, punto 17).
