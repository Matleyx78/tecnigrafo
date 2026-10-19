# Catalogo di parità: comandi

Quanto manca ad AutoCAD, comando per comando. Le tappe dicono cosa si fa; questo
catalogo dice quanto resta al lavoro reale.

## Regole

- **Stato:**
  - *assente*: il comando non esiste;
  - *presente*: esiste, ma mancano opzioni o varianti che chi viene da AutoCAD
    usa (sono elencate nelle note);
  - *completo*: copre l'uso normale di AutoCAD per il disegno 2D.
- **Tappa:** dove la roadmap lo prevede (`docs/tappe/`); *—* se non è ancora
  previsto.
- Nomi e alias sono quelli che si scrivono nella riga di comando di AutoCAD, così
  gli script `.scr` restano validi (ADR 0011, punto 3). Le note sono scritte da
  noi, da ciò che fanno i comandi: non si copiano testi di Autodesk.
- **Si aggiorna alla chiusura di ogni tappa**, nella pull request di chiusura
  ([LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md), sezione 8), e quando
  una tappa nuova prevede un comando.
- Si elencano i comandi del disegno 2D di uso quotidiano. Un comando che manca si
  aggiunge con una riga; non serve un'idea.

## Riepilogo

Aggiornato alla versione **0.3.1**.

| Stato | Comandi |
|---|---|
| completo | 2 |
| presente | 3 |
| assente | 64 |

## Disegno

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `LINE` | `L` | assente | 0.4 | `Invio` alla prima domanda per continuare dall'ultimo punto: 0.7 |
| `PLINE` | `PL` | presente | 0.4 | archi tangenti, *Arc*, *Line*, *CLose* nella 0.4; mancano *Width*, *Halfwidth*, *Length* e le varianti dell'arco (*Angle*, *CEnter*, *Radius*…), da valutare nella 0.7 |
| `CIRCLE` | `C` | assente | 0.4 | centro e raggio o diametro, `2P`, `3P` nella 0.4; tangente-tangente-raggio nella 0.7 |
| `ARC` | `A` | assente | 0.4 | tre punti e inizio-centro-fine nella 0.4; le altre varianti dopo la 1.0 |
| `RECTANG` | `REC` | assente | — | |
| `POLYGON` | `POL` | assente | — | |
| `ELLIPSE` | `EL` | assente | — | ellissi dopo la 1.0 |
| `SPLINE` | `SPL` | assente | — | spline dopo la 1.0 |
| `POINT` | `PO` | assente | — | |
| `XLINE` | `XL` | assente | — | |
| `RAY` | | assente | — | |
| `DONUT` | `DO` | assente | — | |
| `REVCLOUD` | | assente | — | |
| `DIVIDE` | `DIV` | assente | — | |
| `MEASURE` | `ME` | assente | — | |

## Testo, quote, retini e blocchi

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `TEXT` | `DT`, `DTEXT` | assente | 0.4 | una riga, senza *Justify* e *Style* (0.8) |
| `MTEXT` | `T`, `MT` | assente | 0.8 | |
| `STYLE` | `ST` | assente | 0.8 | stili di testo |
| `DIMLINEAR` | `DLI` | assente | 0.8 | |
| `DIMALIGNED` | `DAL` | assente | 0.8 | |
| `DIMANGULAR` | `DAN` | assente | 0.8 | |
| `DIMRADIUS` | `DRA` | assente | 0.8 | |
| `DIMDIAMETER` | `DDI` | assente | 0.8 | |
| `DIMSTYLE` | `D` | assente | 0.8 | stili di quota essenziali |
| `HATCH` | `H` | assente | 0.8 | motivi ANSI31, SOLID, NET |
| `BLOCK` | `B` | assente | 0.8 | |
| `INSERT` | `I` | assente | 0.8 | |
| `WBLOCK` | `W` | assente | — | |

## Modifica

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `ERASE` | `E` | presente | 0.3 | mancano i modi di selezione scritti (*Last*, *Previous*, *All*, *Fence*, *WPolygon*, *CPolygon*) |
| `MOVE` | `M` | assente | 0.7 | |
| `COPY` | `CO`, `CP` | assente | 0.7 | |
| `ROTATE` | `RO` | assente | 0.7 | |
| `SCALE` | `SC` | assente | 0.7 | |
| `MIRROR` | `MI` | assente | 0.7 | |
| `OFFSET` | `O` | assente | 0.7 | |
| `TRIM` | `TR` | assente | 0.7 | |
| `EXTEND` | `EX` | assente | 0.7 | |
| `FILLET` | `F` | assente | 0.7 | |
| `CHAMFER` | `CHA` | assente | 0.7 | |
| `EXPLODE` | `X` | assente | 0.7 | polilinee nella 0.7, blocchi nella 0.8 |
| `JOIN` | `J` | assente | 0.7 | |
| `ARRAY` | `AR` | assente | — | da valutare dopo la 0.7 |
| `ALIGN` | `AL` | assente | — | da valutare dopo la 0.7 |
| `STRETCH` | `S` | assente | — | |
| `BREAK` | `BR` | assente | — | |
| `LENGTHEN` | `LEN` | assente | — | |
| `PEDIT` | `PE` | assente | — | |
| `MATCHPROP` | `MA` | assente | — | |

## Annullamento

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `U` | | completo | 0.2 | |
| `UNDO` | | presente | 0.2 | un passo; mancano il numero di passi e le opzioni *Mark*, *Back*, *BEgin*/*End* |
| `REDO` | | completo | 0.2 | |

## Layer e proprietà

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `LAYER` | `LA` | assente | 0.6 | come pannello agganciabile |
| `PROPERTIES` | `PR`, `CH`, `MO` | assente | 0.6 | come pannello agganciabile |
| `QSELECT` | | assente | 0.6 | selezione per layer e per tipo |
| `LINETYPE` | `LT` | assente | 0.6 | continua, tratteggiata, punto e linea |
| `LTSCALE` | `LTS` | assente | 0.6 | scala globale |

## File, stampa e vista

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `NEW` | | assente | 0.5 | |
| `OPEN` | | assente | 0.5 | |
| `QSAVE` / `SAVE` | | assente | 0.5 | |
| `SAVEAS` | | assente | 0.5 | |
| `PLOT` | | assente | 0.9 | stampa e PDF |
| `PURGE` | `PU` | assente | — | |
| `ZOOM` | `Z` | assente | — | lo zoom con la rotella esiste dalla 0.1; manca il comando con le opzioni (*Extents*, *Window*…) |
| `PAN` | `P` | assente | — | lo spostamento con il mouse esiste dalla 0.1; manca il comando |
| `REGEN` | `RE` | assente | — | |

## Interrogazione

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `DIST` | `DI` | assente | — | |
| `AREA` | `AA` | assente | — | |
| `ID` | | assente | — | |
| `LIST` | `LI` | assente | — | |

## Aiuti al disegno

Non sono comandi da scrivere, ma chi viene da AutoCAD li cerca. Si contano a
parte.

| Aiuto | Tasto | Stato | Tappa | Note |
|---|---|---|---|---|
| Snap a oggetto | `F3` | presente | 0.3 | estremo, medio, centro; inserimento nella 0.4; intersezione, perpendicolare, tangente, quadrante nella 0.7 |
| Orto | `F8` | completo | 0.3 | |
| Tracciamento polare | `F10` | assente | 0.7 | |
| Selezione: clic, finestra, intersezione | | presente | 0.3 | manca la finestra trascinando (idea #61) |
| Grip | | assente | 0.7 | |
