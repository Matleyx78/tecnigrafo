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

Aggiornato alla versione **0.7.0**.

| Stato | Comandi |
|---|---|
| completo | 16 |
| presente | 24 |
| assente | 39 |

## Disegno

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `LINE` | `L` | completo | 0.4 | segmenti, *Close* e *Undo*; `Invio` alla prima domanda continua dall'ultimo punto (0.7) |
| `PLINE` | `PL` | presente | 0.4 | archi tangenti, *Arc*, *Line*, *CLose* nella 0.4; mancano *Width*, *Halfwidth*, *Length* e le varianti dell'arco (*Angle*, *CEnter*, *Radius*…), dopo la 1.0 (decisione del 2026-10-28) |
| `CIRCLE` | `C` | completo | 0.4 | centro e raggio o diametro, `2P`, `3P` nella 0.4; tangente-tangente-raggio (*Ttr*) nella 0.7 |
| `ARC` | `A` | presente | 0.4 | tre punti e inizio-centro-fine nella 0.4; le altre varianti dopo la 1.0 |
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
| `TEXT` | `DT`, `DTEXT` | presente | 0.4 | una o più righe, codici speciali DXF; negli script una riga sola, come AutoCAD (0.7); senza *Justify* e *Style* (0.8) |
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
| `ERASE` | `E` | presente | 0.3 | selezione scritta con un punto, `W`, `C`, `ALL` (0.7); mancano *Last*, *Previous*, *Fence*, *WPolygon*, *CPolygon* |
| `MOVE` | `M` | completo | 0.7 | punto base e secondo punto, *Displacement* |
| `COPY` | `CO`, `CP` | presente | 0.7 | copie multiple con *Undo* ed *Exit*, *Displacement*; una copia sola negli script; mancano *mOde* e *Array* |
| `ROTATE` | `RO` | completo | 0.7 | angolo scritto o indicato, *Copy*, *Reference* con *Points* |
| `SCALE` | `SC` | completo | 0.7 | fattore scritto o indicato, *Copy*, *Reference* con *Points* |
| `MIRROR` | `MI` | completo | 0.7 | con o senza cancellare gli originali; il testo secondo `MIRRTEXT` |
| `OFFSET` | `O` | completo | 0.7 | distanza o *Through*, *Erase*, *Layer*, *Multiple*, *Undo*; giunzioni come `OFFSETGAPTYPE` 0, senza le varianti 1 e 2 |
| `TRIM` | `TR` | presente | 0.7 | modo rapido e standard (*mOde*, `TRIMEXTENDMODE`), `Shift` per prolungare, *Undo*; mancano *cuTting edges*, *Crossing*, *Fence*, *Project*, *Edge*, *eRase* |
| `EXTEND` | `EX` | presente | 0.7 | modo rapido e standard (*mOde*, `TRIMEXTENDMODE`), `Shift` per tagliare, *Undo*; mancano *Boundary edges*, *Crossing*, *Fence*, *Project*, *Edge* |
| `FILLET` | `F` | presente | 0.7 | linee, archi, cerchi e segmenti dritti di una polilinea; *Radius*, *Trim*, *Polyline*, *Multiple*, *Undo*, `Shift` per lo spigolo; non raccorda una polilinea con un'altra entità né i suoi segmenti curvi |
| `CHAMFER` | `CHA` | presente | 0.7 | linee e segmenti dritti di una polilinea; *Distance*, *Angle*, *mEthod*, *Trim*, *Polyline*, *Multiple*, *Undo*, `Shift` per lo spigolo; non smussa una polilinea con un'altra entità |
| `EXPLODE` | `X` | presente | 0.7 | polilinee in linee e archi nella 0.7; blocchi nella 0.8 |
| `JOIN` | `J` | presente | 0.7 | linee allineate, archi dello stesso cerchio (anche in un cerchio), catene di linee, archi e polilinee; sequenza degli script con *cLose*; manca l'alias `J` |
| `MIRRTEXT` | | completo | 0.7 | testo specchiato leggibile (0) o riflesso (1) |
| `TRIMEXTENDMODE` | | completo | 0.7 | modo con cui partono `TRIM` ed `EXTEND` |
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
| `LAYER` | `LA` | presente | 0.6 | pannello agganciabile con tabella degli stati, colori, tipi e spessori |
| `-LAYER` | `-LA` | presente | 0.6 | dalla riga di comando per gli script; elenchi con virgola e caratteri jolly, opzioni principali |
| `CLAYER` | | completo | 0.6 | imposta il layer corrente da riga di comando |
| `PROPERTIES` | `PR`, `CH`, `MO`, `PROPS` | presente | 0.6 | pannello agganciabile con sezione generale e geometria in sola lettura |
| `CHPROP` | | presente | 0.6 | colore, layer, tipo, scala del tipo, spessore; mancano Thickness e Transparency (0.9) |
| `COLOR` | `COL`, `COLOUR`, `-COLOR` | presente | 0.6 | indice AutoCAD, nomi standard e Truecolor RGB; niente cataloghi di colori |
| `QSELECT` | | presente | 0.6 | finestra di dialogo per filtrare per tipo e proprietà |
| `LINETYPE` | `LT`, `LTYPE`, `-LT`, `-LINETYPE` | presente | 0.6 | tipi continui e tipi ISO standard (`ACAD_ISO02W100`, `ACAD_ISO10W100`); caricamento ed elenco; niente file `.lin` personalizzati |
| `LTSCALE` | `LTS` | completo | 0.6 | scala globale dei tipi di linea |
| `CELTSCALE` | | completo | 0.6 | scala del tipo di linea per le nuove entità |
| `LWEIGHT` | `LW`, `LINEWEIGHT`, `-LWEIGHT` | presente | 0.6 | spessore corrente con valori metrici standard o ByLayer/ByBlock |
| `LWDISPLAY` | | completo | 0.6 | visualizzazione a schermo, anche con il pulsante `LWT` |
| `RIBBON` / `RIBBONCLOSE` | | completo | 0.6 | passaggio immediato tra barra multifunzione e interfaccia classica a menu e barre |

## File, stampa e vista

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `NEW` | | presente | 0.5 | azzera il documento e la cronologia; usa lo scheletro interno (mancano i template DWT) |
| `OPEN` | | presente | 0.5 | apre `.cadx` e DXF (AC1015–AC1032); supporto DWG dopo la 1.0 |
| `QSAVE` / `SAVE` | | presente | 0.5 | salvataggio rapido atomico nel file corrente o salvataggio di una copia; formato `.cadx` e DXF |
| `SAVEAS` | | presente | 0.5 | salva con nuovo nome in formato `.cadx` e DXF R2018 |
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
| Snap a oggetto | `F3` | presente | 0.3 | estremo, medio, centro, inserimento nella 0.3; intersezione, perpendicolare, tangente, quadrante nella 0.7; mancano vicino, nodo, prolungamento, parallelo, le intersezioni apparenti e prolungate e il tracciamento degli snap |
| Orto | `F8` | completo | 0.3 | |
| Tracciamento polare | `F10` | presente | 0.7 | incremento con `POLARANG`, distanza scritta lungo la semiretta; mancano gli angoli aggiuntivi |
| Selezione: clic, finestra, intersezione | | presente | 0.3 | anche trascinando (0.7); selezione scritta con un punto, `W`, `C`, `ALL` (0.7); mancano *Fence*, i poligoni, *Last* e *Previous* |
| Grip | | presente | 0.7 | stiramento su linee, archi, cerchi, polilinee e testi; mancano le modalità *Move*, *Rotate*, *Scale*, *Mirror*, più grip caldi e il menu dei grip |
