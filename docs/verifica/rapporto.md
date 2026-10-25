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
| `unhandled:` | solo se ce ne sono: le entità non gestite, per tipo |
| `pointer:` | l'ultimo punto del disegno in cui è stato portato il puntatore (`!move`, `!click`), o `none` |
| `active snap:` | lo snap su cui si trova il puntatore mentre un comando chiede un punto: il tipo e il punto (`Endpoint 100,50`), o `none` |
| `statistics:` | per ogni tipo DXF presente nel disegno, in ordine alfabetico, il tipo, uno spazio e il numero di entità (`LWPOLYLINE 2`); i tipi assenti non si scrivono |

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
