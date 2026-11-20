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

Aggiornato alla versione **0.8**.

| Stato | Comandi |
|---|---|
| completo | 20 |
| presente | 36 |
| assente | 26 |

## Disegno

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `LINE` | `L` | completo | 0.4 | segmenti, *Close* e *Undo*; `Invio` alla prima domanda continua dall'ultimo punto (0.7) |
| `PLINE` | `PL` | presente | 0.4 | archi tangenti, *Arc*, *Line*, *CLose* nella 0.4; mancano *Width*, *Halfwidth*, *Length* e le varianti dell'arco (*Angle*, *CEnter*, *Radius*…), dopo la 1.0 (decisione del 2026-10-28) |
| `CIRCLE` | `C` | completo | 0.4 | centro e raggio o diametro, `2P`, `3P` nella 0.4; tangente-tangente-raggio (*Ttr*) nella 0.7; le cinque varianti nel menu del pulsante del ribbon (0.7.5) |
| `ARC` | `A` | presente | 0.4 | tre punti e inizio-centro-fine nella 0.4, anche dal menu del pulsante del ribbon (0.7.5); le altre varianti dopo la 1.0 |
| `RECTANG` | `REC` | assente | 0.8.2 | |
| `POLYGON` | `POL` | assente | 0.8.2 | |
| `ELLIPSE` | `EL` | presente | 0.8 | asse e fine, *Center*, *Rotation*, *Arc*; manca *Isocircle*; manca l'alias `EL` (0.8.1); `TRIM`, `EXTEND`, `OFFSET` e `FILLET` non accettano ellissi (idea #663) |
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
| `TEXT` | `DT`, `DTEXT` | presente | 0.4 | una o più righe, codici speciali DXF; negli script una riga sola, come AutoCAD (0.7); usa lo stile corrente (0.8); mancano le opzioni *Justify* e *Style* |
| `MTEXT` | `T`, `MT` | presente | 0.8 | dalla riga di comando, con *Height*, *Justify*, *Line spacing*, *Rotation*, *Style*, *Width*; i codici di formato si mostrano come testo; mancano l'editor nella finestra e `TEXTEDIT` (idea #662) e gli alias `T` e `MT` (0.8.1) |
| `STYLE` | `ST` | presente | 0.8 | `-STYLE` (anche `STYLE`) dalla riga di comando: font, altezza fissa, larghezza, obliquo, rovesciato e capovolto; manca la finestra degli stili e l'alias `ST` (0.8.1) |
| `TEXTSTYLE` | | completo | 0.8 | stile di testo corrente |
| `DIMLINEAR` | `DLI`, `DIMLIN` | presente | 0.8 | due punti o un oggetto, *Text*, *Angle*, *Horizontal*, *Vertical*, *Rotated*; manca *Mtext* (idea #662) e l'alias `DLI` (0.8.1) |
| `DIMALIGNED` | `DAL`, `DIMALI` | presente | 0.8 | *Text* e *Angle*; manca *Mtext* (idea #662) e l'alias `DAL` (0.8.1) |
| `DIMANGULAR` | `DAN`, `DIMANG` | presente | 0.8 | due linee, arco, cerchio o tre punti, *Text*, *Angle*, *Quadrant*; manca *Mtext* (idea #662) e l'alias `DAN` (0.8.1) |
| `DIMRADIUS` | `DRA`, `DIMRAD` | presente | 0.8 | *Text* e *Angle*; manca *Mtext* (idea #662) e l'alias `DRA` (0.8.1) |
| `DIMDIAMETER` | `DDI`, `DIMDIA` | presente | 0.8 | *Text* e *Angle*; manca *Mtext* (idea #662) e l'alias `DDI` (0.8.1) |
| `DIMSTYLE` | `D` | presente | 0.8 | `-DIMSTYLE` (anche `DIMSTYLE`) con *Save*, *Restore*, *STatus* e `?`, e le variabili di quota come comandi; una quota con adattamento diverso da `ISO-25` non si rigenera; mancano la finestra degli stili, *Variables*, *Apply* e l'alias `D` (0.8.1) |
| `HATCH` | `H` | presente | 0.8 | `-HATCH` (anche `HATCH`) con un punto interno o *Select objects*, motivi `ANSI31`, `SOLID` e `NET`, scala e angolo; non associativo; contorni solo fra linee, archi, cerchi e polilinee; i retini di AutoCAD con altri motivi si leggono e si vedono; manca la finestra e l'alias `H` (0.8.1) |
| `HPISLANDDETECTION` | | completo | 0.8 | *Normal*, *Outer*, *Ignore* |
| `BLOCK` | `B` | presente | 0.8 | `-BLOCK` (anche `BLOCK`): nome, punto base e oggetti, ridefinizione; manca la finestra e l'alias `B` (0.8.1) |
| `INSERT` | `I` | presente | 0.8 | `-INSERT` (anche `INSERT`) di un blocco o di un file DXF, `nome=file`, *Basepoint*, *Scale* (*Corner*, *XYZ*), *Rotate*; attributi ai valori predefiniti, senza domande; mancano la finestra, le domande degli attributi e l'alias `I` (0.8.1) |
| `WBLOCK` | `W` | assente | — | |

## Modifica

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `ERASE` | `E` | presente | 0.3 | selezione scritta con un punto, `W`, `C`, `ALL` (0.7); mancano *Fence*, *WPolygon*, *CPolygon*; *Last* e *Previous* nella 0.8.1 |
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
| `EXPLODE` | `X` | presente | 0.7 | polilinee in linee e archi nella 0.7; inserimenti e quote nella 0.8 (con scala non uniforme cerchi e archi diventano ellissi); non scompone `MTEXT` e retini |
| `JOIN` | `J` | completo | 0.7 | linee allineate, archi dello stesso cerchio (anche in un cerchio), catene di linee, archi e polilinee; sequenza degli script con *cLose* |
| `MIRRTEXT` | | completo | 0.7 | testo specchiato leggibile (0) o riflesso (1) |
| `TRIMEXTENDMODE` | | completo | 0.7 | modo con cui partono `TRIM` ed `EXTEND` |
| `ARRAY` | `AR` | assente | — | da valutare dopo la 0.7 |
| `ALIGN` | `AL` | assente | — | da valutare dopo la 0.7 |
| `STRETCH` | `S` | assente | — | |
| `BREAK` | `BR` | assente | — | |
| `LENGTHEN` | `LEN` | assente | — | |
| `PEDIT` | `PE` | presente | 0.8 | conversione di linee e archi, *Multiple*, *Close*/*Open*, *Join* con distanza ed *Extend*/*Add*/*Both*, *Ltype gen*, *Reverse*, *Undo*; mancano *Width*, *Edit vertex*, *Fit*, *Spline*, *Decurve* |
| `PEDITACCEPT` | | completo | 0.8 | conversione senza domanda |
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
| `LAYER` | `LA` | presente | 0.6 | pannello agganciabile con tabella degli stati, colori, tipi e spessori; nella 0.7.5 barra di icone, ricerca per nome, stati a icone e linguetta quando non è fissato |
| `-LAYER` | `-LA` | presente | 0.6 | dalla riga di comando per gli script; elenchi con virgola e caratteri jolly, opzioni principali |
| `CLAYER` | | completo | 0.6 | imposta il layer corrente da riga di comando |
| `PROPERTIES` | `PR`, `CH`, `MO`, `PROPS` | presente | 0.6 | pannello agganciabile; nella 0.7.5 griglia con *General* modificabile nella cella e *Geometry* in sola lettura, con lo scorrimento dei vertici delle polilinee |
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
| `NEW` | | presente | 0.5 | dalla 0.7.5 un documento nuovo in una scheda nuova, senza chiudere gli altri, come AutoCAD; usa lo scheletro interno (mancano i template DWT) |
| `OPEN` | | presente | 0.5 | apre `.cadx` e DXF (AC1015–AC1032) in una scheda nuova (0.7.5); un file già aperto attiva la sua scheda; supporto DWG dopo la 1.0 |
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
| Snap a oggetto | `F3` | presente | 0.3 | estremo, medio, centro, inserimento nella 0.3; intersezione, perpendicolare, tangente, quadrante nella 0.7; sulle entità della 0.8, anche dentro i blocchi; mancano vicino (idea #665), nodo, prolungamento, parallelo, le intersezioni apparenti e prolungate |
| Tracciamento degli snap a oggetto | `F11` | presente | 0.8 | acquisizione fermandosi sopra un punto, guide orizzontali e verticali, incroci di due guide e con il polare, distanza scritta lungo una guida; mancano le guide polari dai punti acquisiti (`POLARMODE`), `TRACKPATH` e gli snap *Prolungamento* e *Parallelo* |
| Orto | `F8` | completo | 0.3 | |
| Tracciamento polare | `F10` | presente | 0.7 | incremento con `POLARANG`, distanza scritta lungo la semiretta; mancano gli angoli aggiuntivi |
| Selezione: clic, finestra, intersezione | | presente | 0.3 | anche trascinando (0.7); selezione scritta con un punto, `W`, `C`, `ALL` (0.7); mancano *Fence* e i poligoni; *Last* e *Previous* nella 0.8.1 |
| Grip | | presente | 0.7 | stiramento su linee, archi, cerchi, polilinee e testi; mancano le modalità *Move*, *Rotate*, *Scale*, *Mirror*, più grip caldi e il menu dei grip |
| Immissione dinamica | `F12` | presente | 0.7.5 | domanda del comando, coordinate e, dopo il primo punto, lunghezza e angolo vicino al cursore, con `Tab` e `Invio`; manca la variabile `DYNMODE` |
