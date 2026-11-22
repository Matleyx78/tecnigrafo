# Il rapporto

Formato del rapporto che la modalità di controllo scrive alla fine di uno
script ([ADR 0011](../adr/0011-modalita-di-controllo.md), punto 4). È un
**contratto**: i casi di verifica lo scrivono a mano in `atteso.txt`, e lo
stesso formato descrive le entità nelle [domande al modello](domande.md). Si
cambia solo aggiungendo, con le regole in fondo; per il resto serve un nuovo
ADR.

Il livello 1 e il livello 2 scrivono lo stesso rapporto per lo stesso script.

## Struttura

```
tecnigrafo-report 1

[report 1]
entities:
  257 LWPOLYLINE closed=yes vertex=0,0 vertex=100,0 vertex=100,50 vertex=0,50
messages:
  Command: PLINE
  Specify start point: 0,0
  Specify next point or [Undo]: 100,0
  Specify next point or [Undo]: @0,50
  Specify next point or [Close/Undo]: 50<90
  Specify next point or [Close/Undo]: C
prompt: Command:
preview:
selection: none
rectangle: none
undo:
  Polyline
redo:
snap: on
ortho: off
pointer: none
active snap: none
statistics:
  LWPOLYLINE 1

[final]
...
```

- La prima riga è la versione del formato: `tecnigrafo-report 1`.
- Poi una **sezione** per ogni `!report` dello script, nell'ordine, intitolate
  `[report 1]`, `[report 2]`…, e sempre per ultima la sezione `[final]` con lo
  stato alla fine dello script. Ogni sezione è preceduta da una riga vuota.
- Le righe di una sezione sono sempre quelle qui sotto, in quest'ordine. Un
  **elenco** è una riga che finisce con `:` seguita da una riga per elemento,
  rientrata di due spazi; un elenco vuoto è la sola riga d'intestazione. Gli
  altri **campi** stanno su una riga, `nome: valore`.
- Nessuna riga ha spazi in fondo, tranne quelli che l'utente ha scritto in
  fondo a una riga di comando, riportati nell'eco.

| Riga | Contenuto |
|---|---|
| `entities:` | le entità del disegno, **in ordine di identificativo**, una per riga: l'identificativo, uno spazio, l'entità nel formato qui sotto. L'identificativo è l'handle in decimale; in un disegno nuovo la prima entità è 257 ([ADR 0023](../adr/0023-layer-e-proprieta.md), punto 1) |
| `messages:` | le righe scritte nella cronologia della riga di comando **dalla sezione precedente** (vedi sotto) |
| `prompt:` | la domanda attuale della riga di comando (`Command:` a riposo) |
| `preview:` | le entità dell'anteprima del comando in corso, senza identificativo, nell'ordine in cui il comando le dà |
| `selection:` | gli identificativi della selezione corrente in ordine crescente, separati da uno spazio, o `none` |
| `rectangle:` | il rettangolo di selezione aperto: `window` o `crossing`, il primo angolo e quello che segue il puntatore (`window -5,-5 50,30`), o `none` |
| `undo:` | i nomi dei passi di annullamento, dal più vecchio: l'ultimo è quello che `U` annulla |
| `redo:` | i nomi dei passi da ripetere: l'ultimo è quello che `REDO` ripete |
| `snap:` | `on` o `off`; se qualche modo dello snap è spento, segue `, modes off:` e i modi spenti (`on, modes off: midpoint center`) |
| `ortho:` | `on` o `off` |
| `drawing:` | solo per un disegno legato a un file: il nome e, con modifiche non salvate, `, modified` ([vedi sotto](#il-disegno)) |
| `units:` | solo se diverso da 4: il valore di `INSUNITS` |
| `layers:` | solo se la tabella dei layer non è quella di una sessione nuova: tutti i layer ([vedi sotto](#layer-tipi-di-linea-e-valori-correnti)) |
| `linetypes:` | solo se ce ne sono: i tipi di linea diversi da `ByBlock`, `ByLayer`, `Continuous` |
| `current:` | solo se almeno uno è diverso da una sessione nuova: i cinque valori correnti |
| `ltscale:` | solo se diverso da 1: `LTSCALE` |
| `lwdisplay:` | solo se acceso: `on` |
| `text styles:`, `text style:`, `dimension styles:`, `dimension style:`, `dimension overrides:`, `blocks:` | solo se diversi da una sessione nuova: gli stili e i blocchi della tappa 0.8 ([vedi sotto](#stili-e-blocchi)) |
| `unhandled:` | solo se ce ne sono: le entità non gestite, per tipo |
| `documents:` | solo con più di un documento aperto: i nomi, nell'ordine in cui si sono aperti, e `(active)` dopo quello attivo ([vedi sotto](#il-disegno)) |
| `pointer:` | l'ultimo punto del disegno in cui è stato portato il puntatore (`!move`, `!click`), o `none` |
| `active snap:` | lo snap su cui si trova il puntatore mentre un comando chiede un punto: il tipo e il punto (`Endpoint 100,50`), o `none` |
| `tracking:` | solo se acceso: `on`, il tracciamento degli snap a oggetto ([vedi sotto](#il-tracciamento-degli-snap-a-oggetto)) |
| `acquired:` | solo se ce ne sono: i punti acquisiti, uno per riga, dal più vecchio |
| `active tracking:` | solo se una guida aggancia il puntatore: la guida o l'incrocio e il punto che dà |
| `statistics:` | per ogni tipo DXF presente nel disegno, in ordine alfabetico, il tipo, uno spazio e il numero di entità (`LWPOLYLINE 2`); i tipi assenti non si scrivono. Dalla tappa 0.8 contano anche gli `ATTRIB` degli `INSERT` ([vedi sotto](#tappa-08-entità-composte)) |

### La cronologia (`messages:`)

È ciò che la riga di comando dell'interfaccia mostra nella sua cronologia:

- per ogni riga normale dello script, l'**eco**: la domanda in quel momento,
  uno spazio e il testo scritto (`Specify start point: 0,0`); per una riga
  vuota la sola domanda (`Specify next point or [Undo]:`);
- i messaggi del programma, nell'ordine in cui arrivano (`Undone: Polyline.`,
  `*Cancel*`, `3 found, 5 total`, `Unknown command: XYZ.`).

Clic, movimenti, `!esc`, `!snap` e `!ortho` non hanno eco. Ogni sezione
riporta solo le righe arrivate dopo la sezione precedente: nessuna riga compare
due volte.

### Lo snap attivo

`active snap:` si scrive ai due livelli, perché i due rapporti siano uguali: al
livello 1 lo calcola il runner dei comandi dal puntatore e dalla scala di
`!view`, esattamente come per il marcatore che il livello 2 disegna. I nomi dei
tipi sono `Endpoint`, `Midpoint`, `Center` e, dalla tappa 0.4 (pacchetto P13),
`Insertion`.

### Il tracciamento degli snap a oggetto

Tre campi della tappa 0.8 ([ADR 0049](../adr/0049-tracciamento-degli-snap-a-oggetto.md),
punto 8), scritti ai due livelli dopo `active snap:` e **solo quando sono
diversi da una sessione nuova**, in quest'ordine:

```
pointer: 8,8
active snap: none
tracking: on
acquired:
  0,0
  18,18
active tracking: Horizontal 8,0 from 0,0
```

- **`tracking:`** `on` se il tracciamento è acceso (`!otrack on`, `F11`).
- **`acquired:`** i punti acquisiti con `!hover`, uno per riga e dal più
  vecchio, al più sette; si azzerano quando la richiesta di punto riceve una
  risposta o il comando finisce.
- **`active tracking:`** che cosa fanno le guide del puntatore mentre un
  comando chiede un punto, se lo snap non vince: `Horizontal x,y from x,y` o
  `Vertical x,y from x,y` per una guida sola (il punto dato e il punto
  acquisito della guida), `Crossing x,y` per l'incrocio di due guide,
  `Polar crossing x,y` per l'incrocio di una guida con la semiretta polare.

### Il disegno

Tre campi della tappa 0.5 ([ADR 0021](../adr/0021-comandi-dei-file.md),
punto 6; [ADR 0018](../adr/0018-conservazione-dati-non-gestiti.md), punto 3).
Come ogni campo nuovo si scrivono **solo quando sono diversi da una sessione
nuova**, dopo `ortho:` e in quest'ordine; mancano tutti e tre nei rapporti
della tappa 0.4, che restano validi.

```
ortho: off
drawing: plan.dxf, modified
units: 6
unhandled: HATCH 2, INSERT 1
pointer: none
```

- **`drawing:`** il nome del file del disegno, come l'ha scritto chi l'ha
  aperto o salvato (`plan.dxf`), seguito da `, modified` se il disegno ha
  modifiche non salvate: la pila di annullamento non è alla posizione
  dell'ultimo salvataggio o dell'apertura, quindi annullare fino a quel
  punto toglie `, modified`. Si
  scrive solo per un disegno **legato a un file**, aperto o salvato almeno
  una volta. Un disegno mai salvato (`Drawing1.cadx`, come quello di una
  sessione nuova o dopo `NEW`) non lo scrive, anche con modifiche: ogni
  rapporto della tappa 0.4 disegna qualcosa e cambierebbe. Un disegno
  salvato come `Drawing1.cadx` lo scrive.
- **`units:`** il valore di `INSUNITS` (da 0 a 24), se diverso da 4,
  millimetri, l'unità di un disegno nuovo.
- **`unhandled:`** le entità che il programma conserva senza gestirle (ADR
  0018, punto 3), per tipo in ordine alfabetico, separate da `, `: il tipo,
  uno spazio e il numero (`HATCH 2, INSERT 1`). Non stanno in `entities:`
  né in `statistics:`, che elencano solo le entità gestite.
- **`documents:`** dalla tappa 0.7.5, quando `NEW` e `OPEN` aprono un documento
  nuovo ([ADR 0041](../adr/0041-documenti-e-ambienti.md); DECISIONI.md,
  2026-11-07): i documenti aperti, con il loro nome come in `drawing:`
  (`Drawing2.cadx` anche se non ha un file), separati da `, `, nell'ordine in
  cui si sono aperti, con ` (active)` dopo quello attivo
  (`documents: Drawing1.cadx, plan.dxf (active)`). Si scrive solo con più di un
  documento aperto; tutti gli altri campi del rapporto descrivono il documento
  attivo. Va dopo `unhandled:`.

### Layer, tipi di linea e valori correnti

Cinque campi della tappa 0.6 ([ADR 0023](../adr/0023-layer-e-proprieta.md),
punti 1, 3 e 7; [ADR 0024](../adr/0024-colori.md), punto 6;
[ADR 0025](../adr/0025-tipi-di-linea-e-spessori.md), punti 3 e 6). Come ogni
campo nuovo si scrivono **solo quando sono diversi da una sessione nuova**,
dopo `units:` e prima di `unhandled:`, in quest'ordine; mancano tutti nei
rapporti delle tappe precedenti, che restano validi.

```
units: 6
layers:
  "0" color=7 linetype="Continuous" lineweight=default
  "Muri esterni" color=1 linetype="ACAD_ISO02W100" lineweight=0.50 off frozen locked noplot
linetypes:
  "ACAD_ISO02W100" pattern=12,-3 description="ISO dash __ __ __"
current: layer="Muri esterni" color=bylayer linetype="ByLayer" ltscale=1 lineweight=bylayer
ltscale: 2
lwdisplay: on
unhandled: HATCH 2
```

- **`layers:`** un elenco con **tutti** i layer, nell'ordine della tabella,
  se le righe non sono quelle di una sessione nuova (il solo layer `0`, come
  nella prima riga dell'esempio). Ogni riga: il nome tra virgolette, poi
  `color=`, `linetype=` (nome tra virgolette) e `lineweight=`, sempre; poi
  gli stati, **solo se attivi** e in quest'ordine: `off` (spento), `frozen`
  (congelato), `locked` (bloccato), `noplot` (non stampabile). Un disegno
  senza tabella dei layer (un file letto prima che `io` legga le tabelle,
  pacchetto 0.6-P10) non scrive il campo.
- **`linetypes:`** un elenco dei tipi di linea diversi da `ByBlock`,
  `ByLayer` e `Continuous`, che ogni disegno ha, nell'ordine della tabella;
  manca se non ce ne sono. Ogni riga: il nome tra virgolette,
  `pattern=` con gli elementi separati da `,` (positivo un tratto, negativo
  uno spazio, `0` un punto, in unità del disegno) o `continuous` se il tipo
  non ne ha, e `description=` tra virgolette, sempre, anche vuota
  (`description=""`).
- **`current:`** i cinque valori con cui nasce un'entità nuova
  (`$CLAYER`, `$CECOLOR`, `$CELTYPE`, `$CELTSCALE`, `$CELWEIGHT`), su una
  riga, **tutti** e in quest'ordine, se almeno uno è diverso da quelli di una
  sessione nuova (`layer="0" color=bylayer linetype="ByLayer" ltscale=1
  lineweight=bylayer`).
- **`ltscale:`** la scala globale dei tipi di linea, `LTSCALE`, se diversa
  da 1.
- **`lwdisplay:`** `on` se gli spessori si vedono (`LWDISPLAY`), spento in
  una sessione nuova.

I valori si scrivono come le [proprietà delle entità](#tappa-06-layer-e-proprietà):
colori con la prima colonna della tabella dell'ADR 0024, punto 6; spessori
in millimetri con due decimali (`0.50`), `0` o `default`; numeri come sotto.
I nomi conservano maiuscole e minuscole come nella tabella.

### Stili e blocchi

Sei campi della tappa 0.8 ([ADR 0045](../adr/0045-blocchi-e-attributi.md),
[ADR 0046](../adr/0046-stili-di-testo-e-testo-multiriga.md),
[ADR 0047](../adr/0047-quote-e-stili-di-quota.md)). Come ogni campo nuovo si
scrivono **solo quando sono diversi da una sessione nuova**, dopo
`lwdisplay:` e prima di `unhandled:`, in quest'ordine; mancano tutti nei
rapporti delle tappe precedenti, che restano validi. Il formato è fissato
qui dal pacchetto PT9; li scrivono i pacchetti che creano i loro campi in
`core` (DECISIONI.md, 2026-11-15): `text styles:` e `text style:` PT11,
`dimension styles:`, `dimension style:` e `dimension overrides:` PT22,
`blocks:` PT16. Finché quei pacchetti non sono uniti, nessun rapporto ha
questi campi.

```
lwdisplay: on
text styles:
  "Note" font="romans.shx" height=0 width-factor=0.8 oblique=0 backwards=no upside-down=no
text style: "Note"
dimension styles:
  "ISO-25"
  "Elettrico" DIMASZ:2 DIMTXT:2.5
dimension style: "Elettrico"
dimension overrides: DIMTXT:3.5
blocks:
  "TG_EL" base=0,0 description="Contatto"
  "TG_EL" 259 LINE start=0,0 end=10,0
unhandled: HATCH 2
```

| Riga | Contenuto |
|---|---|
| `text styles:` | un elenco degli stili di testo diversi da `Standard` di una sessione nuova, nell'ordine della tabella: il nome tra virgolette, poi `font=` (tra virgolette), `height=`, `width-factor=`, `oblique=` (gradi), `backwards=` e `upside-down=` (`yes` o `no`), sempre |
| `text style:` | lo stile di testo corrente, tra virgolette, se diverso da `Standard` |
| `dimension styles:` | un elenco di tutti gli stili di quota, nell'ordine della tabella, se la tabella è diversa da quella di una sessione nuova: il nome tra virgolette, poi le variabili essenziali diverse da quelle di `ISO-25`, `NOME:valore`, separate da uno spazio, in ordine alfabetico |
| `dimension style:` | lo stile di quota corrente, tra virgolette, se diverso da `ISO-25` |
| `dimension overrides:` | le variabili di quota correnti diverse da quelle dello stile corrente, `NOME:valore`, separate da uno spazio, in ordine alfabetico |
| `blocks:` | le definizioni di blocco, nell'ordine della tabella: per ognuna una riga `"NOME" base=x,y`, seguita, solo se diversi dal predefinito, da `explodable=no`, `uniform=yes`, `units=N` e `description="…"`; poi una riga per ogni sua entità, nell'ordine della definizione: il nome del blocco tra virgolette, l'identificativo e l'entità nel formato delle [entità](#entità) |

## Numeri

Ogni numero si scrive con la rappresentazione più breve che, riletta, dà lo
stesso `double`, indipendente dalla lingua del sistema:

- punto decimale, nessun separatore delle migliaia, nessuno `.0` finale:
  `100`, `0.1`, `-2.5`, `0.3333333333333333`;
- notazione esponenziale per gli esponenti decimali sotto −4 o sopra 15, con
  il segno e almeno due cifre: `1e+16`, `1e-05`, `3.061616997868383e-15` (come
  `repr()` di Python, salvo lo `.0` finale);
- `-0` si scrive `0`; infinito e NaN sono `inf`, `-inf`, `nan`.

Chi scrive `atteso.txt` non deve indovinare l'ultima cifra: il
[confronto](casi.md) accetta i numeri entro la tolleranza (`1e-9` per difetto),
e accetta anche `100.0` al posto di `100` o `0.00001` al posto di `1e-05`.

Un **punto** è `x,y`, senza spazi. Gli **angoli** sono in gradi, come nel DXF.

## Entità

Un'entità sta su **una riga**: il tipo DXF, poi le proprietà `nome=valore`
separate da uno spazio, sempre nell'ordine indicato. Valori:

- numeri e punti come sopra;
- `yes` / `no` per le proprietà vere o false;
- testi tra virgolette doppie, con `\"` per le virgolette, `\\` per la barra
  rovesciata e `\u{XX}` per i caratteri di controllo (da U+0000 a U+001F e
  U+007F); ogni altro carattere, accentato o no, si scrive com'è, in UTF-8.
  Nelle definizioni delle domande `\u{…}` accetta qualunque carattere Unicode,
  da una a sei cifre esadecimali (`\u{2300}` è `⌀`).

Le proprietà il cui nome inizia con `~` sono **derivate**: le calcola il
programma, servono a controllarlo e non si rileggono.

### Tappa 0.4

Il formato di tutte le entità della tappa è fissato qui, prima che esistano,
perché i riferimenti si possano scrivere prima del lavoro. Dalla tappa 0.4 il
modulo `script` scrive e legge `LWPOLYLINE`; `LINE`, `CIRCLE` e `ARC` li
aggiunge il pacchetto P6, `TEXT` il pacchetto P13.

**`LWPOLYLINE`** (ADR 0004)

```
LWPOLYLINE closed=no vertex=0,0 vertex=10,0 bulge=0.5 vertex=20,0
```

| Proprietà | DXF | Valore |
|---|---|---|
| `closed` | 70, bit 1 | `yes` o `no` |
| `vertex` | 10/20 | un vertice; si ripete per ogni vertice, nell'ordine |
| `bulge` | 42 | il bulge del segmento che **parte** dal vertice appena scritto; si scrive solo se diverso da 0 |

Una polilinea senza vertici è `LWPOLYLINE closed=no`.

**`LINE`** (ADR 0012)

```
LINE start=0,0 end=10,5
```

| Proprietà | DXF | Valore |
|---|---|---|
| `start` | 10/20 | punto iniziale |
| `end` | 11/21 | punto finale |

**`CIRCLE`** (ADR 0012)

```
CIRCLE center=5,5 radius=7.0710678118654755
```

| Proprietà | DXF | Valore |
|---|---|---|
| `center` | 10/20 | centro |
| `radius` | 40 | raggio, > 0 |

**`ARC`** (ADR 0012)

```
ARC center=0,0 radius=10 start-angle=0 end-angle=90
```

| Proprietà | DXF | Valore |
|---|---|---|
| `center` | 10/20 | centro |
| `radius` | 40 | raggio, > 0 |
| `start-angle` | 50 | angolo iniziale in gradi, in [0, 360) |
| `end-angle` | 51 | angolo finale in gradi, in [0, 360), diverso dall'iniziale |

L'arco va sempre in senso antiorario dall'angolo iniziale a quello finale, come
nel DXF: un arco che attraversa 0° ha l'angolo finale minore dell'iniziale
(`start-angle=300 end-angle=30`).

**`TEXT`** (ADR 0013)

```
TEXT position=0,0 alignment-point=0,0 height=2.5 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="Hello" ~box=0,-0.5;5.5,-0.5;5.5,3;0,3
```

| Proprietà | DXF | Valore |
|---|---|---|
| `position` | 10/20 | primo punto d'allineamento |
| `alignment-point` | 11/21 | secondo punto d'allineamento |
| `height` | 40 | altezza delle maiuscole, > 0 |
| `rotation` | 50 | gradi |
| `width-factor` | 41 | fattore di larghezza |
| `oblique` | 51 | angolo obliquo, gradi |
| `style` | 7 | nome dello stile, tra virgolette |
| `mirror-x` | 71, bit 2 | `yes` o `no` |
| `mirror-y` | 71, bit 4 | `yes` o `no` |
| `horizontal` | 72 | `left`, `center`, `right`, `aligned`, `middle`, `fit` |
| `vertical` | 73 | `baseline`, `bottom`, `middle`, `top` |
| `value` | 1 | il testo come è scritto, codici di controllo compresi (`%%c`), tra virgolette |
| `~box` | — | **derivata**: le immagini di `(0, d·h)`, `(w, d·h)`, `(w, a·h)`, `(0, a·h)`, in quest'ordine, separate da `;`: un parallelogramma secondo l'ADR 0022, punto 5; senso antiorario senza specchiatura, orario con una sola specchiatura |

Per `~box`, `h` e `f` sono altezza e fattore di larghezza usati (ricavati per
`Aligned` e `Fit`), `w = L(valore)·h·f`, e `a`/`d` sono ascendente e discendente.
La trasformazione include allineamento, obliquo, specchiatura e rotazione
([ADR 0022](../adr/0022-allineamenti-obliquo-specchiatura-testo.md)).

`~box` permette di controllare le misure del font: si calcola dalle larghezze
lette dal file del font, con uno strumento indipendente. I numeri dell'esempio
mostrano solo la forma, non sono misure di Liberation Sans.

### Tappa 0.5: quota, spessore, estrusione e larghezze

Proprietà del DXF che il modello conserva senza disegnarle
([ADR 0018](../adr/0018-conservazione-dati-non-gestiti.md), punti 4 e 5).
Come ogni proprietà nuova si scrivono **solo quando sono diverse dal valore
predefinito**, in fondo alla riga prima delle derivate, nello stesso ordine
per tutte le entità che le hanno: prima le quote, poi `thickness`, poi
`extrusion`. Le scrivono e le leggono i pacchetti P11 e P12 della tappa 0.5;
qui se ne fissa il formato.

| Proprietà | Entità | DXF | Predefinito |
|---|---|---|---|
| `width` | `LWPOLYLINE`, subito dopo `closed` | 43 | 0 |
| `start-width`, `end-width` | `LWPOLYLINE`, dopo il `vertex` (e il suo `bulge`) del segmento che parte da quel vertice | 40, 41 | 0 |
| `start-z`, `end-z` | `LINE` | 30, 31 | 0 |
| `z` | `CIRCLE`, `ARC`, `TEXT` | 30 | 0 |
| `alignment-z` | `TEXT`, dopo `z` | 31 | 0 |
| `elevation` | `LWPOLYLINE` | 38 | 0 |
| `thickness` | tutte | 39 | 0 |
| `extrusion` | tutte, un vettore `x,y,z` | 210/220/230 | `0,0,1` |

```
LWPOLYLINE closed=yes width=0.5 vertex=0,0 start-width=1 end-width=2 vertex=10,0 bulge=0.5 vertex=10,10 elevation=3 thickness=2
LINE start=0,0 end=10,5 start-z=1 end-z=2
CIRCLE center=-5,5 radius=2 z=4 extrusion=0,0,-1
TEXT position=0,0 alignment-point=0,0 height=2.5 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="A" z=1 alignment-z=1 thickness=0.5 ~box=…
```

- Un vettore si scrive come un punto, con tre numeri: `0,0,-1`. Lo zero di
  una componente si scrive `0`, come ogni numero.
- Per un'entità con estrusione `(0,0,-1)` la geometria scritta è quella nel
  **sistema globale**, dopo la conversione dell'ADR 0018, punto 5 (per
  esempio il centro di un cerchio con la x cambiata di segno), seguita da
  `extrusion=0,0,-1`; le quote restano quelle del file. Un'entità con
  un'altra estrusione non è gestita e non compare tra le entità.
- `start-width` ed `end-width` si scrivono ciascuna solo se diversa da 0,
  come le altre: un segmento che si allarga da 0 a 2 ha solo `end-width=2`.

### Tappa 0.6: layer e proprietà

Le proprietà che ogni entità ha ([ADR 0023](../adr/0023-layer-e-proprieta.md),
punto 2). Come ogni proprietà nuova si scrivono **solo quando sono diverse
dal valore predefinito**, in fondo alla riga, dopo le quote, `thickness` ed
`extrusion` della tappa 0.5 e prima delle derivate, in quest'ordine:

| Proprietà | DXF | Valore | Predefinito |
|---|---|---|---|
| `layer` | 8 | nome tra virgolette (`layer="Muri esterni"`) | `"0"` |
| `color` | 62, 420 | `bylayer`, `byblock`, un indice da 1 a 255, o `r,g,b` (ADR 0024, punto 6) | `bylayer` |
| `linetype` | 6 | nome tra virgolette, anche `"ByBlock"` | `"ByLayer"` |
| `ltscale` | 48 | numero > 0, la scala del tipo di linea dell'entità | 1 |
| `lineweight` | 370 | `bylayer`, `byblock`, `default`, o millimetri con due decimali (`0.35`) o `0` (ADR 0025, punto 6) | `bylayer` |

```
LINE start=0,0 end=10,5 layer="Muri esterni" color=1 linetype="ACAD_ISO02W100" ltscale=0.5 lineweight=0.35
CIRCLE center=0,0 radius=2 thickness=1 color=255,128,0
TEXT position=0,0 … value="A" layer="Quote" ~box=…
```

- I nomi di layer e tipi di linea conservano maiuscole e minuscole, come
  nella tabella; sono tra virgolette, con gli stessi caratteri d'escape dei
  testi. Il confronto con il predefinito non distingue le maiuscole (`"0"`,
  `"BYLAYER"`).
- Un'entità di un disegno che non ha la tabella (un file letto prima del
  pacchetto 0.6-P10) ha i valori predefiniti e non scrive nulla.

**`LWPOLYLINE`** riceve anche una proprietà sua, `linetype-generation`
(DXF 70, bit 128: la generazione continua del tipo di linea, ADR 0025,
punto 4): `yes` o `no`, predefinito `no`, scritta solo se `yes`, **subito
dopo `closed=`** (`LWPOLYLINE closed=no linetype-generation=yes vertex=…`).
Il modello la riceve dal pacchetto 0.6-P10: fino ad allora le definizioni
la leggono e la controllano, ma il rapporto non la scrive.

### Tappa 0.8: entità composte

I sette tipi nuovi della tappa 0.8
([ADR 0044](../adr/0044-entita-composte-e-geometria-generata.md),
[ADR 0045](../adr/0045-blocchi-e-attributi.md),
[ADR 0046](../adr/0046-stili-di-testo-e-testo-multiriga.md),
[ADR 0047](../adr/0047-quote-e-stili-di-quota.md),
[ADR 0048](../adr/0048-retini.md),
[ADR 0052](../adr/0052-ellisse.md)) e gli `ATTRIB` degli inserimenti. Valgono
le regole di sempre: proprietà nell'ordine scritto qui; quelle con un valore
predefinito indicato si scrivono solo quando sono diverse; dopo le proprietà
del tipo vengono `thickness` ed `extrusion` della
[tappa 0.5](#tappa-05-quota-spessore-estrusione-e-larghezze) (questi tipi non
hanno quote, tranne `ATTRIB`, che ha quelle di `TEXT`), poi le proprietà della
[tappa 0.6](#tappa-06-layer-e-proprietà) e per ultime le derivate. Gli angoli
sono in gradi.

Il modulo `script` scrive e legge le righe di tutti e sette i tipi dal
pacchetto PT9. Finché il pacchetto di un tipo non è unito (`core::isSupported`
falso, PT6), la riga di quel tipo in una definizione delle domande si legge e
si controlla, poi si rifiuta con l'errore `<TIPO> is not supported yet.`
(`SOLID is not supported yet.`): i casi di quel tipo restano in attesa in
`tests/verifica/in-attesa/`.

**`SOLID`**

```
SOLID vertex=0,0 vertex=10,0 vertex=0,5 vertex=10,5
```

I quattro vertici nell'ordine del DXF (10, 11, 12, 13), sempre quattro; con
tre vertici nel file, il quarto è uguale al terzo.

**`ELLIPSE`**

```
ELLIPSE center=0,0 major-axis=10,0 ratio=0.5 start=0 end=90
```

| Proprietà | DXF | Valore |
|---|---|---|
| `center` | 10/20 | centro |
| `major-axis` | 11/21 | estremo dell'asse maggiore, **relativo al centro** |
| `ratio` | 40 | rapporto fra asse minore e maggiore, in (0, 1] |
| `start`, `end` | 41, 42 | parametri iniziale e finale **in gradi**, scritti solo per un arco d'ellisse: l'ellisse intera (0 e 360) non li scrive |

**`MTEXT`**

```
MTEXT insertion=0,0 height=2.5 width=60 attachment=top-left rotation=0 style="Standard" value="Prima\\Pseconda" ~lines=2
```

| Proprietà | DXF | Valore |
|---|---|---|
| `insertion` | 10/20 | punto d'inserimento |
| `height` | 40 | altezza, > 0 |
| `width` | 41 | larghezza di riferimento, 0 = senza a capo |
| `attachment` | 71 | `top-left`, `top-center`, `top-right`, `middle-left`, `middle-center`, `middle-right`, `bottom-left`, `bottom-center`, `bottom-right` |
| `rotation` | 50 | gradi |
| `style` | 7 | nome dello stile, tra virgolette |
| `line-spacing` | 44 | fattore d'interlinea, > 0; solo se diverso da 1 |
| `line-spacing-style` | 73 | `exactly`, solo se diverso da *at least* (che si legge come `at-least`) |
| `direction` | 72 | `by-style`, solo se diverso da sinistra a destra (che si legge come `left-to-right`) |
| `value` | 3 e 1 | il valore intero, codici di formattazione compresi, tra virgolette |
| `~lines` | — | **derivata**: il numero di righe della geometria generata, cioè delle sue forme `TEXT` ([ADR 0044](../adr/0044-entita-composte-e-geometria-generata.md), punto 2); si scrive solo quando il tipo è realizzato |

**`ATTDEF`, `ATTRIB` e `INSERT`**

```
ATTDEF position=0,0 alignment-point=0,0 height=1.8 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="N" tag="TG_PIN1" prompt="" invisible=yes constant=yes ~box=…
INSERT block="TG_EL_CONTACT_NO" insertion=10,20 scale=1,1 rotation=0
ATTRIB owner=300 position=… value="-K1" tag="TG_REF" ~box=…
```

- **`ATTDEF`**: le proprietà di `TEXT`, nel suo ordine, con `value` il valore
  predefinito; poi `tag` e `prompt` (tra virgolette, sempre), e `invisible`,
  `constant`, `verify`, `preset` (DXF 70, bit 1, 2, 4, 8), ognuna solo se
  `yes`. La derivata `~box` è quella di `TEXT`.
- **`ATTRIB`**: `owner` (l'identificativo del suo `INSERT`), le proprietà di
  `TEXT`, nel suo ordine, poi `tag` e `invisible`, solo se `yes`; dopo, le
  quote di `TEXT` (`z`, `alignment-z`), `thickness`, `extrusion` e le
  proprietà della tappa 0.6 dell'attributo; per ultima la derivata `~box`. Gli
  `ATTRIB` stanno nell'elenco `entities:` con il loro identificativo, come
  entità del DXF, nell'ordine degli identificativi come le altre, e contano in
  `statistics:` (`ATTRIB 2`). Non si scrivono nell'anteprima e non si
  definiscono da soli nelle domande: un `ATTRIB` appartiene al suo `INSERT`.
- **`INSERT`**: `block` (nome tra virgolette), `insertion`, `scale` (X,Y,
  numeri diversi da 0; uno negativo specchia), `rotation` (gradi).

**`DIMENSION`**

```
DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=50,0 line-point=50,10 rotation=0 text-point=25,10.625 measurement=50 ~text="50"
```

| Proprietà | Quando | Valore |
|---|---|---|
| `kind` | sempre | `linear`, `aligned`, `angular`, `radius`, `diameter`, `other` |
| `style` | sempre | nome dello stile, tra virgolette |
| `origin1`, `origin2` | lineare e allineata | 13/23 e 14/24 |
| `line-point` | lineare, allineata, angolare | 10/20 |
| `rotation` | lineare | 50, gradi |
| `first`, `second` | angolare | le due linee: `x1,y1;x2,y2` (13–14 e 10–15) |
| `arc-point` | angolare | 16/26 |
| `center`, `point` | radiale | 10/20 e 15/25 |
| `point1`, `point2` | diametrale | 15/25 e 10/20 |
| `type` | altro | il tipo DXF (70 & 7), da 0 a 7 |
| `text-point` | sempre | 11/21 |
| `measurement` | sempre | 42 |
| `text` | se non vuoto | il testo sostituito (1), tra virgolette |
| `text-rotation` | se diverso da 0 | 53, gradi |
| `user-text-position` | se `yes` | 70 bit 128 |
| `overrides` | se ce ne sono | le variabili essenziali sovrascritte, `NOME:valore` separate da virgole, in ordine alfabetico |
| `~text` | sempre, quando il tipo è realizzato | **derivata**: il testo che la grafica mostra, senza codici: i valori delle forme `TEXT` della geometria generata, con `%%c`, `%%d`, `%%p` e `%%%` convertiti, separati da un a capo (`\u{0A}`) se sono più d'uno |

Le proprietà si scrivono nell'ordine della tabella, solo quelle del tipo di
quota. In una quota angolare il punto 10 compare due volte, come
`line-point` e come primo punto di `second`: è lo stesso punto.

**`HATCH`**

```
HATCH pattern="ANSI31" solid=no angle=0 scale=1 islands=normal loop=external+polyline vertex=0,0 vertex=10,0 vertex=10,10 vertex=0,10 loop=edges circle=5,5;2 seed=1,1
```

| Proprietà | DXF | Valore |
|---|---|---|
| `pattern` | 2 | nome, tra virgolette |
| `solid` | 70 | `yes` o `no` |
| `angle`, `scale` | 52, 41 | gradi; scala o passo, > 0 |
| `double` | 77 | solo se `yes` |
| `islands` | 75 | `normal`, `outer`, `ignore` |
| `loop` | 92 | un contorno: i segnali `external`, `outermost`, `derived` (bit 1, 16, 4) che ha, uniti da `+` e in quest'ordine, poi `polyline` o `edges`; seguono i suoi elementi |
| `vertex`, `bulge` | 10/20, 42 | in un contorno a polilinea, come `LWPOLYLINE`; il contorno è sempre chiuso |
| `line`, `arc`, `circle` | 72 | in un contorno a lati: `line=x1,y1;x2,y2`, `arc=cx,cy;r;a1;a2` (gradi in [0, 360), sempre antiorario come `ARC`), `circle=cx,cy;r` (un arco intero) |
| `seed` | 10/20 dopo 98 | un punto interno; si ripete, dopo i contorni |

### Campi annunciati per la tappa 0.8.1

I pacchetti indicati aggiungono questi campi solo nelle condizioni elencate.

| Campo | Posizione e condizione | Pacchetto |
|---|---|---|
| `grid snap: unit=x,y base=x,y` | dopo `ortho:`, solo con lo snap alla griglia acceso | PT28 |
| `attdisp: on\|off` | dopo `lwdisplay:`, solo quando `$ATTMODE` è 2 (`on`) o 0 (`off`) | PT27 |
| `active snap: Nearest x,y` | quando il puntatore aggancia uno snap *Vicino* | PT30 |
| `extension x,y` e `parallel x1,y1 x2,y2` | in `acquired:`, rispettivamente l'estremo e gli estremi della linea acquisita | PT31 |
| `Extension x,y from x,y`, `Parallel x,y from x,y` e i relativi incroci | in `active tracking:`, quando la guida o l'incrocio aggancia il puntatore ([ADR 0057](../adr/0057-snap-vicino-prolungamento-parallelo.md), punto 5) | PT31 |
| `lock-position=yes` | in `ATTDEF` dopo `preset`, in `ATTRIB` dopo `invisible`, solo se la posizione è bloccata | PT15 |
| `origin=x,y` | in `HATCH` dopo `islands`, se l'origine del motivo (XDATA `ACAD` 1010/1020) è presente e diversa da `0,0` | PT23 |
| `face=bold`, `face=italic` o `face=bold-italic` | nei `TEXT` di `generated`, dopo `vertical`, solo per una variante diversa da Regular | PT8 |

Nei DXF già presenti in `dxf-test-file` l'origine del motivo è sempre `0,0`.
Per le righe di rapporto attese dopo un cambiamento di comportamento si
applica la regola del [confine di un caso in attesa](casi.md#il-confine-di-un-caso-in-attesa).

### Definizioni nelle domande

Nelle [domande al modello](domande.md) un'entità si scrive nello stesso
formato, con tre libertà:

- le proprietà si possono scrivere in qualunque ordine, ognuna una volta sola
  (tranne `vertex` e `bulge` di `LWPOLYLINE`, il cui ordine conta);
- le proprietà con un valore predefinito si possono omettere:
  `LWPOLYLINE` `closed=no`; `TEXT` `alignment-point=0,0` (come nel DXF),
  `height=2.5`, `rotation=0`, `width-factor=1`, `oblique=0`,
  `style="Standard"`, `mirror-x=no`, `mirror-y=no`, `horizontal=left`,
  `vertical=baseline`. Le altre sono obbligatorie;
- le proprietà derivate (`~…`) si accettano e si ignorano, così si può copiare
  una riga del rapporto.

I tipi della [tappa 0.8](#tappa-08-entità-composte) si scrivono con le stesse
libertà. Si possono omettere: `ELLIPSE` `start` ed `end` insieme (l'ellisse
intera); `MTEXT` `height=2.5`, `width=0`, `attachment=top-left`,
`rotation=0`, `style="Standard"`, `line-spacing=1`,
`line-spacing-style=at-least`, `direction=left-to-right`; `ATTDEF` quelle
di `TEXT` e `prompt=""`, `invisible`, `constant`, `verify`, `preset` (`no`);
`INSERT` `scale=1,1`, `rotation=0`; `DIMENSION` `style="ISO-25"`,
`rotation=0`, `text`, `text-rotation`, `user-text-position=no`,
`overrides`, e nella quota angolare `line-point` (se c'è, deve essere il
primo punto di `second`); `HATCH` `solid=no`, `angle=0`, `scale=1`,
`double=no`, `islands=normal`, i contorni e i punti interni. Una proprietà
di un altro tipo di quota (`center` in una quota lineare) è un errore.

Le proprietà della [tappa 0.5](#tappa-05-quota-spessore-estrusione-e-larghezze)
si accettano come le altre, dal pacchetto che le realizza, e si possono
omettere: valgono i loro valori predefiniti.

Le proprietà della [tappa 0.6](#tappa-06-layer-e-proprietà) si scrivono in
qualunque ordine dopo il tipo, ognuna una volta sola, e si possono omettere:
valgono i loro **valori predefiniti** (layer `"0"`, `bylayer`…), mai i
valori correnti cambiati con `!current`. Si accettano tutte le scritture
dell'ADR 0024, punto 6 (`color=red`, `color=256`) e degli spessori
(`lineweight=0.5`, `0.35mm`); i nomi senza distinguere le maiuscole. Un
layer o un tipo di linea non definito sopra (`LAYER`, `LTYPE`, o uno di
quelli che ogni disegno ha) è un errore del file di domande.

Un'entità scritta dal rapporto si rilegge senza perdite: i numeri hanno tutte
le cifre che servono. Per gli angoli, convertiti tra gradi e radianti, vale la
tolleranza del confronto.

## Come cresce il formato

- Una **proprietà nuova** di un'entità (per esempio il layer, tappa 0.6) si
  scrive **solo quando è diversa dal valore predefinito**, in fondo alla riga
  prima delle derivate. Così i rapporti già scritti restano validi.
- Un **campo nuovo** dello stato si aggiunge solo con lo stesso criterio (si
  scrive solo quando ha un valore diverso da quello di una sessione nuova),
  come già fa `snap:` con i modi spenti.
- Un **tipo nuovo** di entità si aggiunge a questo documento, con le sue
  proprietà, prima del pacchetto che lo realizza.
- Ogni altro cambiamento richiede un nuovo ADR e una nuova versione nella prima
  riga (`tecnigrafo-report 2`).
