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

Aggiornato alla versione **0.8.1**.

| Stato | Comandi |
|---|---|
| completo | 28 |
| presente | 50 |
| assente | 28 |

## Disegno

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `LINE` | `L` | completo | 0.4 | segmenti, *Close* e *Undo*; `Invio` alla prima domanda continua dall'ultimo punto (0.7) |
| `PLINE` | `PL` | presente | 0.4 | archi tangenti, *Arc*, *Line*, *CLose* nella 0.4; mancano *Width*, *Halfwidth*, *Length* e le varianti dell'arco (*Angle*, *CEnter*, *Radius*…), dopo la 1.0 (decisione del 2026-10-28) |
| `CIRCLE` | `C` | completo | 0.4 | centro e raggio o diametro, `2P`, `3P` nella 0.4; tangente-tangente-raggio (*Ttr*) nella 0.7; le cinque varianti nel menu del pulsante del ribbon (0.7.5) |
| `ARC` | `A` | presente | 0.4 | tre punti e inizio-centro-fine nella 0.4, anche dal menu del pulsante del ribbon (0.7.5); le altre varianti dopo la 1.0 |
| `RECTANG` | `REC` | presente | 0.8.1 | polilinea chiusa con *Chamfer*, *Fillet*, *Width*, *Area*, *Dimensions*, *Rotation*, *Elevation* e *Thickness* |
| `POLYGON` | `POL` | presente | 0.8.1 | da 3 a 1024 lati, inscritto, circoscritto o definito da *Edge* |
| `ELLIPSE` | `EL` | presente | 0.8 | asse e fine, *Center*, *Rotation*, *Arc* e alias `EL`; manca *Isocircle*; `TRIM`, `EXTEND`, `OFFSET` e `FILLET` non accettano ellissi (#663, 0.8.2) |
| `SPLINE` | `SPL` | assente | 0.8.2 | con `SPLINEDIT` (#888) |
| `SPLINEDIT` | `SPE` | assente | 0.8.2 | completo, come la `SPLINE` (#888) |
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
| `MTEXT` | `T`, `MT` | presente | 0.8 | editor nella finestra e formati nella 0.8.1; colonne, tabulazioni e rientri restano conservati ma non interpretati |
| `STYLE` | `ST` | presente | 0.8 | `-STYLE` (anche `STYLE`) dalla riga di comando: font, altezza fissa, larghezza, obliquo, rovesciato e capovolto; manca la finestra degli stili |
| `TEXTSTYLE` | | completo | 0.8 | stile di testo corrente |
| `TEXTEDIT` | `ED`, `DDEDIT` | presente | 0.8.1 | testi, `MTEXT` nell'editor con i formati, quote e attributi; negli script usa la riga di comando |
| `DIMLINEAR` | `DLI`, `DIMLIN` | presente | 0.8 | due punti o un oggetto, *Mtext*, *Text*, *Angle*, *Horizontal*, *Vertical*, *Rotated* |
| `DIMALIGNED` | `DAL`, `DIMALI` | presente | 0.8 | *Mtext*, *Text* e *Angle* |
| `DIMANGULAR` | `DAN`, `DIMANG` | presente | 0.8 | due linee, arco, cerchio o tre punti, *Mtext*, *Text*, *Angle*, *Quadrant* |
| `DIMRADIUS` | `DRA`, `DIMRAD` | presente | 0.8 | *Mtext*, *Text* e *Angle* |
| `DIMDIAMETER` | `DDI`, `DIMDIA` | presente | 0.8 | *Mtext*, *Text* e *Angle* |
| `DIMSTYLE` | `D` | presente | 0.8 | `-DIMSTYLE` (anche `DIMSTYLE`) con *Save*, *Restore*, *STatus* e `?`, e le variabili di quota come comandi; una quota con adattamento diverso da `ISO-25` non si rigenera; mancano la finestra degli stili, *Variables* e *Apply* |
| `HATCH` | `H` | presente | 0.8 | `-HATCH` (anche `HATCH`) con un punto interno o *Select objects*, motivi `ANSI31`, `SOLID` e `NET`, scala e angolo; non associativo; contorni solo fra linee, archi, cerchi e polilinee; i retini di AutoCAD con altri motivi si leggono e si vedono; dalla 0.8.1 evidenzia il contorno e le isole prima della conferma; manca la finestra |
| `HPISLANDDETECTION` | | completo | 0.8 | *Normal*, *Outer*, *Ignore* |
| `HATCHEDIT` | `HE` | presente | 0.8.1 | `-HATCHEDIT` con *Properties*, *Origin* e *Style*; mancano contorni, associatività e ordine di disegno |
| `BLOCK` | `B` | presente | 0.8, 0.8.3 | `-BLOCK` (anche `BLOCK`): nome, punto base e oggetti, ridefinizione; la finestra nella 0.8.3 |
| `INSERT` | `I` | presente | 0.8, 0.8.3 | `-INSERT` (anche `INSERT`) di un blocco o file DXF, `nome=file`, *Basepoint*, *Scale* (*Corner*, *XYZ*) e *Rotate*; dalla 0.8.1 chiede gli attributi secondo `ATTREQ`; la finestra, con la libreria di simboli da sfogliare, nella 0.8.3 |
| `ATTDEF` | `ATT` | presente | 0.8.1 | `-ATTDEF` dalla riga di comando; niente attributi multiriga |
| `ATTREQ` | | completo | 0.8.1 | sceglie fra i valori predefiniti e le domande durante l’inserimento |
| `AFLAGS` | | completo | 0.8.1 | maschera dei modi per la prossima definizione di attributo |
| `ATTEDIT` | `ATE` | presente | 0.8.1 | `-ATTEDIT` uno per uno e globale; manca `EATTEDIT` completo |
| `ATTSYNC` | | presente | 0.8.1 | per nome o inserimento; aggiorna tutti gli inserimenti della definizione |
| `BATTMAN` | | presente | 0.8.1 | modifica, riordina, elimina e sincronizza le definizioni; niente estrazione dati |
| `ATTDISP` | | presente | 0.8.1 | *Normal*, *ON* e *OFF* |
| `BEDIT` | `BE` | assente | 0.8.3 | editor dei blocchi |
| `BCLOSE` | | assente | 0.8.3 | chiude l'editor dei blocchi, salvando o scartando |
| `WBLOCK` | `W` | assente | 0.8.3 | |

## Modifica

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `SELECT` | | presente | 0.8.1 | prepara un gruppo richiamabile con *Previous* senza modificare il disegno; mancano *Fence* e i poligoni |
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
| `BREAK` | `BR` | assente | 0.8.2 | |
| `LENGTHEN` | `LEN` | assente | 0.8.2 | |
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
| `PROPERTIES` | `PR`, `CH`, `MO`, `PROPS` | presente | 0.6 | pannello agganciabile; nella 0.7.5 griglia con *General* modificabile nella cella e *Geometry* in sola lettura, con lo scorrimento dei vertici delle polilinee; dalla 0.8.1 mostra tutti i tipi e permette di modificare le proprietà specifiche e i valori degli attributi |
| `CHPROP` | | presente | 0.6 | colore, layer, tipo, scala del tipo, spessore; mancano Thickness e Transparency (0.9) |
| `COLOR` | `COL`, `COLOUR`, `-COLOR` | presente | 0.6 | indice AutoCAD, nomi standard e Truecolor RGB; niente cataloghi di colori |
| `QSELECT` | | presente | 0.6 | finestra di dialogo per filtrare per tipo e proprietà; dalla 0.8.1 riconosce tutti i tipi della 0.8 |
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
| `PURGE` | `PU` | assente | 0.8.3 | dei blocchi; se anche degli altri oggetti con nome, lo decide il piano della 0.8.3 |
| `RENAME` | `REN` | assente | 0.8.3 | dei blocchi; se anche delle altre tabelle, lo decide il piano della 0.8.3 |
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

## Griglia, snap alla griglia e UCS

| Comando | Alias | Stato | Tappa | Note |
|---|---|---|---|---|
| `GRID` | | presente | 0.8.1 | accensione, passo, adattamento allo zoom e suddivisioni; manca la griglia a punti |
| `SNAP` | | presente | 0.8.1 | accensione e passo sui due assi; mancano lo snap polare e quello isometrico |
| `UCSICON` | | presente | 0.8.1 | accesa o spenta, all’origine o nell’angolo; solo UCS 2D |
| `GRIDMODE` | | completo | 0.8.1 | accende o spegne la griglia del disegno |
| `GRIDUNIT` | | completo | 0.8.1 | passo della griglia sugli assi X e Y |
| `GRIDDISPLAY` | | presente | 0.8.1 | gestisce i bit di adattamento e suddivisione |
| `GRIDMAJOR` | | completo | 0.8.1 | intervallo tra le linee principali |
| `SNAPMODE` | | completo | 0.8.1 | accende o spegne lo snap alla griglia del disegno |
| `SNAPUNIT` | | completo | 0.8.1 | passo dello snap sugli assi X e Y |
| `SNAPBASE` | | completo | 0.8.1 | punto base dello snap alla griglia |

## Aiuti al disegno

Non sono comandi da scrivere, ma chi viene da AutoCAD li cerca. Si contano a
parte.

| Aiuto | Tasto | Stato | Tappa | Note |
|---|---|---|---|---|
| Snap a oggetto | `F3` | presente | 0.3 | nella 0.8.1 anche vicino, prolungamento e parallelo, con calamita configurabile; mancano nodo e intersezioni apparenti e prolungate |
| Tracciamento degli snap a oggetto | `F11` | presente | 0.8 | nella 0.8.1 anche guide di prolungamento e parallele; mancano le guide polari dai punti acquisiti (`POLARMODE`) e `TRACKPATH` |
| Orto | `F8` | completo | 0.3 | |
| Griglia | `F7` | presente | 0.8.1 | `GRID`, passo fisso, adattiva e suddivisioni; mancano griglia a punti e rotazione con `SNAPANG` |
| Snap alla griglia | `F9` | presente | 0.8.1 | `SNAP`, passo e base; stile isometrico e `SNAPANG` conservati ma senza effetto; manca lo snap polare |
| Icona dell'UCS | | presente | 0.8.1 | `UCSICON` accesa o spenta, all'origine o nell'angolo; solo UCS 2D |
| Tracciamento polare | `F10` | presente | 0.7 | incremento con `POLARANG`, distanza scritta lungo la semiretta; mancano gli angoli aggiuntivi |
| Selezione: clic, finestra, intersezione | | presente | 0.3 | anche trascinando, con `W`, `C`, `ALL`, *Last* e *Previous*; mancano *Fence* e i poligoni |
| Grip | | presente | 0.7 | stiramento su linee, archi, cerchi, polilinee e testi; dalla 0.8.1 il pannello Proprietà evidenzia il vertice corrente di polilinee e SOLID; mancano le modalità *Move*, *Rotate*, *Scale*, *Mirror*, più grip caldi e il menu dei grip |
| Immissione dinamica | `F12` | presente | 0.7.5 | domanda, coordinate, lunghezza e angolo, con `Tab`, `Invio` e prefisso assoluto `#` nella 0.8.1; manca `DYNMODE` |
